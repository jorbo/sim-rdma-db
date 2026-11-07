#!/usr/bin/env bash
# Throughput-vs-clients sweep for the two-node RDMA B-tree (paper Exp. 1
# style: closed-loop clients, uniform point lookups, all fetches remote).
#
# Runs scripts/deploy-run.sh once per client count with BENCH_SEARCHES and
# SKIP_COPY=1, keeps each head.log and the head's profile summary.csv, and
# prints kernel-only throughput = searches / krnl_1 Total Time.
#
# Run from the repo root on the build machine, after one normal
# deploy-run.sh (SKIP_COPY=0) has put the current build on both nodes.
# xrt.ini must enable the profile (opencl_summary + device_trace) or
# summary.csv has no krnl_1 row.
#
# Usage:
#   scripts/sweep.sh <table-user@host> <head-user@host> [nodes.cfg]
#
# Environment:
#   CLIENTS         client counts to sweep (default: "1 2 4 8 12 16")
#   BENCH_SEARCHES  lookups per run (default: 25600 = 100 kernel launches)
#   BENCH_SEED      key seed (default: 1)
#   REMOTE_DIR      remote working directory (default: btree-run)
#   OUT             output directory (default: btree_logs/sweep-<timestamp>)
set -uo pipefail

TABLE=${1:?usage: sweep.sh <table-user@host> <head-user@host> [nodes.cfg]}
HEAD=${2:?usage: sweep.sh <table-user@host> <head-user@host> [nodes.cfg]}
CFG=${3:-nodes.cfg}
CLIENTS=${CLIENTS:-"1 2 4 8 12 16"}
BENCH_SEARCHES=${BENCH_SEARCHES:-25600}
BENCH_SEED=${BENCH_SEED:-1}
REMOTE_DIR=${REMOTE_DIR:-btree-run}
OUT=${OUT:-btree_logs/sweep-$(date +%Y%m%d-%H%M%S)}
SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)

mkdir -p "$OUT"
echo "== Sweep: clients [$CLIENTS], $BENCH_SEARCHES searches each -> $OUT =="

for c in $CLIENTS; do
	echo "=== clients=$c ==="
	# Stale summary.csv from an earlier run must not be mistaken for this one.
	ssh "$HEAD" "rm -f $REMOTE_DIR/summary.csv" || true
	SKIP_COPY=1 LAUNCH_PROBE=0 SEARCH_CLIENTS=$c \
		BENCH_SEARCHES=$BENCH_SEARCHES BENCH_SEED=$BENCH_SEED \
		REMOTE_DIR=$REMOTE_DIR \
		"$SCRIPT_DIR/deploy-run.sh" "$TABLE" "$HEAD" "$CFG" \
		|| echo "run failed for clients=$c (see $OUT/head.c$c.log)"
	cp -f head.log "$OUT/head.c$c.log" 2>/dev/null || true
	scp -q "$HEAD:$REMOTE_DIR/summary.csv" "$OUT/summary.c$c.csv" 2>/dev/null \
		|| echo "no summary.csv for clients=$c (profiling off in xrt.ini?)"
done

# krnl_1 row of the "Compute Unit Utilization" table: field 6 = number of
# calls, field 10 = total time (ms).
echo
printf "%-8s %-9s %-7s %-8s %-14s %-14s %s\n" \
	clients searches calls cu_ms lookups/s_cu lookups/s_wall result
for c in $CLIENTS; do
	log=$OUT/head.c$c.log sum=$OUT/summary.c$c.csv
	row=$(grep -m1 '^[^,]*,krnl_1,krnl,' "$sum" 2>/dev/null)
	calls=$(echo "$row" | awk -F, '{print $6}')
	cu_ms=$(echo "$row" | awk -F, '{print $10}')
	rate_cu=$(awk -v n="$BENCH_SEARCHES" -v t="$cu_ms" \
		'BEGIN { if (t > 0) printf "%.0f", n / (t / 1000); else print "-" }')
	rate_wall=$(grep -o 'lookups_per_s_wall=[0-9]*' "$log" 2>/dev/null | cut -d= -f2)
	result=$(grep -o 'TEST [A-Z]*' "$log" 2>/dev/null | tail -1)
	printf "%-8s %-9s %-7s %-8s %-14s %-14s %s\n" \
		"$c" "$BENCH_SEARCHES" "${calls:--}" "${cu_ms:--}" "$rate_cu" \
		"${rate_wall:--}" "${result:-no log}"
done | tee "$OUT/results.txt"
