
LIB = -L/usr/lib/x86_64-linux-gnu/openmpi/lib
INCLUDE = -I/usr/lib/x86_64-linux-gnu/openmpi/include
CFLAGS = -std=c11

all: mpish

mpish: mpish.c
	mpicc $(CFLAGS) mpish.c -o mpish -lmpi $(LIB) $(INCLUDE) 

clean:
	rm mpish

	