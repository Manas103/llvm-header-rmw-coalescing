#!/usr/bin/env bash
# Reproduces every number in the "Latency-hazard analysis extension" section
# of the README: the pass's own 240/250 + precision, the clang-tidy
# baseline, and (optionally, since it calls the real `claude` CLI) the LLM
# triage step.
set -euo pipefail
cd "$(dirname "$0")/.."

PLUGIN="${HAZARD_PLUGIN_PATH:-build/libHazardAnalysis.so}"
OPT_BIN="${OPT_BIN:-opt-14}"
CLANG_BIN="${CLANG_BIN:-clang-14}"
CLANG_TIDY_BIN="${CLANG_TIDY_BIN:-clang-tidy-14}"

python3 tools/generate_hazard_corpus.py kernels/hazard_corpus.c docs/hazard_manifest.json

# -disable-O0-optnone: plain -O0 marks every function `optnone`, and LLVM's
# generic FunctionPassManager::run() skips optnone functions for *any*
# function pass, analysis passes included -- not just transforms. Without
# this flag the hazard pass (and even a builtin pass like mem2reg) silently
# visits nothing. See the README Findings section for how this was found.
"$CLANG_BIN" -O0 -Xclang -disable-O0-optnone -S -emit-llvm kernels/hazard_corpus.c -o build_test/hazard_corpus.ll

"$OPT_BIN" -load-pass-plugin="$PLUGIN" -passes="hazard-analysis" -disable-output build_test/hazard_corpus.ll \
  > docs/hazard_findings_raw.txt
python3 - docs/hazard_findings_raw.txt docs/hazard_findings_raw.json << 'EOF'
import json, sys
findings = []
with open(sys.argv[1]) as f:
    for i, line in enumerate(f):
        line = line.strip()
        if not line: continue
        _, func, category, callee, lineno = line.split("|", 4)
        findings.append({"id": i, "func": func, "category": category, "callee": callee})
json.dump(findings, open(sys.argv[2], "w", newline="\n"), indent=2)
print(len(findings), "findings")
EOF

python3 tools/run_hazard_benchmark.py docs/hazard_manifest.json docs/hazard_findings_raw.txt

cp kernels/hazard_corpus.c build_test/hazard_corpus.cpp
"$CLANG_TIDY_BIN" build_test/hazard_corpus.cpp \
  -checks="-*,cppcoreguidelines-no-malloc,android-cloexec-accept,android-cloexec-accept4,android-cloexec-open,android-cloexec-fopen,android-cloexec-socket,android-cloexec-pipe,android-cloexec-pipe2,concurrency-mt-unsafe" \
  -- -std=c++17 > docs/clang_tidy_baseline_output.txt 2>&1 || true

echo "--- clang-tidy vs same manifest ---"
python3 - << 'EOF'
import json, re
manifest = json.load(open("docs/hazard_manifest.json"))
seeded_names = {m["name"] for m in manifest if m["kind"] == "seeded"}
flagged = set()
with open("docs/clang_tidy_baseline_output.txt") as f:
    for line in f:
        m = re.search(r"\b(hot_\w+|clean_\w+)\b", line)
        if m:
            flagged.add(m.group(1))
caught = flagged & seeded_names
result = {"clang_tidy_flagged_total": len(flagged), "clang_tidy_caught_seeded": len(caught), "total_seeded": len(seeded_names)}
print(result)
json.dump(result, open("docs/clang_tidy_scoring_result.json", "w", newline="\n"), indent=2)
EOF

echo "To also run the LLM triage step (calls the real claude CLI, costs real tokens):"
echo "  python3 tools/run_llm_triage.py docs/hazard_findings_raw.json docs/llm_triage"
