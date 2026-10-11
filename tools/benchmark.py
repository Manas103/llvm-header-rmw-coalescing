#!/usr/bin/env python3
"""Reproduces the README's headline numbers: static instruction count and
a simulated per-packet cycle count, for the 18 kernels at clang -O2 versus
the same -O2 IR with this project's rmw-coalesce pass (plus a standard
instcombine/simplifycfg/dce cleanup) run on top.

Usage: python3 tools/benchmark.py <baseline.ll> <optimized.ll>
"""
import re
import sys

# A deliberately simple, disclosed cost model (the README calls these
# "simulated cycles", not measured hardware cycles): memory ops are the
# expensive part on a real pipeline target, calls are the most expensive,
# everything else is cheap integer/bitwise work.
WEIGHTS = {
    "load": 4,
    "store": 4,
    "call": 15,
    "br": 1,
    "ret": 1,
    "and": 1, "or": 1, "xor": 1,
    "shl": 1, "lshr": 1, "ashr": 1,
    "add": 1, "sub": 1,
    "getelementptr": 0,  # address computation, folded into the load/store on real hardware
    "bitcast": 0,
    "zext": 1, "sext": 1, "trunc": 1,
    "icmp": 1, "phi": 0,
}

FUNC_RE = re.compile(r"^define .*@([A-Za-z0-9_]+)\(")

def parse_functions(path):
    """Returns {func_name: [opcode, ...]} for every defined function."""
    funcs = {}
    current = None
    body = []
    with open(path) as f:
        for line in f:
            m = FUNC_RE.match(line)
            if m:
                current = m.group(1)
                body = []
                continue
            if current is None:
                continue
            stripped = line.strip()
            if stripped == "}":
                funcs[current] = body
                current = None
                continue
            if not stripped or stripped.endswith(":"):
                continue  # blank line or basic-block label
            # An instruction line looks like "%9 = and i8 %6, 3" or
            # "store i8 %8, i8* %5" or "tail call void @touch(...)" or "ret void".
            rhs = stripped.split(" = ", 1)[-1] if " = " in stripped else stripped
            opcode = rhs.split()[0]
            if opcode == "tail":
                opcode = "call"
            body.append(opcode)
    return funcs

def cost(ops):
    return sum(WEIGHTS.get(op, 1) for op in ops)

def main():
    baseline_path, optimized_path = sys.argv[1], sys.argv[2]
    baseline = parse_functions(baseline_path)
    optimized = parse_functions(optimized_path)

    kernel_names = sorted(n for n in baseline if n.startswith("k"))
    assert len(kernel_names) == 18, f"expected 18 kernels, found {len(kernel_names)}: {kernel_names}"

    total_base_instr = total_opt_instr = 0
    total_base_cycles = total_opt_cycles = 0
    print(f"{'kernel':<28}{'base_instr':>11}{'opt_instr':>11}{'base_cyc':>10}{'opt_cyc':>9}")
    for name in kernel_names:
        b_ops = baseline[name]
        o_ops = optimized.get(name, b_ops)
        b_i, o_i = len(b_ops), len(o_ops)
        b_c, o_c = cost(b_ops), cost(o_ops)
        total_base_instr += b_i
        total_opt_instr += o_i
        total_base_cycles += b_c
        total_opt_cycles += o_c
        print(f"{name:<28}{b_i:>11}{o_i:>11}{b_c:>10}{o_c:>9}")

    instr_pct = 100.0 * (total_base_instr - total_opt_instr) / total_base_instr
    cycle_pct = 100.0 * (total_base_cycles - total_opt_cycles) / total_base_cycles
    print()
    print(f"totals over {len(kernel_names)} kernels:")
    print(f"  static instructions: {total_base_instr} (-O2 baseline) -> {total_opt_instr} (+rmw-coalesce), "
          f"{instr_pct:.1f}% fewer")
    print(f"  simulated cycles/packet: {total_base_cycles} -> {total_opt_cycles}, {cycle_pct:.1f}% fewer")

if __name__ == "__main__":
    main()
