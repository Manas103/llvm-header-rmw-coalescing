; a lock acquire invoked through a function pointer (not by name) is
; flagged as virtual_dispatch -- the hazard is real, but name-based
; category detection cannot see through the indirection to credit it as
; "locking" specifically. This is the documented limitation the corpus's
; "redirected" cases exercise; this test pins the exact behavior.
; RUN: opt -load-pass-plugin="%hazard_plugin" -passes=hazard-analysis -disable-output "%s" | FileCheck "%s"
; CHECK: HAZARD|kernel|virtual_dispatch|<indirect>
; CHECK-NOT: HAZARD|kernel|locking

%struct.mu = type { i32 }
declare i32 @pthread_mutex_lock(%struct.mu*)
@g_mu = global %struct.mu zeroinitializer

define void @kernel() #0 {
  %fp = alloca i32 (%struct.mu*)*
  store i32 (%struct.mu*)* @pthread_mutex_lock, i32 (%struct.mu*)** %fp
  %loaded = load i32 (%struct.mu*)*, i32 (%struct.mu*)** %fp
  %r = call i32 %loaded(%struct.mu* @g_mu)
  ret void
}

attributes #0 = { hot }
