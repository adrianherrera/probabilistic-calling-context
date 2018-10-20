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

uintptr_t __pcc_query() { return __pcc_V; }
