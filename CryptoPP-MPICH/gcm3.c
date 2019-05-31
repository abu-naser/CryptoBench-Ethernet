#include <stdlib.h>
#include "/home/mpiuser/cryptopp/gcm3header.h"
#include <stdio.h>





int main( int argc, char* argv[] ) {

//void *buf = "Hello, MPI World To All Users For Every Jobs Which Can Be Run MPICH2 == Hello, MPI World To All Users For Every Jobs Which Can Be Run MPICH2!";
void *buf = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
void *ciphertext;
ciphertext=(char*) malloc((1000 * sizeof(char)) );

//cryptopp_init();
void *buf2;
buf2=(char*) malloc((1000 * sizeof(char)) );

//int count=141;
int count=130;
for(int i=0 ; i<10 ; i++){
	
	count=encrypt((unsigned char *)buf, ciphertext , count);
	printf("count 1= %d\n", count);
	count=decrypt((unsigned char *)ciphertext, buf2 ,count);
	printf("count 2= %d\ndecrypt=", count);	
			
	for (int i=0 ; i<count ; i++){
		printf("%c",*((char *)(buf2+i)));fflush(stdout);
		}
	printf("\n\n"); 
}
free(ciphertext);
free(buf2);

return 0;
}

