#!/usr/bin/env bash
# Rebuild host_exe and hardware xclbins for the two-node CloudLab run.
set -euo pipefail

usage() {
	cat <<'USAGE'
Usage:
  scripts/rebuild-hw.sh --platform <platform> [options] [-- <make args...>]

Options:
  --platform <platform>  Vitis platform name or .xpfm path. Can also use PLATFORM.
  --server <0|1|all>    Build one server xclbin or both. Default: all.
  --no-host             Skip rebuilding host_exe.
  --no-force            Do not delete existing krnl.xclbin before make build.
  --dry-run             Print commands without running them.
  -h, --help            Show this help.

Examples:
  scripts/rebuild-hw.sh --platform xilinx_u280_gen3x16_xdma_1_202211_1
  PLATFORM=xilinx_u280_gen3x16_xdma_1_202211_1 scripts/rebuild-hw.sh
  scripts/rebuild-hw.sh --server 1 --no-host -- -j8
USAGE
}

platform=${PLATFORM:-${XILINX_PLATFORM:-}}
server=all
build_host=1
force_relink=1
dry_run=0
make_args=()

while [[ $# -gt 0 ]]; do
	case "$1" in
		--platform)
			[[ $# -ge 2 ]] || { echo "ERROR: --platform needs a value" >&2; exit 2; }
			platform=$2
			shift 2
			;;
		--server)
			[[ $# -ge 2 ]] || { echo "ERROR: --server needs 0, 1, or all" >&2; exit 2; }
			server=$2
			shift 2
			;;
		--no-host)
			build_host=0
			shift
			;;
		--no-force)
			force_relink=0
			shift
			;;
		--dry-run)
			dry_run=1
			shift
			;;
		-h|--help)
			usage
			exit 0
			;;
		--)
			shift
			make_args=("$@")
			break
			;;
		*)
			echo "ERROR: unknown option: $1" >&2
			usage >&2
			exit 2
			;;
	esac
done

if [[ -z "$platform" ]]; then
	echo "ERROR: provide --platform or set PLATFORM" >&2
	exit 2
fi

case "$server" in
	0|1) servers=("$server") ;;
	all) servers=(0 1) ;;
	*)
		echo "ERROR: --server must be 0, 1, or all" >&2
		exit 2
		;;
esac

platform_base=$(basename "$platform")
xsa=${platform_base%.xpfm}

run() {
	printf '+'
	for arg in "$@"; do
		printf ' %q' "$arg"
	done
	printf '\n'
	if [[ "$dry_run" -eq 0 ]]; then
		"$@"
	fi
}

if [[ "$build_host" -eq 1 ]]; then
	run make host TARGET=hw PLATFORM="$platform" "${make_args[@]}"
fi

for srv in "${servers[@]}"; do
	build_dir="build_dir.hw.$xsa.server$srv"
	if [[ "$force_relink" -eq 1 ]]; then
		run rm -f "$build_dir/krnl.xclbin"
	fi
	run make build TARGET=hw SERVER="$srv" PLATFORM="$platform" "${make_args[@]}"
done

if [[ "$dry_run" -eq 0 ]]; then
	echo "== Rebuild complete =="
	echo "host_exe"
	for srv in "${servers[@]}"; do
		echo "build_dir.hw.$xsa.server$srv/krnl.xclbin"
	done
fi
