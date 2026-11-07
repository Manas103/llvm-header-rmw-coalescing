; a hot function with three different hazards must produce three findings,
; one per call site, not a single function-level verdict.
; RUN: opt -load-pass-plugin="%hazard_plugin" -passes=hazard-analysis -disable-output "%s" | FileCheck "%s"
; CHECK: HAZARD|kernel|heap_alloc|malloc
; CHECK: HAZARD|kernel|locking|sem_wait
; CHECK: HAZARD|kernel|blocking_syscall|usleep

%struct.sem = type { i32 }
declare i8* @malloc(i64)
declare i32 @sem_wait(%struct.sem*)
declare i32 @usleep(i32)
@g_sem = global %struct.sem zeroinitializer

define void @kernel() #0 {
  %p = call i8* @malloc(i64 16)
  %r1 = call i32 @sem_wait(%struct.sem* @g_sem)
  %r2 = call i32 @usleep(i32 1000)
  ret void
}

attributes #0 = { hot }
