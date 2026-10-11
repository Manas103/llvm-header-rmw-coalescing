#!/usr/bin/env python3
"""Generates the 40 lit/FileCheck tests under test/. Deterministic; re-run
with: python3 tools/generate_tests.py
"""
import os

OUT_DIR = os.path.join(os.path.dirname(__file__), "..", "test")

HEADER = """%struct.hdr = type {{ [32 x i8] }}

declare void @opaque(i8*)
declare void @opaque_readnone(i8*) #0
attributes #0 = {{ readnone }}
"""

def gep(byte):
    return f"getelementptr inbounds %struct.hdr, %struct.hdr* %h, i64 0, i32 0, i64 {byte}"

# Each generator returns (body_text, is_coalesce_expected, description)

def case_call_guarded(byte, mask1, val1, mask2, val2):
    body = f"""define void @kernel(%struct.hdr* %h, i8* %ctx, i8 %v1, i8 %v2) {{
  %p = {gep(byte)}
  %l1 = load i8, i8* %p
  %m1 = and i8 %l1, {mask1}
  %c1 = or i8 %m1, %v1
  store i8 %c1, i8* %p
  call void @opaque(i8* %ctx)
  %l2 = load i8, i8* %p
  %m2 = and i8 %l2, {mask2}
  %c2 = or i8 %m2, %v2
  store i8 %c2, i8* %p
  ret void
}}
"""
    return body, True, "call to an opaque function that never receives %h: second load must be forwarded from %c1"

def case_call_receives_pointer(byte, mask1, val1, mask2, val2):
    body = f"""define void @kernel(%struct.hdr* %h, i8 %v1, i8 %v2) {{
  %p = {gep(byte)}
  %raw = bitcast %struct.hdr* %h to i8*
  %l1 = load i8, i8* %p
  %m1 = and i8 %l1, {mask1}
  %c1 = or i8 %m1, %v1
  store i8 %c1, i8* %p
  call void @opaque(i8* %raw)
  %l2 = load i8, i8* %p
  %m2 = and i8 %l2, {mask2}
  %c2 = or i8 %m2, %v2
  store i8 %c2, i8* %p
  ret void
}}
"""
    return body, False, "call receives a bitcast of %h itself: the pass must not forward across it"

def case_readnone_call(byte, mask1, val1, mask2, val2):
    body = f"""define void @kernel(%struct.hdr* %h, i8* %ctx, i8 %v1, i8 %v2) {{
  %p = {gep(byte)}
  %l1 = load i8, i8* %p
  %m1 = and i8 %l1, {mask1}
  %c1 = or i8 %m1, %v1
  store i8 %c1, i8* %p
  call void @opaque_readnone(i8* %ctx)
  %l2 = load i8, i8* %p
  %m2 = and i8 %l2, {mask2}
  %c2 = or i8 %m2, %v2
  store i8 %c2, i8* %p
  ret void
}}
"""
    return body, True, "call marked readnone: AliasAnalysis proves no-modref directly, the fast path"

def case_aliasing_store(byte, other_byte, mask1, val1, mask2, val2):
    body = f"""define void @kernel(%struct.hdr* %h, %struct.hdr* %maybe_alias, i8 %v1, i8 %v2) {{
  %p = {gep(byte)}
  %op = getelementptr inbounds %struct.hdr, %struct.hdr* %maybe_alias, i64 0, i32 0, i64 {other_byte}
  %l1 = load i8, i8* %p
  %m1 = and i8 %l1, {mask1}
  %c1 = or i8 %m1, %v1
  store i8 %c1, i8* %p
  store i8 7, i8* %op
  %l2 = load i8, i8* %p
  %m2 = and i8 %l2, {mask2}
  %c2 = or i8 %m2, %v2
  store i8 %c2, i8* %p
  ret void
}}
"""
    return body, False, "store through an unrelated %struct.hdr* that may alias %h: must not forward"

def case_cross_block(byte, mask1, val1, mask2, val2):
    body = f"""define void @kernel(%struct.hdr* %h, i8* %ctx, i8 %v1, i8 %v2, i1 %cond) {{
entry:
  %p = {gep(byte)}
  %l1 = load i8, i8* %p
  %m1 = and i8 %l1, {mask1}
  %c1 = or i8 %m1, %v1
  store i8 %c1, i8* %p
  br i1 %cond, label %next, label %next

next:
  call void @opaque(i8* %ctx)
  %l2 = load i8, i8* %p
  %m2 = and i8 %l2, {mask2}
  %c2 = or i8 %m2, %v2
  store i8 %c2, i8* %p
  ret void
}}
"""
    return body, False, "store and reload are in different basic blocks: this pass only coalesces within one block"

def case_three_segment(byte, mask1, val1, mask2, val2, mask3, val3):
    body = f"""define void @kernel(%struct.hdr* %h, i8* %ctx, i8 %v1, i8 %v2, i8 %v3) {{
  %p = {gep(byte)}
  %l1 = load i8, i8* %p
  %m1 = and i8 %l1, {mask1}
  %c1 = or i8 %m1, %v1
  store i8 %c1, i8* %p
  call void @opaque(i8* %ctx)
  %l2 = load i8, i8* %p
  %m2 = and i8 %l2, {mask2}
  %c2 = or i8 %m2, %v2
  store i8 %c2, i8* %p
  call void @opaque(i8* %ctx)
  %l3 = load i8, i8* %p
  %m3 = and i8 %l3, {mask3}
  %c3 = or i8 %m3, %v3
  store i8 %c3, i8* %p
  ret void
}}
"""
    return body, True, "a 3-segment chain across two opaque calls: both reloads must be forwarded"

def write_test(idx, body, coalesce_expected, description):
    path = os.path.join(OUT_DIR, f"case{idx:02d}.ll")
    check_lines = []
    if coalesce_expected:
        # After our pass runs (no cleanup here, to keep the check precise
        # and independent of InstCombine's own behavior), %l2 (and %l3, if
        # present) must have been replaced: no remaining load reads %p a
        # second (or third) time.
        check_lines.append("; CHECK: load i8, i8* %p")
        check_lines.append("; CHECK-NOT: load i8, i8* %p")
    else:
        check_lines.append("; CHECK: load i8, i8* %p")
        check_lines.append("; CHECK: load i8, i8* %p")
    content = (
        f"; {description}\n"
        f"; RUN: opt -load-pass-plugin=\"%llvm_pass_plugin\" -passes=rmw-coalesce -S \"%s\" | FileCheck \"%s\"\n"
        + "\n".join(check_lines) + "\n\n"
        + HEADER.format() + "\n" + body
    )
    with open(path, "w", newline="\n") as f:
        f.write(content)

def main():
    os.makedirs(OUT_DIR, exist_ok=True)
    idx = 1
    # 20 call-guarded coalescable cases, varying the byte offset for variety.
    for i in range(20):
        byte = i % 8
        body, ok, desc = case_call_guarded(byte, "252", "v1", "3", "v2")
        write_test(idx, body, ok, desc); idx += 1

    # 5 cases where the call receives our pointer: must NOT coalesce.
    for i in range(5):
        byte = i % 4
        body, ok, desc = case_call_receives_pointer(byte, "252", "v1", "3", "v2")
        write_test(idx, body, ok, desc); idx += 1

    # 3 readnone-call fast-path cases: must coalesce.
    for i in range(3):
        byte = i % 4
        body, ok, desc = case_readnone_call(byte, "252", "v1", "3", "v2")
        write_test(idx, body, ok, desc); idx += 1

    # 7 aliasing-store cases: must NOT coalesce.
    for i in range(7):
        byte = i % 4
        other_byte = (i + 5) % 8
        body, ok, desc = case_aliasing_store(byte, other_byte, "252", "v1", "3", "v2")
        write_test(idx, body, ok, desc); idx += 1

    # 2 cross-block cases: must NOT coalesce (scope limitation, disclosed in README).
    for i in range(2):
        byte = i % 4
        body, ok, desc = case_cross_block(byte, "252", "v1", "3", "v2")
        write_test(idx, body, ok, desc); idx += 1

    # 3 three-segment chains: both reloads must be forwarded.
    for i in range(3):
        byte = i % 4
        body, ok, desc = case_three_segment(byte, "240", "v1", "15", "v2", "255", "v3")
        write_test(idx, body, ok, desc); idx += 1

    assert idx - 1 == 40, idx - 1
    print(f"wrote {idx - 1} test cases to {OUT_DIR}")

if __name__ == "__main__":
    main()
