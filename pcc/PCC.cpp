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
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

using namespace llvm;

#define DEBUG_TYPE "pcc"

namespace {

static const char *const PCCCalculateName = "__pcc_calculate";
static const char *const PCCVarName = "__pcc_V";

/// ProbabilisticCallingContext: instrument the code in a module to maintain a
/// probabilistic unique value representing the current calling context.
class ProbabilisticCallingContext : public ModulePass {
public:
  static char ID;
  ProbabilisticCallingContext() : ModulePass(ID) {}

  bool runOnModule(Module &M) override;
};

} // namespace

// Adapted from `llvm::checkSanitizerInterfaceFunction`
static Function *checkPCCInterfaceFunction(Constant *FuncOrBitcast) {
  if (isa<Function>(FuncOrBitcast)) {
    return cast<Function>(FuncOrBitcast);
  }

  FuncOrBitcast->print(errs());
  errs() << '\n';
  std::string Err;
  raw_string_ostream Stream(Err);
  Stream << PCCCalculateName << " function redefined: " << *FuncOrBitcast;
  report_fatal_error(Err);
}

char ProbabilisticCallingContext::ID = 0;

bool ProbabilisticCallingContext::runOnModule(Module &M) {
  LLVMContext &C = M.getContext();
  const DataLayout &DL = M.getDataLayout();

  // Either 32 or 64 bit depending on the target
  IntegerType *IntTy = DL.getIntPtrType(C);

  // PCCCalculate is the function `__pcc_calculate` that (funny enough)
  // calculates the probabilistic calling context
  Function *PCCCalculate = checkPCCInterfaceFunction(
      M.getOrInsertFunction(PCCCalculateName, IntTy, IntTy, IntTy));

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
    LoadInst *Temp = EntryIRB.CreateLoad(PCCVar);

    for (auto I = inst_begin(F); I != inst_end(F); ++I) {
      if (auto *Call = dyn_cast<CallInst>(&*I)) {
        // (2) at each call site, compute the next calling context and update
        // the global variable `V`
        //
        // Note that a hash of the method name and line number are used for
        // `cs` in Mike Bond's original paper. Since this is C/C++ and not
        // Java, we just assign a random value to `cs` instead :)
        ConstantInt *CS = ConstantInt::get(IntTy, random());

        IRBuilder<> CallSiteIRB(Call);
        Value *Mul = CallSiteIRB.CreateMul(ConstantInt::get(IntTy, 3), Temp);
        Value *Add = CallSiteIRB.CreateAdd(Mul, CS);
        CallSiteIRB.CreateStore(Add, PCCVar);
      } else if (auto *Return = dyn_cast<ReturnInst>(&*I)) {
        // (3) at function return, store the local copy back into the global
        // variable `V` (this redundancy is helpful for correctly maintaining
        // `V` in the face of exception control flow)
        IRBuilder<> ReturnIRB(Return);
        ReturnIRB.CreateStore(Temp, PCCVar);
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
