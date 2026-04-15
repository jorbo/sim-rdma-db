# RoCE v2 HLS IP

You can simulate the RoCE stack here. To package IP, go to the root directory of fpga-network-stack repo.

## Prerequisites

- Xilinx Vivado 2019.2
- cmake 3.0 or higher

Supported boards (out of the box)
- Xilinx VC709
- Xilinx VCU118
- Alpha Data ADM-PCIE-7V3
- Alveo u280 (new added)

## Simulate with HLS code

1. Create a build directory and configure build

```bash
$ mkdir build
$ cd build
$ cmake .. -DDATA_WIDTH=64 -DCLOCK_PERIOD=3.2 -DFPGA_PART=xcu280-fsvh2892-2L-e
```

2. Run c-simulation

```bash
$ make csim
```

## Test example for RoCE stack

### TX test

To test tx path, use function `test_tx_debug()` (around line 439) in file `test_rocev2.cpp`.

1. First provide Infiniband **QP context** and RDMA **connection infomation** as well as the **IP addresses** to the stack (around line 473-493). These interface definition in HLS:

```cpp
typedef enum {RESET, INIT, READY_RECV, READY_SEND, SQ_ERROR, ERROR} qpState;

struct qpContext
{
    qpState     newState;
    ap_uint<24> qp_num;
    ap_uint<24> remote_psn;
    ap_uint<24> local_psn;
    ap_uint<16> r_key;
    ap_uint<48> virtual_address;
};
struct ifConnReq
{
    ap_uint<16> qpn;
    ap_uint<24> remote_qpn;
    ap_uint<128> remote_ip_address;
    ap_uint<16> remote_udp_port;
};

hls::stream<qpContext>& s_axis_qp_interface,
hls::stream<ifConnReq>& s_axis_qp_conn_interface,
```

2. Then write **`txMeta`** to issue the RDMA operation(around line 540). The `txMeta` interface definition in HLS:

```cpp
typedef enum {APP_READ, APP_WRITE, APP_PART, APP_POINTER, APP_READ_CONSISTENT} appOpCode;

struct txMeta
{
    appOpCode   op_code;
    ap_uint<24> qpn;
    ap_uint<48> local_vaddr;
    ap_uint<48> remote_vaddr;
    ap_uint<32> length;
};
hls::stream<txMeta>& s_axis_tx_meta,
hls::stream<net_axis<WIDTH> >& s_axis_tx_data,
```

3. Ensure that the `test_tx_debug()` function is called in the main function (around bottom). Then run `csim` to perform c-simulation. You can get the output packet in file `rdma_txwerteread.out`.

---

For example, to issue a RDMA write command. This example issues a RDMA write. The data to be transmitted is located at local memory with `local_address=dramPtr` and `length=128`.

```txt
------------------------------------------------------
 [tx RDMA write]
------------------------------------------------------
         LOCAL          REMOTE
IP:     0b01d4d2    -> 0b01d4d1
UDP:    4853        -> 12b7
QPN:    11          -> 12
PSN:    9dbe5d      -> 9dbe5d

OP:     APP_WRITE
QPN:    11
l_Addr: dramPtr
r_Addr: 0x7fc292600000
len:    128 = 0x80

------------------------------------------------------
output packet:

45 00 00 7c 00 00 00 00 40 11 00 00 0b 01 d4 d2 ffff 0
0b 01 d4 d1 48 53 12 b7 00 68 00 00 06 00 ff ff ffff 0
00 00 00 12 80 9d be 5d 00 00 7f c2 92 60 00 00 ffff 0
00 00 00 00 00 00 00 80 14 00 00 00 00 00 00 00 ffff 0
1c 00 00 00 00 00 00 00 24 00 00 00 00 00 00 00 ffff 0
2c 00 00 00 00 00 00 00 34 00 00 00 00 00 00 00 ffff 0
3c 00 00 00 00 00 00 00 44 00 00 00 00 00 00 00 ffff 0
4c 00 00 00 00 00 00 00 ef be ad de 00 00 00 00 0fff 1

45 00 00 6c 00 00 00 00 40 11 00 00 0b 01 d4 d2 ffff 0
0b 01 d4 d1 48 53 12 b7 00 58 00 00 08 00 ff ff ffff 0
00 00 00 12 80 9d be 5e 54 00 00 00 00 00 00 00 ffff 0
5c 00 00 00 00 00 00 00 64 00 00 00 00 00 00 00 ffff 0
6c 00 00 00 00 00 00 00 74 00 00 00 00 00 00 00 ffff 0
7c 00 00 00 00 00 00 00 84 00 00 00 00 00 00 00 ffff 0
8c 00 00 00 00 00 00 00 ef be ad de 00 00 00 00 0fff 1

------------------------------------------------------
```

### RX test

To test rx path, use tx test to generate the test packet and feed it into the stack.

1. Follow TX test to generate the ingress packets. Then copy the packets from `rdma_txwerteread.out` to file `write_read_read_large_receiver.in`.
2. Ensure that the `test_rx()` function is called in the main function (around bottom). Then run `csim` to prform c-simulation. You can find the output packets in the file `write_read_read_large_receiver.out`.

---

For example, to consume a RDMA write and a RDMA read command. The ingress packets are already provided in file `write_read_read_large_receiver.in`.

In this example, the local memoey is initiated to all zero. The incomming packets contain a RDMA write command and a RDMA read command. A block of data with `length=0x80` should be written to the local memory at `virtual_address=0x00`. Then the read command will read the memory from `virtual_address=0x10` with a `length= 0x40`. You can see from the last output packet that the corrent data has been read back.

```txt
------------------------------------------------------
 [rx RDMA write + read]
------------------------------------------------------
         REMOTE          LOCAL
IP:     0b01d4d1    -> 0b01d4d2
UDP:    4853        -> 12b7
QPN:    12          -> 11
PSN:    9dbe5d      -> 9dbe5d

RDMA_WRITE: vaddr = 0x00, len = 0x80
RDMA_READ: vaddr = 0x10, len = 0x40

------------------------------------------------------
rx ingress packets:

pkt 1 - RDMA_WRITE_FIRST: RETH + payld
45 00 00 7c 00 00 00 00 40 11 00 00 0b 01 d4 d1 ffff 0
0b 01 d4 d2 48 53 12 b7 00 68 00 00 06 00 ff ff ffff 0
00 00 00 11 80 9d be 5d 00 00 00 00 00 00 00 00 ffff 0
00 00 00 00 00 00 00 80 14 00 00 00 00 00 00 00 ffff 0
1c 00 00 00 00 00 00 00 24 00 00 00 00 00 00 00 ffff 0
2c 00 00 00 00 00 00 00 34 00 00 00 00 00 00 00 ffff 0
3c 00 00 00 00 00 00 00 44 00 00 00 00 00 00 00 ffff 0
4c 00 00 00 00 00 00 00 ef be ad de 00 00 00 00 0fff 1

pkt 2 - RDMA_WRITE_LAST: payld
45 00 00 6c 00 00 00 00 40 11 00 00 0b 01 d4 d1 ffff 0
0b 01 d4 d2 48 53 12 b7 00 58 00 00 08 00 ff ff ffff 0
00 00 00 11 80 9d be 5e 54 00 00 00 00 00 00 00 ffff 0
5c 00 00 00 00 00 00 00 64 00 00 00 00 00 00 00 ffff 0
6c 00 00 00 00 00 00 00 74 00 00 00 00 00 00 00 ffff 0
7c 00 00 00 00 00 00 00 84 00 00 00 00 00 00 00 ffff 0
8c 00 00 00 00 00 00 00 ef be ad de 00 00 00 00 0fff 1

pkt 3 - RDMA_READ_ONLY: RETH + payld
45 00 00 3c 00 00 00 00 40 11 00 00 0b 01 d4 d1 ffff 0
0b 01 d4 d2 48 53 12 b7 00 28 00 00 0c 00 ff ff ffff 0
00 00 00 11 80 9d be 5f 00 00 00 00 00 00 00 10 ffff 0
00 00 00 00 00 00 00 40 ef be ad de 00 00 00 00 0fff 1

------------------------------------------------------
tx output packets:

pkt 1 - ACK: AETH
45 00 00 30 00 00 00 00 40 11 00 00 0b 01 d4 d2 ffff 0
0b 01 d4 d1 12 b7 48 53 00 1c 00 00 11 00 ff ff ffff 0
00 00 00 12 00 9d be 5d 1f 00 00 01 ef be ad de ffff 1

pkt 2 - ACK: AETH
45 00 00 30 00 00 00 00 40 11 00 00 0b 01 d4 d2 ffff 0
0b 01 d4 d1 12 b7 48 53 00 1c 00 00 11 00 ff ff ffff 0
00 00 00 12 00 9d be 5e 1f 00 00 02 ef be ad de ffff 1

pkt 3 - RDMA_READ_RESP_ONLY: AETH + payld
45 00 00 70 00 00 00 00 40 11 00 00 0b 01 d4 d2 ffff 0
0b 01 d4 d1 12 b7 48 53 00 5c 00 00 10 00 ff ff ffff 0
00 00 00 12 00 9d be 5f 1f 00 00 03 24 00 00 00 ffff 0
00 00 00 00 2c 00 00 00 00 00 00 00 34 00 00 00 ffff 0
00 00 00 00 3c 00 00 00 00 00 00 00 44 00 00 00 ffff 0
00 00 00 00 4c 00 00 00 00 00 00 00 54 00 00 00 ffff 0
00 00 00 00 5c 00 00 00 00 00 00 00 ef be ad de ffff 1
------------------------------------------------------
```

## RoCE packet explaination

### RDMA write

```txt
45 00 00 7c 00 00 00 00 40 11 00 00 0b 01 d4 d2 ffff 0
<------------------------IP--------------------
0b 01 d4 d1 48 53 12 b7 00 68 00 00 06 00 ff ff ffff 0
----------> <-----UDP-------------> <----------
00 00 00 12 80 9d be 5d 00 00 7f c2 92 60 00 00 ffff 0
----BTH---------------> <-----RETH-------------
00 00 00 00 00 00 00 80 14 00 00 00 00 00 00 00 ffff 0
----------------------> <-----payload----------
1c 00 00 00 00 00 00 00 24 00 00 00 00 00 00 00 ffff 0
2c 00 00 00 00 00 00 00 34 00 00 00 00 00 00 00 ffff 0
3c 00 00 00 00 00 00 00 44 00 00 00 00 00 00 00 ffff 0
4c 00 00 00 00 00 00 00 ef be ad de 00 00 00 00 0fff 1
---payload------------> <--ICRC--->
```

### RDMA read ACK

```txt
45 00 00 70 00 00 00 00 40 11 00 00 0b 01 d4 d2 ffff 0
<------------------------IP--------------------
0b 01 d4 d1 12 b7 48 53 00 5c 00 00 10 00 ff ff ffff 0
----------> <-----UDP-------------> <----------
00 00 00 12 00 9d be 5f 1f 00 00 03 24 00 00 00 ffff 0
----BTH---------------> <---AETH--> <--payload-
00 00 00 00 2c 00 00 00 00 00 00 00 34 00 00 00 ffff 0
00 00 00 00 3c 00 00 00 00 00 00 00 44 00 00 00 ffff 0
00 00 00 00 4c 00 00 00 00 00 00 00 54 00 00 00 ffff 0
00 00 00 00 5c 00 00 00 00 00 00 00 ef be ad de ffff 1
---payload------------------------> <--ICRC--->
```

### header explaination

#### IP

```txt
45 00 00 4c 00 00 00 00 40 11 00 00 0b 01 d4 d1
      totLen               ptc      <--lIP---->
0b 01 d4 d2 
<--rIP---->
```

#### UDP

```txt
            c0 00 12 b7 00 38 00 00
            <Psrc><Pdst><len>      
```

#### BTH (Base transport header)

```txt
                                    06 00 ff ff
                                    Op    <Pkey>
00 00 00 12 80 9d be 5d
   <-QPN-->    <-PSN-->
```

#### RETH (RDMA extended transport header)

```txt
                        00 00 7f c2 92 60 00 00
                        <-----vaddr----------->
00 00 00 00 00 00 00 80
<--rKey---> <-DMAlen-->
```

#### AETH (ACK extended transport header)

```txt
                        1f 00 00 03
                        ack<--MSN->
```

## Misc

- To change **MTU**, modify `PMTU` in `rocev2_config.hpp.in`.
- The "[PASSED]" message of stdout doesn't mean anything.
