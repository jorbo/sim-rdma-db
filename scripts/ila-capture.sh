#!/usr/bin/env bash
# Capture ILA state (ila_stack_top counters/handshakes) from a running or
# hung two-node RDMA B-tree test.
#
# Runs locally on the machine hosting the card, or remotely over ssh.
#
# Usage:
#   scripts/ila-capture.sh --ltx <path/to/probes.ltx> [options]
#
# Options:
#   --ltx <file>      probes file from the hw link (required). Look in
#                     _x.hw.<xsa>.serverN/link/vivado/vpl/prj/prj.runs/impl_1/
#                     or build_dir.hw.<xsa>.serverN/ for *.ltx
#   --host <ssh-dest> run the capture on a remote node (copies ltx+tcl over,
#                     copies results back). Default: run locally.
#   --mode snapshot|trigger   snapshot (default): immediate capture — read
#                     the free-running pkg/drop counters after a hang.
#                     trigger: arm on --pattern, then run the workload.
#   --pattern <glob>  probe name glob for trigger mode, matched against the
#                     names dumped in *.probes.txt (e.g. '*tx_meta*valid*')
#   --out <prefix>    output file prefix (default: ila.<timestamp>)
#   --vivado <bin>    vivado_lab or vivado binary (default: autodetect)
#
# What to look for (see ila_stack_top in rocetest_krnl/src/hdl/stack_top.sv):
#   probe6/7   role tx_meta valid/ready — did krnl emit the RDMA command?
#   probe18/19 rx/tx pkg counters       — did packets leave / arrive?
#   probe11-16 mem read/write cmd       — did the request/response reach DMA?
#   crc/psn drop counters (ila_stack_top_inter) — silent drops on rx.
set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
TCL=$SCRIPT_DIR/ila-capture.tcl

ltx="" host="" mode=snapshot pattern="" out="ila.$(date +%Y%m%d-%H%M%S)" vivado=""

while [[ $# -gt 0 ]]; do
	case "$1" in
		--ltx)     ltx=$2; shift 2 ;;
		--host)    host=$2; shift 2 ;;
		--mode)    mode=$2; shift 2 ;;
		--pattern) pattern=$2; shift 2 ;;
		--out)     out=$2; shift 2 ;;
		--vivado)  vivado=$2; shift 2 ;;
		-h|--help) grep '^#' "$0" | sed 's/^# \{0,1\}//'; exit 0 ;;
		*) echo "ERROR: unknown option: $1" >&2; exit 2 ;;
	esac
done

[[ -n "$ltx" ]] || { echo "ERROR: --ltx is required" >&2; exit 2; }
[[ "$mode" == snapshot || "$mode" == trigger ]] || {
	echo "ERROR: --mode must be snapshot or trigger" >&2; exit 2; }
if [[ "$mode" == trigger && -z "$pattern" ]]; then
	echo "ERROR: trigger mode needs --pattern (run snapshot first; pick a name from *.probes.txt)" >&2
	exit 2
fi

find_vivado='
	for v in vivado_lab vivado; do
		command -v $v >/dev/null 2>&1 && { echo $v; exit 0; }
	done
	for p in /opt/Xilinx/Vivado_Lab/*/bin/vivado_lab /opt/Xilinx/Vivado/*/bin/vivado \
	         /tools/Xilinx/Vivado_Lab/*/bin/vivado_lab /tools/Xilinx/Vivado/*/bin/vivado; do
		[ -x "$p" ] && { echo "$p"; exit 0; }
	done
	exit 1
'

run_capture() {
	# $1=vivado $2=ltx $3=out — runs in the current directory.
	local viv=$1 l=$2 o=$3
	# hw_server must be up for the debug bridge to enumerate.
	pgrep -x hw_server >/dev/null 2>&1 || {
		echo "Starting hw_server..."
		hw_server -d >/dev/null 2>&1 || "$(dirname "$viv")/hw_server" -d >/dev/null 2>&1 || {
			echo "ERROR: could not start hw_server" >&2; return 1; }
		sleep 2
	}
	"$viv" -mode batch -nolog -nojournal -source ila-capture.tcl \
		-tclargs "$l" "$o" "$mode" "$pattern"
}

if [[ -z "$host" ]]; then
	[[ -f "$ltx" ]] || { echo "ERROR: missing $ltx" >&2; exit 1; }
	if [[ -z "$vivado" ]]; then
		vivado=$(bash -c "$find_vivado") || {
			echo "ERROR: no vivado_lab/vivado on PATH — pass --vivado" >&2; exit 1; }
	fi
	cp -f "$TCL" ./ila-capture.tcl 2>/dev/null || true
	run_capture "$vivado" "$ltx" "$out"
	echo "== Results: ${out}.* =="
else
	[[ -f "$ltx" ]] || { echo "ERROR: missing $ltx" >&2; exit 1; }
	rdir="btree-ila"
	echo "== Copying ltx + tcl to $host:$rdir =="
	ssh "$host" "mkdir -p $rdir"
	scp "$ltx" "$host:$rdir/probes.ltx"
	scp "$TCL" "$host:$rdir/ila-capture.tcl"
	echo "== Running capture on $host =="
	ssh "$host" "cd $rdir
		viv='$vivado'
		if [ -z \"\$viv\" ]; then viv=\$(bash -c '$find_vivado') || {
			echo 'ERROR: no vivado_lab/vivado found on $host' >&2; exit 1; }
		fi
		pgrep -x hw_server >/dev/null 2>&1 || {
			echo 'Starting hw_server...'
			hw_server -d >/dev/null 2>&1 || \"\$(dirname \"\$viv\")/hw_server\" -d >/dev/null 2>&1 || {
				echo 'ERROR: could not start hw_server' >&2; exit 1; }
			sleep 2
		}
		\"\$viv\" -mode batch -nolog -nojournal -source ila-capture.tcl \
			-tclargs probes.ltx '$out' '$mode' '$pattern'"
	echo "== Copying results back =="
	scp "$host:$rdir/${out}.*" .
	echo "== Results: ${out}.* =="
fi
