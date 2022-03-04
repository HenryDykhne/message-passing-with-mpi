# message-passing-with-mpi

Everything should work as expected
dont try to give commands longer than 4096 chars.
dont try to run it with more than 10^99 processes. (I'm sure it wont be an issue.)
dont try to run it with less than 2 processes.

build code with: `make`
run code with: `mpirun -np 4 ./mpish` for 4 processes.
if that does not work, use: `mpirun -oversubscribe -np 4 ./mpish`
