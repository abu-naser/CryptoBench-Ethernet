/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
/* added by abu naser */
//#include <sodium.h> // added by abu naser
//#include "/home/mpiuser/boringssl/include/openssl/evp.h"
//#include "/home/mpiuser/boringssl/include/openssl/aes.h"
//#include "/home/mpiuser/boringssl/include/openssl/err.h"
//#include <openssl/evp.h>
//#include <openssl/aes.h>
//#include <openssl/err.h>
//#include <openssl/aead.h>

unsigned char ciphertext[4194304+18];
EVP_AEAD_CTX *ctx = NULL;
unsigned char key [32] = {'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z','a','b','c','d','e','f'};
unsigned char nonce[12] = {'1','2','3','4','5','6','7','8','9','0','1','2'};  
int nonceCounter; 
int keysize = 16;

 //char nonce[16];
 //char ADDITIONAL_DATA[10];
  //int ADDITIONAL_DATA_LEN=6;
  //
  //char ADDITIONAL_DATA[6] = {'1','2','3','4','5','6'};
//unsigned int ADDITIONAL_DATA_LEN;
//unsigned char nonce[12];
//unsigned char ADDITIONAL_DATA[6];
/* end of add */

  /*************************************************************************/
 /*                            Pre-CTR Mode                               */
/*    pre_start: Enc start counter; pre_end: pre-calculation end counter */
/*************************************************************************/                                                               
int pre_start[1024], pre_end[1024], first_flag[1024],iv_counter[1024],sloop_flag[1024],rloop_flag[1024],amount;
unsigned char send_ciphertext[419+400], p[8020], IV[1024][16], enc_iv[1024][16], pre_calculator[2][66];
EVP_CIPHER_CTX *ctx_enc;



/* -- Begin Profiling Symbol Block for routine MPI_Send */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Send = PMPI_Send
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Send  MPI_Send
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Send as PMPI_Send
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Send(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
             MPI_Comm comm) __attribute__((weak,alias("PMPI_Send")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Send
#define MPI_Send PMPI_Send

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Send

/*@
    MPI_Send - Performs a blocking send

Input Parameters:
+ buf - initial address of send buffer (choice) 
. count - number of elements in send buffer (nonnegative integer) 
. datatype - datatype of each send buffer element (handle) 
. dest - rank of destination (integer) 
. tag - message tag (integer) 
- comm - communicator (handle) 

Notes:
This routine may block until the message is received by the destination 
process.

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_COMM
.N MPI_ERR_COUNT
.N MPI_ERR_TYPE
.N MPI_ERR_TAG
.N MPI_ERR_RANK

.seealso: MPI_Isend, MPI_Bsend
@*/
int MPI_Send(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
	     MPI_Comm comm)
{
    static const char FCNAME[] = "MPI_Send";
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPID_Request * request_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_SEND);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPID_MPI_PT2PT_FUNC_ENTER_FRONT(MPID_STATE_MPI_SEND);
    
    /* Validate handle parameters needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_COMM(comm, mpi_errno);
	}
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */
    
    /* Convert MPI object handles to object pointers */
    MPID_Comm_get_ptr( comm, comm_ptr );

    /* Validate parameters if error checking is enabled */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPID_Comm_valid_ptr( comm_ptr, mpi_errno, FALSE );
            if (mpi_errno) goto fn_fail;
	    
	    MPIR_ERRTEST_COUNT(count, mpi_errno);
	    MPIR_ERRTEST_SEND_RANK(comm_ptr, dest, mpi_errno);
	    MPIR_ERRTEST_SEND_TAG(tag, mpi_errno);
	    
	    /* Validate datatype handle */
	    MPIR_ERRTEST_DATATYPE(datatype, "datatype", mpi_errno);
	    
	    /* Validate datatype object */
	    if (HANDLE_GET_KIND(datatype) != HANDLE_KIND_BUILTIN)
	    {
		MPID_Datatype *datatype_ptr = NULL;

		MPID_Datatype_get_ptr(datatype, datatype_ptr);
		MPID_Datatype_valid_ptr(datatype_ptr, mpi_errno);
		if (mpi_errno) goto fn_fail;
		MPID_Datatype_committed_ptr(datatype_ptr, mpi_errno);
		if (mpi_errno) goto fn_fail;
	    }
	    
	    /* Validate buffer */
	    MPIR_ERRTEST_USERBUFFER(buf,count,datatype,mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    
    mpi_errno = MPID_Send(buf, count, datatype, dest, tag, comm_ptr, 
			  MPID_CONTEXT_INTRA_PT2PT, &request_ptr);
    if (mpi_errno != MPI_SUCCESS) goto fn_fail;

    if (request_ptr == NULL)
    {
	goto fn_exit;
    }

    /* If a request was returned, then we need to block until the request 
       is complete */
    if (!MPID_Request_is_complete(request_ptr))
    {
	MPID_Progress_state progress_state;
	    
	MPID_Progress_start(&progress_state);
        while (!MPID_Request_is_complete(request_ptr))
	{
	    mpi_errno = MPID_Progress_wait(&progress_state);
	    if (mpi_errno != MPI_SUCCESS)
	    {
		/* --BEGIN ERROR HANDLING-- */
		MPID_Progress_end(&progress_state);
		goto fn_fail;
		/* --END ERROR HANDLING-- */
	    }
	}
	MPID_Progress_end(&progress_state);
    }

    mpi_errno = request_ptr->status.MPI_ERROR;
    MPID_Request_release(request_ptr);
    
    if (mpi_errno != MPI_SUCCESS) goto fn_fail;

    /* ... end of body of routine ... */
    
  fn_exit:
    MPID_MPI_PT2PT_FUNC_EXIT(MPID_STATE_MPI_SEND);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_send", 
	    "**mpi_send %p %d %D %i %t %C", buf, count, datatype, dest, tag, comm);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

/*Added by Abu Naser, june 11 */

#if 0
int MPI_SEC_Send_primery(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
	     MPI_Comm comm)
{
    int mpi_errno = MPI_SUCCESS;
    int i;
    //char * temp_buf;
    //temp_buf = (char *) MPIU_Malloc((count) * sizeof(datatype));

    /* copy original data to temp buffer */
   // memcpy(temp_buf,buf,sizeof(datatype) * count);

   // for(i=0;i<count;i++){
   //     printf("temp_buf=%x,%c\n",*temp_buf,*temp_buf);fflush(stdout);
   // }
   
    //int var = sodium_init();
    
    //unsigned char ciphertext[MESSAGE_LEN + crypto_aead_aes256gcm_ABYTES];
   // char * ciphertext;
    //ciphertext=(char *) MPIU_Malloc(((count) * sizeof(datatype))  + 16);
    int ciphertext_len=0; 
    // int ADDITIONAL_DATA_LEN = 6;
     char nonce[12] = {'1','2','3','4','5','6','7','8','9','0','1','2'};
    // char ADDITIONAL_DATA[6] = {'1','2','3','4','5','6'};
    //unsigned char decrypted[MESSAGE_LEN];
	//unsigned long long decrypted_len;
    //unsigned char * MESSAGE; 
    int * val;
    unsigned char * c;
    int   max_out_len = 64 + count;

    /*EVP_AEAD_CTX *enctx = NULL; 
    enctx = EVP_AEAD_CTX_new(EVP_aead_aes_256_gcm_siv(),
                            key,
                            32, 0);*/ 
   
    //crypto_aead_aes256gcm_encrypt(ciphertext, &ciphertext_len,
    //                          buf, count,
    //                          ADDITIONAL_DATA, ADDITIONAL_DATA_LEN,
    //                          NULL, nonce, key);
     if(!EVP_AEAD_CTX_seal(ctx, ciphertext,
                                     &ciphertext_len, max_out_len,
                                     nonce, 12,
                                     buf,  count,
                                     NULL, 0)
            ){
                //printf("Encryption done: EVP_aead_aes_256_gcm, ciphertext length is %lu count=%d\n",ciphertext_len,count);fflush(stdout);
                //for(j=0;j<ciphertext_len;j++)
                //    printf("%02x ",(unsigned int)ciphertext[j]);
                //printf("\n");    

              printf("error in encryption\n");fflush(stdout);
            }
            //else{
              //  printf("encryption success\n");
           // }
   
    
	//printf("send: sending cipher text length is = %d\n",ciphertext_len);
    //fflush(stdout);
	///for(i=0;i<ciphertext_len;i++){
	//	printf("%x ",*((unsigned char *)(ciphertext+i)));
    //    fflush(stdout);
    //}
	//printf("\n"); fflush(stdout);
    
 
    mpi_errno=MPI_Send(ciphertext, ciphertext_len, datatype, dest, tag, comm);

    //memcpy(buf,temp_buf,sizeof(datatype)*count);
    //MPIU_Free(temp_buf);
    //MPIU_Free(ciphertext);
    //EVP_AEAD_CTX_free(enctx);
    return mpi_errno;
}
#endif

/* variable nonce implementation */
int MPI_SEC_Send(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
	     MPI_Comm comm)
{
    int mpi_errno = MPI_SUCCESS;
    unsigned long ciphertext_len=0;
    int  sendtype_sz;           
    
    MPI_Type_size(datatype, &sendtype_sz);         
    unsigned long   max_out_len = (unsigned long) (16 + (sendtype_sz*count));
    
    /* Set the nonce in send_ciphertext */
    RAND_bytes(ciphertext, 12); // 12 bytes of nonce
    /*nonceCounter++;
    memset(ciphertext, 0, 8);
    ciphertext[8] = (nonceCounter >> 24) & 0xFF;
    ciphertext[9] = (nonceCounter >> 16) & 0xFF;
    ciphertext[10] = (nonceCounter >> 8) & 0xFF;
    ciphertext[11] = nonceCounter & 0xFF;*/

    /* ciphertext's first 12 byte is now nonce. so cipher will start from */
    /* ciphertext+12. And nonce is from ciphertext                   */    
    if(!EVP_AEAD_CTX_seal(ctx, ciphertext+12,
                         &ciphertext_len, max_out_len,
                         ciphertext, 12,
                         buf,  (unsigned long)(count*sendtype_sz),
                        NULL, 0)){
              printf("error in encryption\n");
              fflush(stdout);
        }
    
    /* Sending additional 12 bytes for nonce */  
	mpi_errno = MPI_Send(ciphertext, ciphertext_len+12, MPI_CHAR, dest, tag, comm);

    return mpi_errno;
}

/* Fixed nonce implementation */
#if 0
int MPI_SEC_Send(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
	     MPI_Comm comm)
{
    int mpi_errno = MPI_SUCCESS;
    unsigned long ciphertext_len=0;
    int  sendtype_sz;           
    
    MPI_Type_size(datatype, &sendtype_sz);         
    unsigned long   max_out_len = (unsigned long) (16 + (sendtype_sz*count));
    
    if(!EVP_AEAD_CTX_seal(ctx, ciphertext,
                         &ciphertext_len, max_out_len,
                         nonce, 12,
                         buf,  (unsigned long)(count*sendtype_sz),
                        NULL, 0)){
              printf("error in encryption\n");
              fflush(stdout);
        }
    
	mpi_errno = MPI_Send(ciphertext, ciphertext_len, MPI_CHAR, dest, tag, comm);

    return mpi_errno;
}
#endif

#if 0

void IV_Count(unsigned char *IV ,int cter){
	uint32_t n = 16, c = (uint32_t) cter;
	do {
        --n;
        c += IV[n];
        IV[n] = (uint8_t)c;
        c >>= 8;
    } while (n);
}

int MPI_PreCtr_Send(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm)
{
    int mpi_errno = MPI_SUCCESS;
	int i,len,segments,pre_fin,enc_start;		
	int rmd, sendtype_sz, msg_rmd,x,y;

	MPI_Type_size(datatype, &sendtype_sz);	
	unsigned long long blocktype_send= (unsigned long long)sendtype_sz*count;

	#if 0
	if (blocktype_send > 8192){
			printf("\nError: message size must <= 8K!!!\n");
			return 1;
	}
	#endif

	//total pre-calcu bytes
	amount= pre_end[dest] - pre_start[dest];

	if (blocktype_send >= 66560-pre_start[dest]){
		if(first_flag[dest] ==1){
			/* use CTR mode */
			memcpy(send_ciphertext,&IV[dest][0],16);
			EVP_EncryptInit_ex(ctx_enc, NULL, NULL, key, &IV[dest][0]);
			EVP_EncryptUpdate(ctx_enc, &send_ciphertext[16], &len, buf, blocktype_send);
			iv_counter[dest] = (blocktype_send-1)/16+1;
			pre_end[dest] =0;
			first_flag[dest] =0;
            mpi_errno=MPI_Send(send_ciphertext,blocktype_send+16, MPI_CHAR, dest, tag, comm);
		}else{
			//1. Use the rest of the Array
			segments =66560-pre_end[dest];
			if(segments !=0){
				pre_fin = pre_end[dest]; 
				memcpy(&enc_iv[dest][0],&IV[dest][0],16);
				IV_Count(&enc_iv[dest][0],iv_counter[dest]);
				EVP_EncryptInit_ex(ctx_enc, NULL, NULL, key, &enc_iv[dest][0]);
				EVP_EncryptUpdate(ctx_enc, &pre_calculator[dest][pre_fin], &len, p, segments);
				iv_counter[dest] += (segments-1)/16+1;
			}
			rmd=66560-pre_start[dest];
			enc_start = pre_start[dest];
			for(i=0; i< rmd; i++){
        		send_ciphertext[i] = (unsigned char )(pre_calculator[dest][enc_start+i] ^ *((unsigned char *)(buf+i)));	
			}

			//2.Msg_rmd
			msg_rmd = blocktype_send-rmd;
			if(msg_rmd>=66560){
				/* For msg size >=66560, use CTR mode */
				memcpy(&enc_iv[dest][0],&IV[dest][0],16);
				IV_Count(&enc_iv[dest][0],iv_counter[dest]);
				EVP_EncryptInit_ex(ctx_enc, NULL, NULL, key, &enc_iv[dest][0]);
				EVP_EncryptUpdate(ctx_enc, &send_ciphertext[rmd], &len, buf+rmd, msg_rmd);
				iv_counter[dest] += (msg_rmd-1)/16+1; 
				pre_start[dest] = 0;
				pre_end[dest] =0;
                mpi_errno=MPI_Send(send_ciphertext,blocktype_send, MPI_CHAR, dest, tag, comm);
			}else{
				/*Recycle*/
				//printf("\n[ISEND Cycle start ISEND]: iv_counter=%d, rmd=%d\n",iv_counter[dest],rmd);
				memcpy(&enc_iv[dest][0],&IV[dest][0],16);
				IV_Count(&enc_iv[dest][0],iv_counter[dest]);
				segments =((blocktype_send-rmd-1)/16)*16+16;
		
				EVP_EncryptInit_ex(ctx_enc, NULL, NULL, key, &enc_iv[dest][0]);
				EVP_EncryptUpdate(ctx_enc, &pre_calculator[dest][0], &len, p, segments);
				iv_counter[dest] += (segments/16); 
				pre_end[dest] = segments;

				for(i=0; i< blocktype_send-rmd; i++){
        			send_ciphertext[rmd+i] = (unsigned char )(pre_calculator[dest][i] ^ *((unsigned char *)(buf+rmd+i)));
	    		}
				pre_start[dest] = blocktype_send-rmd;
				//printf("\n[ISEND]:Enc_Start=%d ,Enc_End=%d Enc_Counter=%d\n",pre_start[dest], pre_end[dest],iv_counter[dest]);
				mpi_errno=MPI_Send(send_ciphertext,blocktype_send, MPI_CHAR, dest, tag, comm);
			}	
		}

	}else if(blocktype_send > amount){
		//Generate more pre-ctr blocks
		memcpy(&enc_iv[dest][0],&IV[dest][0],16);
		IV_Count(&enc_iv[dest][0],iv_counter[dest]);
		segments =((blocktype_send-amount-1)/16)*16+16;
		pre_fin = pre_end[dest]; 
		EVP_EncryptInit_ex(ctx_enc, NULL, NULL, key, &enc_iv[dest][0]);
		EVP_EncryptUpdate(ctx_enc, &pre_calculator[dest][pre_fin], &len, p, segments); 
		iv_counter[dest] += (segments/16); //upper integer
		pre_end[dest] += segments ;
		//Encryption
   		if (first_flag[dest] ==1){
	   		memcpy(send_ciphertext,&IV[dest][0],16);
			//printf("\n[ISEND]:Enc_Start=%d ,Enc_End=%d Enc_Counter=%d\n",pre_start[dest], pre_end[dest],iv_counter[dest]);
	  	    for(i=0; i< blocktype_send; i++){
		  			send_ciphertext[i+16] = (unsigned char )(pre_calculator[dest][i] ^ *((unsigned char *)(buf+i)));
	  		}
	   		first_flag[dest] =0;
	   		pre_start[dest] +=blocktype_send;
            mpi_errno=MPI_Send(send_ciphertext,blocktype_send+16, MPI_CHAR, dest, tag, comm);
	
		}else{
			//printf("\n[ISEND]:Enc_Start=%d ,Enc_End=%d Enc_Counter=%d\n",pre_start[dest], pre_end[dest],iv_counter[dest]);
			for(i=0; i< blocktype_send; i++){
			enc_start = pre_start[dest];
        	send_ciphertext[i] = (unsigned char )(pre_calculator[dest][enc_start+i] ^ *((unsigned char *)(buf+i)));
	   		}
			pre_start[dest] +=blocktype_send;
			mpi_errno=MPI_Send(send_ciphertext,blocktype_send, MPI_CHAR, dest, tag, comm);
		}			
	}else{
		   //Encryption
   			if (first_flag[dest] ==1){
	   			memcpy(send_ciphertext,&IV[dest][0],16);
				//printf("\n[ISEND]:Enc_Start=%d ,Enc_End=%d Enc_Counter=%d\n",pre_start[dest], pre_end[dest],iv_counter[dest]);
	  	    	for(i=0; i< blocktype_send; i++){
		  			send_ciphertext[i+16] = (unsigned char )(pre_calculator[dest][i] ^ *((unsigned char *)(buf+i)));
	  			 }
	   			first_flag[dest] =0;
	   			pre_start[dest] +=blocktype_send;
                mpi_errno=MPI_Send(send_ciphertext,blocktype_send+16, MPI_CHAR, dest, tag, comm);
			}else{
				//printf("\n[ISEND]:Enc_Start=%d ,Enc_End=%d Enc_Counter=%d\n",pre_start[dest], pre_end[dest],iv_counter[dest]);
				for(i=0; i< blocktype_send; i++){
				enc_start = pre_start[dest];
        		send_ciphertext[i] = (unsigned char )(pre_calculator[dest][enc_start+i] ^ *((unsigned char *)(buf+i)));
	   		    }
			    pre_start[dest] +=blocktype_send;
                mpi_errno=MPI_Send(send_ciphertext,blocktype_send, MPI_CHAR, dest, tag, comm);
			}
	
	}
	//Generationg new pre-ctr blocks
	amount= pre_end[dest] - pre_start[dest];
    if(amount<128){
        if(pre_end[dest]<66560){
            memcpy(&enc_iv[dest][0],&IV[dest][0],16);
            IV_Count(&enc_iv[dest][0],iv_counter[dest]);
			EVP_EncryptInit_ex(ctx_enc, NULL, NULL, key, &enc_iv[dest][0]);
			pre_fin = pre_end[dest];
            x=66560-pre_end[dest];
            y= ((x) < (128)) ? (x) : (128);
			EVP_EncryptUpdate(ctx_enc, &pre_calculator[dest][pre_fin], &len, p, y);
			iv_counter[dest] += (y/16);
            pre_end[dest] += y;
		}
	}
	return mpi_errno;
}

#endif





void init_crypto(){
    nonceCounter=0;
    ctx = EVP_AEAD_CTX_new(EVP_aead_aes_256_gcm_siv(),
                            key,
                            32, 0);
    return;                        
}


void init_boringssl_256_siv(){
    //unsigned char key_boringssl_siv_32 [32] = {'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z','a','b','c','d','e','f'};
    nonceCounter=0;
    ctx = EVP_AEAD_CTX_new(EVP_aead_aes_256_gcm_siv(),
                            key,
                            32, 0);
    return;                        
}

void init_boringssl_128_siv(){
   // unsigned char key_boringssl_siv_16 [16] = {'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p'};
    nonceCounter=0;
    ctx = EVP_AEAD_CTX_new(EVP_aead_aes_128_gcm_siv(),
                            key,
                            16, 0);
    return;                        
}

void init_boringssl_128(){
   // unsigned char key_boringssl_16 [16] = {'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p'};
    nonceCounter=0;
    ctx = EVP_AEAD_CTX_new(EVP_aead_aes_128_gcm(),
                            key,
                            16, 0);
    return;                        
}

void init_boringssl_256(){
    //unsigned char key_boringssl_32 [32] = {'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z','a','b','c','d','e','f'};
    nonceCounter=0;
    ctx = EVP_AEAD_CTX_new(EVP_aead_aes_256_gcm(),
                            key,
                            32, 0);
    return;                        
}

#if 0

void init_boringssl_ctr_128(){

	ctx_enc = EVP_CIPHER_CTX_new();
	EVP_EncryptInit_ex(ctx_enc, EVP_aes_128_ctr(), NULL, NULL, NULL);
	ctx_dec = EVP_CIPHER_CTX_new();
	EVP_DecryptInit_ex(ctx_dec, EVP_aes_128_ctr(), NULL, NULL, NULL);
    return;                        
}

void init_boringssl_ctr_256(){

	ctx_enc = EVP_CIPHER_CTX_new();
	EVP_EncryptInit_ex(ctx_enc, EVP_aes_256_ctr(), NULL, NULL, NULL);
	ctx_dec = EVP_CIPHER_CTX_new();
	EVP_DecryptInit_ex(ctx_dec, EVP_aes_256_ctr(), NULL, NULL, NULL);
    return;                        
}
#endif

/* Initial Key Aggrement */
/*
int MPI_SEC_Initial_Key_Aggrement(int keysz){

 if(keysz == 16 || keysz == 32)
  keysize = keysz;
 else
 {
     keysize = 16;
 }
  
   
}
*/
/*End of adding */

/*Pre-CTR Mode*/
#if 0
void init_prectr_128(){
	int len, i, rank, world_size;
	memset(p,0,8020);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

	ctx_enc = EVP_CIPHER_CTX_new();
	EVP_EncryptInit_ex(ctx_enc, EVP_aes_128_ctr(), NULL, NULL, NULL);

	//Generate different Pre-CTR Arrays for different receivers
	for (i=0;i<world_size;i++){
		if(i != rank){
			RAND_bytes(&IV[i][0], 16);
			EVP_EncryptInit_ex(ctx_enc, NULL, NULL, key, &IV[i][0]);
			EVP_EncryptUpdate(ctx_enc, &pre_calculator[i][0], &len, p, 20480);
			iv_counter[i] = 1280;
			pre_start[i] = 0;
			pre_end[i] = 20480;
		}
		first_flag[i] = 1;
		dec_flag[i] =1;
		sloop_flag[i]=1;
		rloop_flag[i]=1;
	}
	
	ctx_dec = EVP_CIPHER_CTX_new();
	EVP_DecryptInit_ex(ctx_dec, EVP_aes_128_ctr(), NULL, NULL, NULL);
    return 0;
}
#endif

int MPI_CTR_Send(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm , int max_pack)
{
	#if 0
    int mpi_errno = MPI_SUCCESS;
	int world_rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
	int i=0;
			
	char * ciphertext;
	int sendtype_sz;
	MPI_Type_size(datatype, &sendtype_sz);
    	
	unsigned long long blocktype_send= (unsigned long long) sendtype_sz*count;		

	RAND_bytes(send_ciphertext, 16);
	EVP_EncryptInit_ex(ctx_enc, NULL, NULL, gcm_key, send_ciphertext);
	EVP_EncryptUpdate(ctx_enc, send_ciphertext+16, &outlen_enc, buf, blocktype_send);

	mpi_errno=MPI_Send(send_ciphertext,blocktype_send+16, MPI_CHAR, dest, tag, comm);
	
	return mpi_errno;
	#endif
}