; call marked readnone: AliasAnalysis proves no-modref directly, the fast path
; RUN: opt -load-pass-plugin="%llvm_pass_plugin" -passes=rmw-coalesce -S "%s" | FileCheck "%s"
; CHECK: load i8, i8* %p
; CHECK-NOT: load i8, i8* %p

%struct.hdr = type { [32 x i8] }

declare void @opaque(i8*)
declare void @opaque_readnone(i8*) #0
attributes #0 = { readnone }

define void @kernel(%struct.hdr* %h, i8* %ctx, i8 %v1, i8 %v2) {
  %p = getelementptr inbounds %struct.hdr, %struct.hdr* %h, i64 0, i32 0, i64 0
  %l1 = load i8, i8* %p
  %m1 = and i8 %l1, 252
  %c1 = or i8 %m1, %v1
  store i8 %c1, i8* %p
  call void @opaque_readnone(i8* %ctx)
  %l2 = load i8, i8* %p
  %m2 = and i8 %l2, 3
  %c2 = or i8 %m2, %v2
  store i8 %c2, i8* %p
  ret void
}
