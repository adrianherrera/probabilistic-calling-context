//===-- PCC.cpp - Probabilistic Calling Context ---------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains an implementation of Michael Bond's "probabilistic
/// calling context" (PCC) for maintaining runtime calling context information.
///
//===----------------------------------------------------------------------===//

#include <stdint.h>
#include <stdlib.h>

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InlineAsm.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

using namespace llvm;

#define DEBUG_TYPE "pcc"

namespace {

static const char *const PCCVarName = "__pcc_V";

static cl::opt<bool> ClUseCallSitePC(
    "pcc-use-call-site-pc",
    cl::desc("Use the program counter for the call site identifier"),
    cl::init(false));

/// ProbabilisticCallingContext: instrument the code in a module to maintain a
/// probabilistic unique value representing the current calling context.
class ProbabilisticCallingContext : public ModulePass {
public:
  static char ID;
  ProbabilisticCallingContext() : ModulePass(ID) {}

  bool runOnModule(Module &M) override;
};

} // end anonymous namespace

char ProbabilisticCallingContext::ID = 0;

bool ProbabilisticCallingContext::runOnModule(Module &M) {
  LLVMContext &C = M.getContext();
  const DataLayout &DL = M.getDataLayout();

  // Either 32 or 64 bit depending on the target
  IntegerType *IntTy = DL.getIntPtrType(C);

  // Decide on whether to use the program counter or a random integer as the
  // call site identifier
  InlineAsm *ReadPC = nullptr;
  if (ClUseCallSitePC) {
    ReadPC = InlineAsm::get(
        FunctionType::get(IntegerType::getInt64Ty(C), /* isVarArg */ false),
        "leaq (%rip), $0", /* Constraints */ "=r", /* hasSideEffects */ false);
  }

  // PCCVar is the variable `__pcc_V` that stores the probabilistic calling
  // context
  GlobalVariable *PCCVar = new GlobalVariable(
      M, IntTy, false, GlobalValue::ExternalLinkage, nullptr, PCCVarName,
      nullptr, GlobalVariable::GeneralDynamicTLSModel, 0, false);

  for (auto &F : M.functions()) {
    // We can only instrument functions that we have an implementation of
    if (F.isDeclaration()) {
      continue;
    }

    // (1) at the beginning of each function, load the the current PCC value
    // into the local variable `temp`
    BasicBlock::iterator IP = F.getEntryBlock().getFirstInsertionPt();
    IRBuilder<> EntryIRB(&*IP);
    auto *Temp = EntryIRB.CreateLoad(PCCVar);

    for (auto It = inst_begin(F); It != inst_end(F); ++It) {
      Instruction *I = &*I;
      if (isa<CallInst>(I) || isa<InvokeInst>(I)) {
        // (2) at each call site, compute the next calling context and update
        // the global variable `V`
        //
        // Note that a hash of the method name and line number are used for
        // `cs` in Mike Bond's original paper. Since this is C/C++ and not
        // Java, we either use the call site runtime address or assign a random
        // value to `cs` instead :)
        IRBuilder<> IRB(I);

        Value *CS = nullptr;
        if (ReadPC) {
          CS = IRB.CreateZExtOrTrunc(IRB.CreateCall(ReadPC), IntTy);
        } else {
          CS = ConstantInt::get(IntTy, random());
        }

        assert(CS);
        auto *Mul = IRB.CreateMul(ConstantInt::get(IntTy, 3), Temp);
        auto *Add = IRB.CreateAdd(Mul, CS);
        IRB.CreateStore(Add, PCCVar);
      } else if (isa<ReturnInst>(I)) {
        // (3) at function return, store the local copy back into the global
        // variable `V` (this redundancy is helpful for correctly maintaining
        // `V` in the face of exception control flow)
        IRBuilder<> IRB(I);
        IRB.CreateStore(Temp, PCCVar);
      }
    }
  }

  return true;
}

static RegisterPass<ProbabilisticCallingContext>
    X("pcc", "Probabilistic calling context pass", false, false);

static void registerPCCPass(const PassManagerBuilder &,
                            legacy::PassManagerBase &PM) {
  PM.add(new ProbabilisticCallingContext());
}

static RegisterStandardPasses
    RegisterPCCPass(PassManagerBuilder::EP_OptimizerLast, registerPCCPass);

static RegisterStandardPasses
    RegisterPCCPass0(PassManagerBuilder::EP_EnabledOnOptLevel0,
                     registerPCCPass);
