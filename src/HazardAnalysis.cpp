//===- HazardAnalysis.cpp - flag latency hazards in hot functions -------===//
//
// A different kind of pass than RmwCoalesce.cpp above: an *analysis*, not a
// transform. For every function Clang marked `hot` (a real LLVM IR function
// attribute, set by the source-level `__attribute__((hot))`, not a project
// convention), this walks every instruction and reports four kinds of
// latency hazard:
//
//   - heap allocation:    a direct call to a name on a fixed allocator list
//   - locking:            a direct call to a name on a fixed lock-acquire list
//   - blocking syscall:   a direct call to a name on a fixed blocking-call list
//   - virtual dispatch:   any call whose callee is not a known Function,
//                         i.e. any indirect call (CallBase::isIndirectCall())
//
// The indirect-call check is a real, honest proxy for virtual dispatch, not
// a precise one: a C++ virtual call and a plain C function-pointer callback
// compile to the exact same IR shape (load a function pointer, call through
// it), and this pass cannot and does not try to tell them apart. That is
// disclosed in the README as this analysis's main source of false
// positives, and it is deliberately exercised by this project's own
// corpus (see tools/generate_hazard_corpus.py, the "clean callback"
// functions).
//
// Name-based detection has the opposite, symmetric limitation: a locking or
// syscall hazard invoked *through* a function pointer rather than by name is
// still flagged (as a virtual-dispatch hazard, correctly -- there really is
// an indirect call there) but not also credited to its true category, since
// name-based detection cannot see through the indirection to know which
// function is actually being called. Also disclosed, also deliberately
// exercised by the corpus's "redirected" cases.
//===----------------------------------------------------------------------===//
#include "llvm/IR/Function.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"

#include <array>
#include <string>

using namespace llvm;

namespace {

const std::array<const char*, 5> kHeapSyms = {"malloc", "calloc", "realloc",
                                               "aligned_alloc", "posix_memalign"};
const std::array<const char*, 6> kLockSyms = {
    "pthread_mutex_lock", "pthread_mutex_trylock", "pthread_rwlock_rdlock",
    "pthread_rwlock_wrlock", "pthread_spin_lock", "sem_wait"};
const std::array<const char*, 17> kSyscallSyms = {
    "read", "write", "recv", "send", "recvfrom", "sendto", "accept", "connect",
    "sleep", "usleep", "nanosleep", "poll", "select", "waitpid", "fopen",
    "fread", "fwrite"};

template <typename Arr>
bool contains(const Arr& arr, StringRef name) {
    for (const char* s : arr)
        if (name == s) return true;
    return false;
}

// Reports one finding to stdout in a simple, greppable, pipe-delimited
// format the Python benchmark harness parses: tools/run_hazard_benchmark.py
// is the only consumer, and keeping the format in one place (this comment
// plus that script) rather than inventing a schema avoids drift.
void report(StringRef func, StringRef category, StringRef callee, unsigned line) {
    outs() << "HAZARD|" << func << "|" << category << "|" << callee << "|" << line << "\n";
}

unsigned lineOf(const Instruction& I) {
    if (const DebugLoc& DL = I.getDebugLoc()) return DL.getLine();
    return 0;
}

bool runOnFunction(Function& F) {
    if (!F.hasFnAttribute(Attribute::Hot)) return false;
    bool Found = false;
    for (Instruction& I : instructions(F)) {
        auto* CB = dyn_cast<CallBase>(&I);
        if (!CB) continue;
        if (CB->isIndirectCall()) {
            report(F.getName(), "virtual_dispatch", "<indirect>", lineOf(I));
            Found = true;
            continue;
        }
        Function* Callee = CB->getCalledFunction();
        if (!Callee) continue;
        StringRef Name = Callee->getName();
        if (contains(kHeapSyms, Name)) {
            report(F.getName(), "heap_alloc", Name, lineOf(I));
            Found = true;
        } else if (contains(kLockSyms, Name)) {
            report(F.getName(), "locking", Name, lineOf(I));
            Found = true;
        } else if (contains(kSyscallSyms, Name)) {
            report(F.getName(), "blocking_syscall", Name, lineOf(I));
            Found = true;
        }
    }
    return Found;
}

struct HazardAnalysisPass : PassInfoMixin<HazardAnalysisPass> {
    PreservedAnalyses run(Function& F, FunctionAnalysisManager&) {
        runOnFunction(F);
        return PreservedAnalyses::all();  // analysis only, never mutates the IR
    }
};

}  // namespace

extern "C" ::llvm::PassPluginLibraryInfo LLVM_ATTRIBUTE_WEAK llvmGetPassPluginInfo() {
    return {
        LLVM_PLUGIN_API_VERSION, "HazardAnalysis", LLVM_VERSION_STRING,
        [](PassBuilder& PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager& FPM, ArrayRef<PassBuilder::PipelineElement>) {
                    if (Name == "hazard-analysis") {
                        FPM.addPass(HazardAnalysisPass());
                        return true;
                    }
                    return false;
                });
        }};
}
