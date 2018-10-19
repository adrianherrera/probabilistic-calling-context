//===-- pcc-rt.cpp - Probabilistic Calling Context runtime ----------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the run time component used to calculate and maintain
/// the probabilsitic calling context (PCC).
///
//===----------------------------------------------------------------------===//

#include "pcc.h"

/// Unique value representing the current calling context.
__thread uintptr_t __pcc_V = 0;

/// Non-commutative but efficiently composable function to compute PCC values.
///
/// This corresponds to the function \p f in Mike Bond's paper.
///
/// \param V current calling context value
/// \param cs call site identifier
///
/// \returns the updated calling context value
uintptr_t __pcc_calculate(uintptr_t V, uintptr_t cs) { return 3 * V + cs; }

uintptr_t __pcc_get() { return __pcc_V; }
