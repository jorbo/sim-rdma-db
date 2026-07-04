#include "bootstrap.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <cerrno>


// Wire format for a single bootstrap exchange message (13 bytes, no
// padding): the sender's id, its QPN, and the device address of its
// RDMA-exposed tree memory.
#pragma pack(push, 1)
struct QpnMsg {
    uint8_t  node_id;
    int32_t  qpn;
    uint64_t vaddr;
};
#pragma pack(pop)


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


// Send this node's info and record the peer's in the config tables.
static void exchange_qpn(int sock, node_id_t my_id, int my_qpn,
                         uint64_t my_vaddr, RdmaConfig& cfg) {
    QpnMsg out_msg = {my_id, my_qpn, my_vaddr};
    QpnMsg in_msg;
    // Send before receive — both sides do the same, so no deadlock (13
    // bytes fit comfortably in the kernel's TCP send buffer).
    send_exact(sock, &out_msg, sizeof(out_msg));
    recv_exact(sock, &in_msg, sizeof(in_msg));
    cfg.qpn_table[in_msg.node_id]   = in_msg.qpn;
    cfg.vaddr_table[in_msg.node_id] = in_msg.vaddr;
}


RdmaConfig bootstrap_rdma(
    node_id_t                      my_id,
    const std::vector<NodeConfig>& nodes,
    int                            local_qpn,
    uint64_t                       local_vaddr
) {
    RdmaConfig cfg;
    cfg.my_node_id = my_id;
    memset(cfg.qpn_table, 0, sizeof(cfg.qpn_table));
    memset(cfg.vaddr_table, 0, sizeof(cfg.vaddr_table));
    cfg.qpn_table[my_id]   = local_qpn;
    cfg.vaddr_table[my_id] = local_vaddr;

    // Count peers on each side.
    int n_lower = 0, n_higher = 0;
    for (auto& nc : nodes) {
        if (nc.id < my_id) ++n_lower;
        if (nc.id > my_id) ++n_higher;
    }

    // Open a server socket so peers with lower ids can connect to us.
    int srv = ::socket(AF_INET, SOCK_STREAM, 0);
    if (srv < 0) throw std::runtime_error("socket() failed");
    int opt = 1;
    ::setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(BOOTSTRAP_PORT);
    if (::bind(srv, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0)
        throw std::runtime_error("bind() failed on port " + std::to_string(BOOTSTRAP_PORT));
    ::listen(srv, n_lower + 1);

    // Phase 1: connect to all peers with higher node_id.
    // Lower-id nodes initiate so we never have a circular wait.
    for (auto& nc : nodes) {
        if (nc.id <= my_id) continue;
        int s = ::socket(AF_INET, SOCK_STREAM, 0);
        sockaddr_in peer{};
        peer.sin_family = AF_INET;
        ::inet_pton(AF_INET, nc.host_ip.c_str(), &peer.sin_addr);
        peer.sin_port = htons(BOOTSTRAP_PORT);
        // Retry until the remote server socket is ready.
        while (::connect(s, reinterpret_cast<sockaddr*>(&peer), sizeof(peer)) != 0) {
            ::usleep(50000); // 50 ms
        }
        exchange_qpn(s, my_id, local_qpn, local_vaddr, cfg);
        ::close(s);
    }

    // Phase 2: accept from all peers with lower node_id.
    // They've already finished their connect phase and are in exchange_qpn,
    // or will connect imminently (their data is buffered by TCP).
    for (int i = 0; i < n_lower; ++i) {
        int s = ::accept(srv, nullptr, nullptr);
        if (s < 0) throw std::runtime_error("accept() failed");
        exchange_qpn(s, my_id, local_qpn, local_vaddr, cfg);
        ::close(s);
    }

    ::close(srv);
    return cfg;
}
