; direct blocking syscall in a hot function must be flagged
; RUN: opt -load-pass-plugin="%hazard_plugin" -passes=hazard-analysis -disable-output "%s" | FileCheck "%s"
; CHECK: HAZARD|kernel|blocking_syscall|read

declare i64 @read(i32, i8*, i64)

define void @kernel() #0 {
  %r = call i64 @read(i32 0, i8* null, i64 1)
  ret void
}

attributes #0 = { hot }
