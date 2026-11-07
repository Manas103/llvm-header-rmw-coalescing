; direct heap allocation in a hot function must be flagged
; RUN: opt -load-pass-plugin="%hazard_plugin" -passes=hazard-analysis -disable-output "%s" | FileCheck "%s"
; CHECK: HAZARD|kernel|heap_alloc|malloc

declare i8* @malloc(i64)

define void @kernel() #0 {
  %p = call i8* @malloc(i64 64)
  ret void
}

attributes #0 = { hot }
