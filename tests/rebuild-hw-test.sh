#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."

platform=xilinx_u280_gen3x16_xdma_1_202211_1

full=$(scripts/rebuild-hw.sh --platform "$platform" --dry-run)
grep -F "+ make host TARGET=hw PLATFORM=$platform" <<<"$full" >/dev/null
grep -F "+ rm -rf _x.hw.$platform.server0 package.hw.server0" <<<"$full" >/dev/null
grep -F "+ rm -f build_dir.hw.$platform.server0/krnl.link.xclbin build_dir.hw.$platform.server0/krnl.xclbin" <<<"$full" >/dev/null
grep -F "+ make build TARGET=hw SERVER=0 PLATFORM=$platform" <<<"$full" >/dev/null
grep -F "+ rm -rf _x.hw.$platform.server1 package.hw.server1" <<<"$full" >/dev/null
grep -F "+ rm -f build_dir.hw.$platform.server1/krnl.link.xclbin build_dir.hw.$platform.server1/krnl.xclbin" <<<"$full" >/dev/null
grep -F "+ make build TARGET=hw SERVER=1 PLATFORM=$platform" <<<"$full" >/dev/null

server1=$(PLATFORM="$platform" scripts/rebuild-hw.sh --server 1 --no-host --dry-run)
if grep -F "make host" <<<"$server1" >/dev/null; then
	echo "unexpected host build for --no-host" >&2
	exit 1
fi
if grep -F "SERVER=0" <<<"$server1" >/dev/null; then
	echo "unexpected server0 build for --server 1" >&2
	exit 1
fi
grep -F "+ rm -rf _x.hw.$platform.server1 package.hw.server1" <<<"$server1" >/dev/null
grep -F "+ rm -f build_dir.hw.$platform.server1/krnl.link.xclbin build_dir.hw.$platform.server1/krnl.xclbin" <<<"$server1" >/dev/null
grep -F "+ make build TARGET=hw SERVER=1 PLATFORM=$platform" <<<"$server1" >/dev/null
