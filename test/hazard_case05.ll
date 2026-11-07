; the same malloc call in a function NOT marked hot must produce no finding
; at all -- the pass is scoped to hot functions by design, not every
; allocation in the program.
; RUN: opt -load-pass-plugin="%hazard_plugin" -passes=hazard-analysis -disable-output "%s" | FileCheck "%s" --allow-empty
; CHECK-NOT: HAZARD

declare i8* @malloc(i64)

define void @cold_kernel() {
  %p = call i8* @malloc(i64 64)
  ret void
}
