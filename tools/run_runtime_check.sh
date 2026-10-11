#!/usr/bin/env bash
# Builds kernels.c twice (once plain, once renamed k_* -> opt_k_* and run
# through this project's pass plus a standard cleanup pipeline), links both
# into test/runtime_check.c, and runs the resulting differential check.
# Usage: tools/run_runtime_check.sh <path-to-libRmwCoalesce.so>
set -euo pipefail
cd "$(dirname "$0")/.."
PLUGIN="$1"
OUT=build_artifacts
mkdir -p "$OUT"

clang-14 -O2 -S -emit-llvm kernels/kernels.c -o "$OUT/baseline.ll"
sed -E 's/@(k[A-Za-z0-9_]*)\(/@opt_\1(/' "$OUT/baseline.ll" > "$OUT/baseline_renamed.ll"
opt-14 -load-pass-plugin="$PLUGIN" -passes='rmw-coalesce,instcombine,simplifycfg,dce' -S "$OUT/baseline_renamed.ll" -o "$OUT/optimized_renamed.ll"

clang-14 -O2 -c "$OUT/baseline.ll" -o "$OUT/baseline.o"
clang-14 -O2 -c "$OUT/optimized_renamed.ll" -o "$OUT/optimized.o"
clang-14 -O2 test/runtime_check.c "$OUT/baseline.o" "$OUT/optimized.o" -o "$OUT/runtime_check"
"$OUT/runtime_check"
