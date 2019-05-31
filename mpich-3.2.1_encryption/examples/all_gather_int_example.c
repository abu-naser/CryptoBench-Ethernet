#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <mpi.h>
#include <assert.h>



int main(int argc, char** argv) {


  MPI_Init(NULL, NULL);

  int world_rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
  int world_size;
  int local_nums[2] = {-1,-1};
  int local_array[12] = {-1, -1, -1, -1, -1, -1, -1};
  MPI_Comm_size(MPI_COMM_WORLD, &world_size);

  
  if (world_rank == 0)
    local_nums[0] = local_nums[1] = 0;
  else if (world_rank == 1)
    local_nums[0] = local_nums[1] = 1;
  else if (world_rank == 2)
    local_nums[0] = local_nums[1] = 2;
  else if (world_rank == 3)
    local_nums[0] = local_nums[1] = 3;

  printf("rank %d will send %d %d\n",world_rank, local_nums[0], local_nums[1]);fflush(stdout);

  
  MPI_Allgather(local_nums, 2, MPI_INT, local_array, 2, MPI_INT, MPI_COMM_WORLD); 

  printf("rank %d received %d, %d, %d, %d, %d, %d, %d, %d\n",world_rank, local_array[0], local_array[1], local_array[2],local_array[3], local_array[4], local_array[5], local_array[6],local_array[7]);
  fflush(stdout);


  MPI_Barrier(MPI_COMM_WORLD);
  MPI_Finalize();
}