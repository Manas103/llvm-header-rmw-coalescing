; a direct call to an unrecognized function in a hot function must produce
; no finding -- the pass has a fixed symbol list, not a general "any call
; is suspicious" rule.
; RUN: opt -load-pass-plugin="%hazard_plugin" -passes=hazard-analysis -disable-output "%s" | FileCheck "%s" --allow-empty
; CHECK-NOT: HAZARD

declare i32 @helper_add(i32, i32)

define void @kernel() #0 {
  %r = call i32 @helper_add(i32 1, i32 2)
  ret void
}

attributes #0 = { hot }
