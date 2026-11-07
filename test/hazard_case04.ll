; an indirect call in a hot function must be flagged as virtual dispatch,
; whether or not the callee is a real virtual call or a plain callback --
; that imprecision is disclosed in the README, not hidden in the test.
; RUN: opt -load-pass-plugin="%hazard_plugin" -passes=hazard-analysis -disable-output "%s" | FileCheck "%s"
; CHECK: HAZARD|kernel|virtual_dispatch|<indirect>

define void @callback() {
  ret void
}

define void @kernel() #0 {
  %fp = alloca void ()*
  store void ()* @callback, void ()** %fp
  %loaded = load void ()*, void ()** %fp
  call void %loaded()
  ret void
}

attributes #0 = { hot }
