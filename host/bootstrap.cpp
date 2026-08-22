#include "bootstrap.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <poll.h>
#include <chrono>
#include <cstring>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <cerrno>


// Wire format for a single bootstrap exchange message (17 bytes, no
// padding): the sender's id, its QPN, the device address of its
// RDMA-exposed tree memory, and the root pointer of its tree (0 if the
// sender serves no tree).
#pragma pack(push, 1)
struct QpnMsg {
    uint8_t  node_id;
    int32_t  qpn;
    uint64_t vaddr;
    bptr_t   root;
};
#pragma pack(pop)

// Keep the already-established bootstrap connections alive through FPGA RoCE
// setup. The same sockets then provide an ordered readiness rendezvous without
// another port, connection race, or firewall requirement.
static std::vector<int> bootstrap_peer_sockets;


std::vector<NodeConfig> parse_node_config(const std::string& path) {
    std::vector<NodeConfig> nodes;
    std::ifstream f(path);
    if (!f) throw std::runtime_error("Cannot open node config: " + path);
    std::string line;
    while (std::getline(f, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream ss(line);
        int id;
        std::string host_ip, fpga_ip;
        if (!(ss >> id >> host_ip))
            throw std::runtime_error("Bad config line: " + line);
        ss >> fpga_ip; // optional third column
        nodes.push_back({(node_id_t)id, host_ip, fpga_ip});
    }
    return nodes;
}


static void send_exact(int fd, const void* buf, size_t len) {
    const char* p = static_cast<const char*>(buf);
    while (len > 0) {
        ssize_t n = ::send(fd, p, len, 0);
        if (n <= 0) throw std::runtime_error("send failed");
        p += n; len -= n;
    }
}


static void recv_exact(int fd, void* buf, size_t len) {
    char* p = static_cast<char*>(buf);
    while (len > 0) {
        ssize_t n = ::recv(fd, p, len, 0);
        if (n <= 0) throw std::runtime_error("recv failed");
        p += n; len -= n;
    }
}


static int make_server_socket(uint16_t port, int backlog) {
    int srv = ::socket(AF_INET, SOCK_STREAM, 0);
    if (srv < 0) throw std::runtime_error("socket() failed");

    int opt = 1;
    ::setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(port);
    if (::bind(srv, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        ::close(srv);
        throw std::runtime_error("bind() failed on port " + std::to_string(port));
    }
    if (::listen(srv, backlog + 1) < 0) {
        ::close(srv);
        throw std::runtime_error("listen() failed on port " + std::to_string(port));
    }
    return srv;
}


static int connect_with_retry(const std::string& host, uint16_t port) {
    sockaddr_in peer{};
    peer.sin_family = AF_INET;
    peer.sin_port   = htons(port);
    if (::inet_pton(AF_INET, host.c_str(), &peer.sin_addr) != 1)
        throw std::runtime_error("Bad peer IPv4 address: " + host);

    for (;;) {
        int s = ::socket(AF_INET, SOCK_STREAM, 0);
        if (s < 0) throw std::runtime_error("socket() failed");
        if (::connect(s, reinterpret_cast<sockaddr*>(&peer), sizeof(peer)) == 0)
            return s;
        ::close(s);
        ::usleep(50000); // 50 ms
    }
}


using ReadyClock = std::chrono::steady_clock;
using ReadyDeadline = ReadyClock::time_point;


static int remaining_ms(const ReadyDeadline& deadline) {
    auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
        deadline - ReadyClock::now()).count();
    return remaining > 0 ? static_cast<int>(remaining) : 0;
}


static short poll_until(int fd, short events, const ReadyDeadline& deadline,
                        const char* operation) {
    for (;;) {
        int timeout = remaining_ms(deadline);
        if (timeout == 0)
            throw std::runtime_error(std::string(operation) + " timed out");

        pollfd pfd{fd, events, 0};
        int rc = ::poll(&pfd, 1, timeout);
        if (rc > 0) return pfd.revents;
        if (rc == 0)
            throw std::runtime_error(std::string(operation) + " timed out");
        if (errno != EINTR)
            throw std::runtime_error(std::string(operation) + " failed");
    }
}


// Send this node's info and record the peer's in the config tables.
static void exchange_qpn(int sock, node_id_t my_id, int my_qpn,
                         uint64_t my_vaddr, bptr_t my_root,
                         RdmaConfig& cfg) {
    QpnMsg out_msg = {my_id, my_qpn, my_vaddr, my_root};
    QpnMsg in_msg;
    // Send before receive — both sides do the same, so no deadlock (17
    // bytes fit comfortably in the kernel's TCP send buffer).
    send_exact(sock, &out_msg, sizeof(out_msg));
    recv_exact(sock, &in_msg, sizeof(in_msg));
    cfg.qpn_table[in_msg.node_id]   = in_msg.qpn;
    cfg.vaddr_table[in_msg.node_id] = in_msg.vaddr;
    cfg.root_table[in_msg.node_id]  = in_msg.root;
}


RdmaConfig bootstrap_rdma(
    node_id_t                      my_id,
    const std::vector<NodeConfig>& nodes,
    int                            local_qpn,
    uint64_t                       local_vaddr,
    bptr_t                         local_root
) {
    for (int s : bootstrap_peer_sockets) ::close(s);
    bootstrap_peer_sockets.clear();

    RdmaConfig cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.my_node_id = my_id;
    cfg.qpn_table[my_id]   = local_qpn;
    cfg.vaddr_table[my_id] = local_vaddr;
    cfg.root_table[my_id]  = local_root;

    // Count peers that will connect to this node.
    int n_lower = 0;
    for (auto& nc : nodes) {
        if (nc.id < my_id) ++n_lower;
    }

    // Open a server socket so peers with lower ids can connect to us.
    int srv = make_server_socket(BOOTSTRAP_PORT, n_lower);

    // Phase 1: connect to all peers with higher node_id.
    // Lower-id nodes initiate so we never have a circular wait.
    for (auto& nc : nodes) {
        if (nc.id <= my_id) continue;
        int s = connect_with_retry(nc.host_ip, BOOTSTRAP_PORT);
        exchange_qpn(s, my_id, local_qpn, local_vaddr, local_root, cfg);
        bootstrap_peer_sockets.push_back(s);
    }

    // Phase 2: accept from all peers with lower node_id.
    // They've already finished their connect phase and are in exchange_qpn,
    // or will connect imminently (their data is buffered by TCP).
    for (int i = 0; i < n_lower; ++i) {
        int s = ::accept(srv, nullptr, nullptr);
        if (s < 0) throw std::runtime_error("accept() failed");
        exchange_qpn(s, my_id, local_qpn, local_vaddr, local_root, cfg);
        bootstrap_peer_sockets.push_back(s);
    }

    ::close(srv);
    return cfg;
}


static void send_ready(int sock, bool local_ready,
                       const ReadyDeadline& deadline) {
    const uint8_t out = local_ready ? 1 : 0;
    (void)poll_until(sock, POLLOUT, deadline, "RoCE readiness send");
    if (::send(sock, &out, sizeof(out), MSG_NOSIGNAL) !=
        static_cast<ssize_t>(sizeof(out)))
        throw std::runtime_error("RoCE readiness send failed");
}


static bool receive_ready(int sock, const ReadyDeadline& deadline) {
    uint8_t in = 0;
    (void)poll_until(sock, POLLIN, deadline, "RoCE readiness receive");
    if (::recv(sock, &in, sizeof(in), 0) !=
        static_cast<ssize_t>(sizeof(in)))
        throw std::runtime_error("RoCE readiness receive failed");
    return in == 1;
}


bool synchronize_roce_ready(
    node_id_t                      my_id,
    const std::vector<NodeConfig>& nodes,
    bool                           local_ready
) {
    size_t expected_peers = 0;
    for (const auto& nc : nodes)
        if (nc.id != my_id) ++expected_peers;
    if (bootstrap_peer_sockets.size() != expected_peers)
        throw std::runtime_error("RoCE readiness called without all bootstrap peers");

    bool all_ready = local_ready;
    const ReadyDeadline deadline = ReadyClock::now() +
        std::chrono::seconds(ROCE_READY_TIMEOUT_SECONDS);

    try {
        for (int s : bootstrap_peer_sockets)
            send_ready(s, local_ready, deadline);
        for (int s : bootstrap_peer_sockets)
            all_ready = receive_ready(s, deadline) && all_ready;
    }
    catch (...) {
        for (int s : bootstrap_peer_sockets) ::close(s);
        bootstrap_peer_sockets.clear();
        throw;
    }

    for (int s : bootstrap_peer_sockets) ::close(s);
    bootstrap_peer_sockets.clear();
    return all_ready;
}
