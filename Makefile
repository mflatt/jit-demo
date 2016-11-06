
OBJS = main.o lookup.o hash.o struct.o eval.o compile.o jit.o print.o fail.o continue.o gc.o
HEADERS = lookup.h hash.h fail.h struct.h continue.h jit.h gc.h compile.h
CFLAGS = -O2 -g -Wall

mr : $(OBJS)
	$(CC) -o mr $(OBJS) $(LDFLAGS) $(LIBS)

main.o : $(HEADERS)
eval.o : $(HEADERS)
compile.o : $(HEADERS)
jit.o : $(HEADERS)
print.o : $(HEADERS)
lookup.o : $(HEADERS)
struct.o : struct.h gc.h
fail.o : fail.h
hash.o : hash.h
gc.o : $(HEADERS)

clean:
	rm -f *.o mr
