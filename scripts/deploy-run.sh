#!/usr/bin/env bash
# Deploy and run the two-node RDMA B-tree test on OCT machines.
#
# Run from the repo root on the build machine after:
#   make host TARGET=hw PLATFORM=$PLATFORM
#   make build TARGET=hw SERVER=0 PLATFORM=$PLATFORM
#   make build TARGET=hw SERVER=1 PLATFORM=$PLATFORM
#
# Usage:
#   scripts/deploy-run.sh <table-user@host> <head-user@host> [nodes.cfg]
#
# Environment:
#   REMOTE_DIR     remote working directory (default: btree-run)
#   RDMA_SELFTEST  1 (default) fires the host-driven RDMA READ on the
#                  head node before the search workload; set 0 to skip
#
# The host binary is copied, not rebuilt: both machines must run the
# same XRT version as the build machine.
#
# Start order does not matter: the table node retries its bootstrap
# connect until the head is listening, and the head blocks in accept
# until the table's tree is built.
set -euo pipefail

TABLE=${1:?usage: deploy-run.sh <table-user@host> <head-user@host> [nodes.cfg]}
HEAD=${2:?usage: deploy-run.sh <table-user@host> <head-user@host> [nodes.cfg]}
CFG=${3:-nodes.cfg}
REMOTE_DIR=${REMOTE_DIR:-btree-run}
SELFTEST=${RDMA_SELFTEST:-1}
# Non-interactive ssh does not load the XRT environment; source it
# explicitly in every remote run command.
XRT_SETUP=${XRT_SETUP:-/opt/xilinx/xrt/setup.sh}

XSA=xilinx_u280_gen3x16_xdma_1_202211_1
BD0=build_dir.hw.$XSA.server0/krnl.xclbin
BD1=build_dir.hw.$XSA.server1/krnl.xclbin
CFG_BASE=$(basename "$CFG")

for f in host_exe "$BD0" "$BD1" "$CFG"; do
	[ -f "$f" ] || { echo "ERROR: missing $f" >&2; exit 1; }
done
# Head node needs a third fpga_ip column to program the RoCE stack.
if ! awk '!/^#/ && NF > 0 && NF < 3 {exit 1}' "$CFG"; then
	echo "ERROR: $CFG needs 'node_id host_ip fpga_ip' lines for hardware runs" >&2
	exit 1
fi

echo "== Copying artifacts =="
for h in "$TABLE" "$HEAD"; do
	ssh "$h" "mkdir -p $REMOTE_DIR && [ -f $XRT_SETUP ]" || {
		echo "ERROR: $XRT_SETUP not found on $h — is XRT installed?" >&2
		exit 1
	}
done
scp host_exe "$CFG" "$TABLE:$REMOTE_DIR/"
scp "$BD0" "$TABLE:$REMOTE_DIR/krnl.server0.xclbin"
scp host_exe "$CFG" "$HEAD:$REMOTE_DIR/"
scp "$BD1" "$HEAD:$REMOTE_DIR/krnl.server1.xclbin"

echo "== Starting table node on $TABLE (background) =="
TABLE_PID=$(ssh "$TABLE" \
	". $XRT_SETUP > /dev/null && cd $REMOTE_DIR && \
	 nohup ./host_exe krnl.server0.xclbin 0 $CFG_BASE \
	 > table.log 2>&1 & echo \$!")
echo "table node pid $TABLE_PID; log: $REMOTE_DIR/table.log"

cleanup() {
	echo "== Stopping table node =="
	ssh "$TABLE" "kill $TABLE_PID 2>/dev/null || true"
	echo "== Table node log =="
	ssh "$TABLE" "cat $REMOTE_DIR/table.log" || true
}
trap cleanup EXIT

echo "== Running head node on $HEAD (RDMA_SELFTEST=$SELFTEST) =="
set +e
ssh "$HEAD" \
	". $XRT_SETUP > /dev/null && cd $REMOTE_DIR && \
	 RDMA_SELFTEST=$SELFTEST ./host_exe \
	 krnl.server1.xclbin 1 $CFG_BASE" | tee head.log
RC=${PIPESTATUS[0]}
set -e

echo "== Head node exit code: $RC =="
exit "$RC"
