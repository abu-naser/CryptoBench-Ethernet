#include "/home/mpiuser/cryptopp/gcm3header.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#define sz 1048576*2*2+1

int main( int argc, char* argv[] ) {


	char buf[sz];

	for(int j=0;j<sz;j++)
		buf[j]='a';

	int var;
	struct timeval  tv1, tv2;
		
	void *ciphertext;
	//char *ciphertext;
	ciphertext=(char*) malloc((sz * sizeof(char)) );

	void *buf2;
	//char *buf2;

	buf2=(char*) malloc((sz * sizeof(char)) );

	int count;
	
	printf ("\nStart : 1 K\n\n");
	
	for(int i=1024; i<=sz; i*=2){
	
		count=i;
		
		 gettimeofday(&tv1, NULL);
		 
		 for(int j=0; j<100; j++){
		
			count=encrypt(buf, ciphertext , i);

		
			decrypt(ciphertext, buf2 ,count);
			
		}
		
		gettimeofday(&tv2, NULL);
		
	//	printf ("%llu %f\n",i, (double) (tv2.tv_usec - tv1.tv_usec)  +   (double) (tv2.tv_sec - tv1.tv_sec) * 1000000); 
		printf ("%f\n", (double) ((tv2.tv_usec - tv1.tv_usec)  +   (double) (tv2.tv_sec - tv1.tv_sec) * 1000000)/100); 
	}
	
	printf ("\nEnd : 4 M\n\n");
	
	free(ciphertext);
	free(buf2);

	return 0;
}
