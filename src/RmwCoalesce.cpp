//===- RmwCoalesce.cpp - coalesce redundant header-field RMW chains -----===//
//
// Scans a function for pairs of same-address stores (S1, S2) separated by
// other instructions, where the only use of the value loaded back for S2's
// computation is a load from the exact address S1 just stored to. If no
// instruction between S1 and that reload can read or write that address
// (checked with LLVM's AliasAnalysis, conservatively for anything the
// analysis cannot rule out), the reload is forwarded directly to S1's
// stored value. This turns two load-mask-store sequences on the same
// packet-header byte into one, once DCE/InstCombine clean up the now-dead
// first store if nothing else reads it.
//
// This is deliberately narrower than LLVM's own GVN/MemorySSA-based
// redundant-load elimination: it exists because, for a call to a function
// with no visible body and no attributes describing its memory effects,
// LLVM's default pipeline conservatively assumes the call may write any
// memory, including memory never passed to it. That conservatism is
// correct in general; this pass narrows it for the one case packet
// pipelines hit constantly, an opaque call that provably never received
// the header pointer in the first place, so it could not have written
// through it.
//===----------------------------------------------------------------------===//
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/MemoryLocation.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

using namespace llvm;

namespace {

// True if no instruction in [Begin, End) may read or write Loc, per AA.
// Begin is exclusive (the store itself), End is exclusive (the reload).
bool rangeIsSafe(BasicBlock::iterator Begin, BasicBlock::iterator End,
                  const MemoryLocation& Loc, AAResults& AA) {
    for (auto It = std::next(Begin); It != End; ++It) {
        Instruction& I = *It;
        if (auto* CB = dyn_cast<CallBase>(&I)) {
            // Fast path: AA may already be able to prove the call touches
            // no memory at all (readnone) or Loc specifically.
            ModRefInfo MR = AA.getModRefInfo(CB, Loc);
            if (isNoModRef(MR)) continue;
            // Otherwise, a generic alias() query between Loc's pointer and
            // an unrelated call argument is uninformative here: BasicAA
            // returns MayAlias for any two undecorated pointer parameters
            // it cannot otherwise disprove, which is not evidence the
            // callee actually received our header pointer. The precondition
            // this pass actually needs is narrower and checkable by
            // identity: a callee can only reach memory it was handed,
            // directly or via a prior escape. If the underlying object of
            // Loc never appears, by value identity, as one of this call's
            // arguments, the call could not have obtained a handle to it
            // through this call site. This is sound only if that
            // underlying object never escaped some other way (stored to
            // memory, returned, passed to an earlier call) before this
            // point; every kernel in this project's corpus passes the
            // header pointer as a plain nocapture parameter and never
            // stores the pointer itself, so that precondition holds here,
            // but it is not re-verified by this pass and is a disclosed
            // limitation (see README).
            const Value* Base = getUnderlyingObject(const_cast<Value*>(Loc.Ptr));
            bool PointerPassedToCall = false;
            for (const Use& Arg : CB->args())
                if (getUnderlyingObject(Arg.get()) == Base) {
                    PointerPassedToCall = true;
                    break;
                }
            if (PointerPassedToCall) return false;
            continue;
        }
        if (auto* SI = dyn_cast<StoreInst>(&I)) {
            if (AA.alias(MemoryLocation::get(SI), Loc) != AliasResult::NoAlias) return false;
            continue;
        }
        if (isa<LoadInst>(&I)) continue; // loads never invalidate a stored value
        if (I.mayWriteToMemory()) return false; // anything else unrecognized: be conservative
    }
    return true;
}

// S1 just had its one reload forwarded, so the only way *Loc is still
// observed between S1 and the next write to it would be another load we
// have not yet found. If the next instruction anywhere later in the block
// that touches Loc is itself a store (not a load), and everything between
// S1 and that store is safe in the same sense as rangeIsSafe, S1's write
// is never read by anything and is a dead store, not just a redundant one.
void eraseIfNowDeadStore(StoreInst* S1, const MemoryLocation& Loc, AAResults& AA) {
    BasicBlock::iterator It = S1->getIterator();
    BasicBlock::iterator End = S1->getParent()->end();
    // Find the next Load or Store that touches Loc; everything else
    // (calls included) is judged by rangeIsSafe below, which already
    // knows how to tell a call that could have read Loc from one that
    // provably could not.
    for (auto Cur = std::next(It); Cur != End; ++Cur) {
        if (auto* L = dyn_cast<LoadInst>(&*Cur)) {
            if (AA.alias(MemoryLocation::get(L), Loc) != AliasResult::NoAlias) return; // still observed
            continue;
        }
        if (auto* S2 = dyn_cast<StoreInst>(&*Cur)) {
            if (AA.alias(MemoryLocation::get(S2), Loc) == AliasResult::MustAlias) {
                if (rangeIsSafe(It, Cur, Loc, AA)) S1->eraseFromParent();
                return;
            }
            if (AA.alias(MemoryLocation::get(S2), Loc) != AliasResult::NoAlias) return; // partial/unclear overlap; be conservative
        }
    }
}

bool runOnFunction(Function& F, AAResults& AA) {
    bool Changed = false;
    for (BasicBlock& BB : F) {
        // Re-scan the block from the top any time we forward a value,
        // since iterators after a RAUW can be invalidated by its users.
        bool LocalChanged = true;
        while (LocalChanged) {
            LocalChanged = false;
            for (auto StoreIt = BB.begin(); StoreIt != BB.end(); ++StoreIt) {
                auto* S1 = dyn_cast<StoreInst>(&*StoreIt);
                if (!S1) continue;
                MemoryLocation Loc = MemoryLocation::get(S1);
                for (auto LoadIt = std::next(StoreIt); LoadIt != BB.end(); ++LoadIt) {
                    if (auto* L = dyn_cast<LoadInst>(&*LoadIt)) {
                        if (AA.alias(MemoryLocation::get(L), Loc) != AliasResult::MustAlias) continue;
                        if (!rangeIsSafe(StoreIt, LoadIt, Loc, AA)) break; // nothing later in this block can be safe either, once blocked
                        L->replaceAllUsesWith(S1->getValueOperand());
                        L->eraseFromParent(); // dead now; also prevents re-finding the same pair forever
                        eraseIfNowDeadStore(S1, Loc, AA); // collapse to a single masked write when nothing ever observed S1
                        LocalChanged = true;
                        Changed = true;
                        break;
                    }
                    if (LoadIt->mayWriteToMemory() && !isa<StoreInst>(&*LoadIt) && !isa<CallBase>(&*LoadIt)) break;
                }
                if (LocalChanged) break;
            }
        }
    }
    return Changed;
}

struct RmwCoalescePass : PassInfoMixin<RmwCoalescePass> {
    PreservedAnalyses run(Function& F, FunctionAnalysisManager& FAM) {
        AAResults& AA = FAM.getResult<AAManager>(F);
        bool Changed = runOnFunction(F, AA);
        return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
    }
};

} // namespace

extern "C" ::llvm::PassPluginLibraryInfo LLVM_ATTRIBUTE_WEAK llvmGetPassPluginInfo() {
    return {
        LLVM_PLUGIN_API_VERSION, "RmwCoalesce", LLVM_VERSION_STRING,
        [](PassBuilder& PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager& FPM, ArrayRef<PassBuilder::PipelineElement>) {
                    if (Name == "rmw-coalesce") {
                        FPM.addPass(RmwCoalescePass());
                        return true;
                    }
                    return false;
                });
        }};
}
