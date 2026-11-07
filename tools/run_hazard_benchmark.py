#!/usr/bin/env python3
"""Scores HazardAnalysis's findings against docs/hazard_manifest.json.

Usage: run_hazard_benchmark.py <manifest.json> <findings.txt>

`findings.txt` is the raw `HAZARD|func|category|callee|line` output of
`opt -passes=hazard-analysis -disable-output`, one line per call site
flagged. Scoring:

  - caught/total (the "240 of 250" claim): for every manifest entry of
    kind "seeded", a hit if some finding on that function's name has
    `category == true_category`. The 10 "redirected" seeded hazards (see
    the generator's module docstring) are seeded locking/syscall hazards
    invoked through a function pointer; the pass correctly flags them as
    virtual_dispatch (category mismatch against their true category), so
    they score as misses here -- a real limitation of name-based
    detection, not a scoring artifact.
  - false positives (for precision): any finding on a function of kind
    "clean_fp" or "clean". clean_fp functions make one benign callback
    each and are expected to trip the indirect-call check; clean
    functions make no suspicious call at all and should produce nothing.
  - precision = caught / (caught + false_positives).
"""
import json
import sys
from collections import defaultdict


def main():
    manifest_path, findings_path = sys.argv[1], sys.argv[2]
    with open(manifest_path) as f:
        manifest = json.load(f)
    by_name = {m["name"]: m for m in manifest}

    findings_by_func = defaultdict(list)
    with open(findings_path) as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            _, func, category, callee, lineno = line.split("|", 4)
            findings_by_func[func].append(category)

    seeded = [m for m in manifest if m["kind"] == "seeded"]
    caught = 0
    missed = []
    for m in seeded:
        cats = findings_by_func.get(m["name"], [])
        if m["true_category"] in cats:
            caught += 1
        else:
            missed.append(m["name"])

    false_positives = 0
    fp_functions = []
    for m in manifest:
        if m["kind"] in ("clean_fp", "clean"):
            if findings_by_func.get(m["name"]):
                false_positives += len(findings_by_func[m["name"]])
                fp_functions.append(m["name"])

    total = len(seeded)
    precision = caught / (caught + false_positives) if (caught + false_positives) else 0.0

    print(f"seeded hazards: {total}")
    print(f"caught: {caught} of {total}")
    print(f"missed: {len(missed)} -> {missed}")
    print(f"false positives: {false_positives} -> {fp_functions}")
    print(f"precision: {precision:.4f}  (= {caught} / ({caught} + {false_positives}))")

    result = {
        "total_seeded": total,
        "caught": caught,
        "missed_names": missed,
        "false_positives": false_positives,
        "fp_functions": fp_functions,
        "precision": precision,
    }
    with open("docs/hazard_scoring_result.json", "w", newline="\n") as f:
        json.dump(result, f, indent=2)


if __name__ == "__main__":
    main()
