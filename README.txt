This is an interpreter in C for a mini Scheme/Racket language that
support functions, numbers, `if0`, and a top-level table of
definitions. Continuations are explicit, and memory is managed with a
two-space copying collector.

There's no reader. Programs are constructed direct in AST form using a
C API, as demonstrated in "main.c".

Numbers are encoded either as record or using a fixnum encoding where
the low bit is set to indicate a number instead of a pointer.
Configure with `FIXNUM_ENCODING` in "struct.h".

The evaluator supports an optional compilation pass that converts a
local-variable reference to a De Bruijn index and substitutes a
top-level reference with its value (which creates a cyclic structure
for a recursive function). This compiler is already a kind of
just-in-time compiler, because it relies on having values for
top-level definitions.

A jit-in-time compiler to machine code is supported when `USE_JIT` is
defined to 1 and when linked with GNU lightning. To build in that mode,
build with something like

 make CPPFLAGS="-DUSE_JIT=1" LIBS="-llightning"

JIT compilation to native code works only on functions that don't have
variables (i.e., where variables have been compiled to De Bruijn
indices or replaced with global values).
