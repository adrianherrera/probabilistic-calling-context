#include "llvm/IR/InstIterator.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"

using namespace llvm;

#define DEBUG_TYPE "pcc"

namespace {

class ProbabilisticCallingContext : public ModulePass {
public:
    static char ID;
    ProbabilisticCallingContext() : ModulePass(ID) {}

    bool runOnModule(Module &M) override;
};

}

static Function *createF(Module &M) {
    LLVMContext &C = M.getContext();
    const DataLayout &DL =  M.getDataLayout();
    
    IntegerType *IntTy = DL.getIntPtrType(C);
    Type *Args[] = {
        IntTy,
        IntTy,
    };

    FunctionType *FnTy = FunctionType::get(IntTy, Args, false);
    Function *F = Function::Create(FnTy, GlobalValue::InternalLinkage, "pcc_f", &M);
    F->addFnAttr(Attribute::NoInline);
    BasicBlock::Create(C, "entry", F);

    ConstantInt *Three = ConstantInt::get(IntTy, 3);

    IRBuilder<> IRB(&F->getEntryBlock());
    
}

static Value *computePCC(CallInst *Call, Value *V) {
    auto *IntTy = dyn_cast<IntegerType>(V->getType());
    assert(IntTy);

    ConstantInt *Three = ConstantInt::get(IntTy, 3);

    IRBuilder<> IRB(Call);
    Value *Mul = IRB.CreateMul(Three, V);
}

char ProbabilisticCallingContext::ID = 0;

bool ProbabilisticCallingContext::runOnModule(Module &M) {
    LLVMContext &C = M.getContext();
    const DataLayout &DL = M.getDataLayout();

    IntegerType *IntTy = DL.getIntPtrType(C);
    ConstantInt *InitV = ConstantInt::get(IntTy, 0);
    GlobalVariable *V = new GlobalVariable(
            M, IntTy, false, GlobalValue::InternalLinkage, InitV, "V");

    for (auto &F : M.functions()) {
        IRBuilder<> IRB(&F.getEntryBlock());

        LoadInst *Temp = IRB.CreateLoad(V);

        for (auto I = inst_begin(F); I != inst_end(F); ++I) {
            if (auto *Call = dyn_cast<CallInst>(&*I)) {
            }
        }
    }

    return true;
}

static RegisterPass<ProbabilisticCallingContext> X(
        "pcc", "Probabilistic calling context pass", false, false);
