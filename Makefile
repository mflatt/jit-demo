
OBJS = main.o lookup.o hash.o struct.o eval.o print.o fail.o continue.o gc.o
HEADERS = lookup.h hash.h fail.h struct.h continue.h gc.h
CFLAGS = -Wall

mr : $(OBJS)
	$(CC) -o mr $(OBJS)

main.o : $(HEADERS)
eval.o : $(HEADERS)
print.o : $(HEADERS)
lookup.o : $(HEADERS)
struct.o : struct.h gc.h
fail.o : fail.h
hash.o : hash.h
gc.o : $(HEADERS)

clean:
	rm -f *.o mr
