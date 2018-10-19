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
static const char *const PCCVName = "__pcc_V";

class ProbabilisticCallingContext : public ModulePass {
public:
  static char ID;
  ProbabilisticCallingContext() : ModulePass(ID) {}

  bool runOnModule(Module &M) override;
};

} // namespace

static Function *getPCCFunction(Constant *FuncOrBitcast) {
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

  IntegerType *IntTy = DL.getIntPtrType(C);

  Function *PCCFunc = getPCCFunction(
      M.getOrInsertFunction(PCCCalculateName, IntTy, IntTy, IntTy));
  GlobalVariable *PCCVar = new GlobalVariable(
      M, IntTy, false, GlobalValue::ExternalLinkage, nullptr, PCCVName, nullptr,
      GlobalVariable::GeneralDynamicTLSModel, 0, false);

  for (auto &F : M.functions()) {
    if (F.isDeclaration()) {
      continue;
    }

    BasicBlock::iterator IP = F.getEntryBlock().getFirstInsertionPt();
    IRBuilder<> EntryIRB(&*IP);
    LoadInst *Temp = EntryIRB.CreateLoad(PCCVar);

    for (auto I = inst_begin(F); I != inst_end(F); ++I) {
      if (auto *Call = dyn_cast<CallInst>(&*I)) {
        ConstantInt *CS = ConstantInt::get(IntTy, random());

        IRBuilder<> CallSiteIRB(Call);
        CallInst *CallF = CallSiteIRB.CreateCall(PCCFunc, {Temp, CS});
        CallSiteIRB.CreateStore(CallF, PCCVar);
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
