#include <stdio.h>
#include <mpi.h>

int main(int argc, char *argv[])
{
    int errs = 0;
    int wrank, wsize;
    

    MPI_Init(NULL, NULL);
    MPI_Comm_rank(MPI_COMM_WORLD, &wrank);
    MPI_Comm_size(MPI_COMM_WORLD, &wsize);
    printf("app wrank =%d\n",wrank);
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Finalize();
return 0;
}