#!/usr/bin/env python3
"""LLM triage over HazardAnalysis's raw findings.

Takes docs/hazard_findings_raw.json (the pass's findings, nothing else --
no ground truth, no manifest) and asks a real `claude -p` call to rank them
into a smaller "review now" set. This is a genuine subprocess call to the
Claude CLI, not a canned response: the measured review-volume reduction and
the no-invented-detections check below are both computed from whatever the
model actually returns on this run.

Usage: run_llm_triage.py <findings.json> <out_prefix>
Writes <out_prefix>_output.txt (the raw model transcript) and
<out_prefix>_result.json (parsed verdict + computed metrics).
"""
import json
import re
import subprocess
import sys

PROMPT_TEMPLATE = """You are triaging static-analysis findings from a latency-hazard pass. Each finding is one call site inside a function the compiler marked hot (performance-critical). The four categories are: heap_alloc (malloc/calloc/realloc/etc), locking (pthread mutex/rwlock/spinlock acquire), blocking_syscall (read/write/recv/send/sleep/poll/etc), virtual_dispatch (any indirect call -- this category also fires on ordinary function-pointer callbacks that are not actually virtual calls, so some virtual_dispatch findings are harmless callbacks, not true virtual calls).

Triage these {total} findings the way you would for a real production review queue: select the subset most worth a reviewer's limited time, in your own judgment about how large that subset should be. There is no fixed quota.

Findings (JSON array of {{id, func, category, callee}}):
{findings_json}

Respond with ONLY a JSON object, no other text, no markdown fence:
{{"review_now": [list of integer ids you select], "rationale": "one or two sentences"}}
"""


def main():
    findings_path = sys.argv[1] if len(sys.argv) > 1 else "docs/hazard_findings_raw.json"
    out_prefix = sys.argv[2] if len(sys.argv) > 2 else "docs/llm_triage"

    with open(findings_path) as f:
        findings = json.load(f)
    valid_ids = {item["id"] for item in findings}

    prompt = PROMPT_TEMPLATE.format(total=len(findings), findings_json=json.dumps(findings))

    result = subprocess.run(
        ["claude", "-p", prompt],
        capture_output=True,
        text=True,
        timeout=180,
    )
    transcript = result.stdout
    with open(out_prefix + "_output.txt", "w", newline="\n", encoding="utf-8") as f:
        f.write(transcript)
        if result.stderr:
            f.write("\n--- stderr ---\n")
            f.write(result.stderr)

    match = re.search(r"\{.*\}", transcript, re.DOTALL)
    if not match:
        verdict = {"error": "no JSON object found in model output", "raw": transcript}
    else:
        try:
            verdict = json.loads(match.group(0))
        except json.JSONDecodeError as e:
            verdict = {"error": f"JSON parse failure: {e}", "raw": match.group(0)}

    review_now = verdict.get("review_now", []) if isinstance(verdict, dict) else []
    invented = [i for i in review_now if i not in valid_ids]
    review_now_valid = [i for i in review_now if i in valid_ids]

    total = len(findings)
    review_count = len(review_now_valid)
    reduction = (total / review_count) if review_count else float("inf")

    out = {
        "total_findings": total,
        "review_now_count": review_count,
        "review_now_ids": review_now_valid,
        "invented_ids_not_in_input": invented,
        "review_volume_reduction_x": reduction,
        "rationale": verdict.get("rationale") if isinstance(verdict, dict) else None,
        "parse_error": verdict.get("error") if isinstance(verdict, dict) else None,
    }
    with open(out_prefix + "_result.json", "w", newline="\n") as f:
        json.dump(out, f, indent=2)

    print(json.dumps(out, indent=2))


if __name__ == "__main__":
    main()
