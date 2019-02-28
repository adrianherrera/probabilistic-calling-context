# Probabilistic Calling Context

An implementation of Mike Bond's "probabilistic calling context" (PCC) in LLVM,
as described in his OOPSLA 2007 paper
[Probabilistic Calling Context](http://web.cse.ohio-state.edu/~bond.213/pcc-oopsla-2007.pdf).

## Background

A *dynamic calling context* is the sequence of active function invocations that
lead to a particular program location. This could be described exactly using
a stack backtrace, however walking the stack can be expensive (particularly if
the calling context is queried regularly). To counter this cost, Mike Bond
proposed a *probabilistic* calling context (PCC) that continuously maintains a
value that represents the current calling context with very low overhead. The
following function is used to compute and maintain the PCC:

```
f(V, cs) := 3 * V + cs
```

Where `V` is the current calling context value and `cs` is an identifier for
the call site of a function.

### Differences to the original paper

In the original PCC paper, `cs` was statically computed by hashing the function
name and line number. This works well for Java programs (for which PCC was
originally implemented for), however within the LLVM framework this approach is
more difficult to implement (although still doable). Instead, we assign `cs` a
random number (computed at compile time). This random number serves as an
identifier for each call site. This approach is inspired by the instrumentation
used in the American Fuzzy Lop (AFL) fuzzer.

## Usage

To build:

```console
mkdir build
cd build
cmake ..
make
```

This will produce both `pcc-llvm.so` and `libpcc-rt.a`.

* `Transform/pcc-llvm.so`: The LLVM pass; and
* `Runtime/libpcc-rt.a`: A static library that is linked with your application
and gives you the ability to query the calling context via `__pcc_query`
(remember to include `pcc.h`).
