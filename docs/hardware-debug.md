# Hardware debug recipe (two-node U280 on CloudLab OCT)

Everything below runs from the repo root on the build host (wolverine)
unless a line says otherwise. Node roles: **node 0 = table**
(`krnl.server0.xclbin`), **node 1 = head** (`krnl.server1.xclbin`).

## 1. Run the test

```
make host TARGET=hw PLATFORM=xilinx_u280_gen3x16_xdma_1_202211_1
scripts/rebuild-hw.sh --platform xilinx_u280_gen3x16_xdma_1_202211_1 --server all --no-host
scripts/deploy-run.sh <table-user@host> <head-user@host> nodes.cfg
```

Useful environment: `RDMA_SELFTEST=1` issues one host-driven RDMA READ of
the table's leaf 0 before the B-tree run (expect landing bytes
`1 2 ffffffff ...`), `ILA_ARMING=1` pauses the head until you press ENTER
so an ILA can be armed first, `RUN_TIMEOUT=<s>` declares a hang and
collects `debug.head.log` / `debug.table.log` (CU status, debug-IP
status, dmesg if readable, `xrt-kds.log`) **before** resetting the cards.

## 2. Is the kernel stuck, and where?

On the hung node, before anything resets it:

```
source /opt/xilinx/xrt/setup.sh
xbutil examine -d <bdf> -r dynamic-regions      # bdf from `xbutil examine`
```

| `krnl_1` status | meaning |
|---|---|
| `START` | kernel launched, stuck in its own datapath |
| `IDLE` / never started | XRT/KDS launch problem, check `xrt-kds.log` |
| `rocetest_krnl_1` not `DONE\|IDLE` | QP setup never completed |

## 3. Debug bridge (XVC) for the ILAs

The ILA cores are reached over PCIe XVC, not JTAG. Use the native ELF
from the Vivado install; the `xvc_pcie.zip` driver build does not open the
device.

On the FPGA node (xclbin must be loaded). The remote home is `/users/<user>`
on OCT nodes; `btree-run` only exists once `deploy-run.sh` has run there,
so create it on a fresh node:
```
ssh <node> 'mkdir -p ~/btree-run'
scp -p /home/Xilinx/Vivado/2023.2/bin/unwrapped/lnx64.o/xvc_pcie <node>:~/btree-run/xvc_pcie
ssh <node>
source /opt/xilinx/xrt/setup.sh
ls -l /dev/xfpga/xvc_pub.*                       # suffix differs per machine
~/btree-run/xvc_pcie -d /dev/xfpga/xvc_pub.u<NNN> -s TCP::10200
```

On wolverine (one node at a time; re-point the tunnel to switch nodes):
```
ssh -N -L 10200:localhost:10200 <node> &
hw_server -s TCP::3121 -e "set auto-open-servers xilinx-xvc:localhost:10200"
```

On wolverine `/tmp` is mounted `noexec`, and `cs_server` (launched by
`vivado_lab`) must map a bundled `libz.so.1` from its temp dir, failing
with `libz.so.1: failed to map segment from shared object`. Point the temp
dir somewhere executable before running `vivado_lab`:
```
mkdir -p .vivado-tmp
export TMPDIR=$PWD/.vivado-tmp TMP=$TMPDIR TEMP=$TMPDIR
```
Set all three to the same absolute path, or unset all three. An *empty*
`TMPDIR`/`TMP`/`TEMP` makes Vivado (including a v++ link run) try to
create `/<pid>` at the filesystem root and fail with
`[Common 17-1974] Error while creating directory path /<pid>`, hours into
place-and-route. Check with `env | grep -i '^tmp\|^temp'` before a build.

`scripts/ila-capture.tcl` connects to `localhost:3121`. Do **not** use
`scripts/ila-capture.sh` without `--host` in this arrangement: its local
mode tries to start another `hw_server` on 3121.

## 4. Capturing

Probes file: `build_dir.hw.<xsa>.server<N>/krnl.link.ltx` from the **same**
link run as the deployed xclbin.

Snapshot (free-running counters, all six ILAs, no workload needed):
```
vivado_lab -mode batch -nolog -nojournal -source scripts/ila-capture.tcl \
  -tclargs build_dir.hw.<xsa>.server1/krnl.link.ltx head-snap snapshot
```

Trigger (arm, then press ENTER at the `ILA_ARMING=1` prompt):
```
vivado_lab -mode batch -nolog -nojournal -source scripts/ila-capture.tcl \
  -tclargs build_dir.hw.<xsa>.server1/krnl.link.ltx head-trig trigger '<probe-glob>' '' '' 64
```
Only the ILA whose probe list matches the glob is armed; the others are
skipped. Useful globs: `'*net_slot_0_axis_tvalid'` (ILA 6, kernel
`tx_meta` valid), `'*s_axis_roce_role_tx_meta*valid*'` (ILA 3, stack side),
`'*axis_roce_to_roce_slice*valid*'` (ILA 4, RoCE tx).

Which ILA is what (head build): 1/2 CMAC rx/tx counters, 3 stack_top
network + role tx_meta, 4 rocev2 internals + packet counters, 5 DataMover
write path, 6 system ILA on `krnl_1` (`tx_meta`, gmem1, gmem3).

Caveat: a **snapshot** shows register contents, and HLS SRL FIFO outputs
and AXI skid buffers hold stale values from earlier runs. Only triggered
captures and counters are evidence of what *this* run did.

## 5. Reading the counters (ILA 4, both nodes)

After a passing run with N searches at depth 2:

| counter | head | table |
|---|---|---|
| `roce_tx_pkg_counter` | 2N | 2N |
| `roce_rx_pkg_counter` | 2N | 2N |
| `regCrcDropPkgCount` | 0 | 0 |
| `regInvalidPsnDropCount` | **1** | **1** |

The PSN drop count reads 1 on both nodes even with zero RoCE traffic; it
is a startup artifact of the stack. Treat 1 as baseline, only >1 is a
real drop. CMAC `tx_total_bytes / tx_total_packets == 68` means only
ARP-sized frames left, i.e. no RDMA request was ever sent.

## 6. Checking the HLS schedule (no hardware needed)

After `make installip-hls`, verify stream ordering in the kernel:
```
grep -nE '^ST_[0-9]+ :.*(s_axis_completion|m_axis_tx_meta|free_slots|gmem5)' \
  build/krnl/krnl_prj/solution1/.autopilot/db/sm_search*.verbose.sched.rpt | cut -c1-150
```
Rules that have bitten us: a blocking stream read must not share an FSM
state with a write it depends on (the `tx_meta` write / completion read
hang, fixed with a PROTOCOL region, later replaced by non-blocking
`nbreadreq`); a slot must be freed (`free_slots` write) in a state after
the AXI data `read`, not the `readreq`.

## 7. Simulation pitfalls

- `make csim-krnl` can reuse stale objects; `rm -rf
  build/krnl/krnl_prj/solution1/csim` after header/constant changes.
- The `m_axi ... depth=` pragma sizes **cosim's** memory model. Too small
  and RTL simulation reads junk past the end while csim passes.
- The testbench pre-stages completion tokens, so cosim cannot catch a
  kernel that waits before sending; only the schedule grep or hardware can.
- A local csim without Vitis works with ~150 lines of shim headers for
  `ap_int.h`, `hls_stream.h`, `ap_axi_sdata.h`, `etc/ap_utils.h` and
  `g++ -DHLS`; good for fast iteration on `krnl/test`.
