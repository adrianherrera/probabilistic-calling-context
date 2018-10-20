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
uintptr_t __pcc_query();
