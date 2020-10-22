
LIB = -L/usr/lib/x86_64-linux-gnu/openmpi/lib
INCLUDE = -I/usr/lib/x86_64-linux-gnu/openmpi/include

all: mpish

mpish:
	gcc mpish.c -o mpish -lmpi   $(LIB) $(INCLUDE)

clean:
	rm mpish