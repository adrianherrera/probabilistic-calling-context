//===-- pcc.h - Probabilistic Calling Context runtime ---------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file can be used to query the current calling context.
///
//===----------------------------------------------------------------------===//

#include <stdint.h>

/// Query the current calling context.
///
/// This is different to what is presented in Mike Bond's paper. In the
/// original paper, to check \p V at a given program point, the PCC calculate
/// function \p f was called using the value of \p cs for the current call site
/// (not necessary a call site), i.e., current function name and line number.
/// As we are not using a function name and line number hash for calculating
/// \p V, we cannot apply this technique. Instead we just return the current
/// value of \c V.
uintptr_t __pcc_query();
