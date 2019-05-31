#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#define FLOAT_PRECISION 2
#define FIELD_WIDTH 20
#define sz 4194304+17
#define rcv_sz 4194304*2*2
#define MG 1048576.00
#define MG_4 4194304
#define MG_2 2097152
#define MG_1 1048576
#define K_8 8192
#define K_1 1024


int choping_size [100] = {472, 500, 484, 512, 
                        972, 1000, 996, 1024, 
                        1464, 1492, 1472, 1500,  
                        1972, 2000, 2020, 2048,
                        3972, 4000, 4068, 4096, 
                        7972, 8000, 8164, 8192};

int choping_option = 10;

int main(int argc, char** argv) {

  MPI_Init(NULL, NULL);
  int world_rank,world_size,j,datasz,iteration, i, data_slice;
  char * sendbuf, *recvbuf;
  double t_start, t_end, t;

  MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
  MPI_Comm_size(MPI_COMM_WORLD, &world_size);

  sendbuf = (char *)malloc(sz * sizeof(char));
  recvbuf = (char *)malloc(rcv_sz * sizeof(char));

  memset(sendbuf,'a',4194304);
  memset(recvbuf,'b',4194304);
  
   
  

  if(world_rank == 0){
   printf("Base: without encryption\n");   
   printf("\n# Size 		 Bandwidth (MB/s)\n");
   fflush(stdout);
  }

  /*#################################  Encrypted Messages #################################*/
  init_crypto();

  

        

  if(world_rank == 0){ 
    printf("\n /*#################################  Encrypted Allgather Test #################################*/\n");
  }
  
    iteration = 2;
  
    //for(i=0; i<24; i++){
        
       

        


        for(datasz=K_1; datasz<=MG_4; datasz*=2){

            

	        if(datasz > K_8)
                iteration = 2;

            for(j=1;j<=iteration+20;j++){
                if(j == 20){
                    t_start = MPI_Wtime();
                }
                /*
                if (world_rank == 0) {
                    MPI_SEC_Choping_Send(sendbuf, datasz, MPI_CHAR, 1, 0, MPI_COMM_WORLD);
                    MPI_SEC_Choping_Recv(recvbuf, datasz, MPI_CHAR, 1, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);    
                }
                else if(world_rank == 1){
                    MPI_SEC_Choping_Recv(recvbuf, datasz, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    MPI_SEC_Choping_Send(sendbuf, datasz, MPI_CHAR, 0, 1, MPI_COMM_WORLD);  
                
                }
                */
                MPI_SEC_Allgather(sendbuf, datasz, MPI_CHAR, recvbuf,  datasz, MPI_CHAR, MPI_COMM_WORLD);
            }
            t_end = MPI_Wtime();
            if(world_rank == 0) {

            t = t_end - t_start;
            double tmp = (datasz * 2)/ MG * iteration;
            fprintf(stdout, "%-*d%*.*f\n", 10, datasz, FIELD_WIDTH,
                        FLOAT_PRECISION, tmp / t);                   
            fflush(stdout);
            }
        }

   // }

  free(sendbuf);
  free(recvbuf);

  MPI_Finalize();
}
