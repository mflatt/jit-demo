This is an interpreter in C for a mini Scheme/Racket language that
support functions, numbers, `if0`, and a top-level table of
definitions. Continuations are explicit, and memory is managed with a
two-space copying collector.

There's no reader. Programs are constructed direct in AST form using a
C API, as demonstrated in "main.c".

Numbers are encoded either as record or using a fixnum encoding where
the low bit is set to indicate a number instead of a pointer.
Configure with `FIXNUM_ENCODING` in "struct.h".
