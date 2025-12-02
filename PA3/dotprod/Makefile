# To test with a different size, specify the SIZE in the make command.
# For example, type "make SIZE=32768".

LIB=.
CFLAGS = -O3 -msse2 -mfpmath=sse -DSIZE=${SIZE}

SIZE := 65536

all:	reg ur4 ae4 sse

reg: test.c $(LIB)/reg_dotprod.c 
	gcc $(CFLAGS) -o reg $(LIB)/test.c $(LIB)/reg_dotprod.c

ur4: test.c ur4_dotprod.c
	gcc $(CFLAGS) -o ur4 $(LIB)/test.c $(LIB)/ur4_dotprod.c

ae4: test.c ae4_dotprod.c
	gcc $(CFLAGS) -o ae4 $(LIB)/test.c $(LIB)/ae4_dotprod.c

sse: test.c sse_dotprod.c
	gcc $(CFLAGS) -o sse $(LIB)/test.c $(LIB)/sse_dotprod.c

clean:
	rm -f reg ur4 ae4 sse
