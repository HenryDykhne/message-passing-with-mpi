#include <stdio.h>
#include <mpi.h>

int main(void) {
    int my_rank, comm_sz;
     
    /* Let the system do what it needs to start up MPI */
    MPI_Init(NULL, NULL);

    /* Get my process rank */
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

    /* Find out how many processes are being used */
    MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);

    /* Shut down MPI */
    MPI_Finalize();

   return 0;
}