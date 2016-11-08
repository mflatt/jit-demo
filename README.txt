This is an interpreter in C for a mini Scheme/Racket language that
support functions, numbers, `if0`, and a top-level table of
definitions --- and a just-in-time (JIT) compiler to machine code that
specializes frequently called closures.

There's no reader. Programs are constructed direct in AST form using a
C API, as demonstrated in "test.c".

Continuations are explicit. A standard eval--continue interpreter is
implemented in "eval.c".

Memory is managed with a two-space copying collector. The test suite
allocates an AST before the GC is enabled so that all the test
variables don't have to be registered as GC roots.

Numbers are encoded either as record or using a fixnum encoding where
the low bit is set to indicate a number instead of a pointer.
Configure with `FIXNUM_ENCODING` in "struct.h". The fixnum encoding is
required for native-code JIT mode.

The evaluator supports an optional compilation pass that converts a
local-variable reference to a De Bruijn index and substitutes a
top-level reference with its value (which creates a cyclic structure
for a recursive function). This compiler is already a kind of
just-in-time compiler, because it relies on having values for
top-level definitions.

A JIT compiler to machine code is supported when `USE_JIT` is defined
to 1 and when linked with GNU lightning. To build in that mode, build
with something like

 make CPPFLAGS="-DUSE_JIT=1" LIBS="-llightning"

The JIT specializes closures by recompiling the closure body (taking
into accunt actual values in the closure) after a closure is called
`SPECIALIZE_AFTER_COUNT` times. This specialization pass inlines
simple functions. Setting `SPECIALIZE_AFTER_COUNT` to -1 effectively
disables specialization.

Naturally, the "test.c" examples include variants of `fib`. Change
"main.c" to run test_Y_fib() or test_alt_fib() to try variants.
Specialization does little for Y-combinator recursion, but it
eliminates the abstraction overhead for the "alt" variant.
