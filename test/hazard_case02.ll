; direct lock acquire in a hot function must be flagged
; RUN: opt -load-pass-plugin="%hazard_plugin" -passes=hazard-analysis -disable-output "%s" | FileCheck "%s"
; CHECK: HAZARD|kernel|locking|pthread_mutex_lock

%struct.mu = type { i32 }
declare i32 @pthread_mutex_lock(%struct.mu*)
@g_mu = global %struct.mu zeroinitializer

define void @kernel() #0 {
  %r = call i32 @pthread_mutex_lock(%struct.mu* @g_mu)
  ret void
}

attributes #0 = { hot }
