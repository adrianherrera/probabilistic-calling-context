#include "pcc.h"

__thread uintptr_t __pcc_V = 0;

uintptr_t __pcc_calculate(uintptr_t V, uintptr_t cs) {
    return 3 * V + cs;
}

uintptr_t __pcc_get() {
    return __pcc_V;
}
