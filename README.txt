This is an interpreter in C for a mini Scheme/Racket language that
support functions, numbers, `if0`, and a top-level table of
definitions. Continuations are explicit, and memory is managed with a
two-space copying collector.

There's no reader. Programs are constructed direct in AST form using a
C API, as demonstrated in "main.c".
