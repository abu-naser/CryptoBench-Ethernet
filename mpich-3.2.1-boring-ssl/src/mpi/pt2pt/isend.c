/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* added by Abu Naser */
#include <openssl/evp.h>
#include <openssl/aes.h>
#include <openssl/err.h>
#include <openssl/aead.h>
//unsigned char Iciphertext[4194304+18];
//unsigned char Iciphertext[3000][1100000];
unsigned char Iciphertext[NON_BLOCKING_SEND_RECV_SIZE][NON_BLOCKING_SEND_RECV_SIZE_2];
int isendCounter = 0;

//Pre-CTR Mode
int enc_dest[1024], psend_counter, ctr_swait_flag[1024], ctr_rwait_flag[1024];
psend_counter=0;
/* End of add. */

/* -- Begin Profiling Symbol Block for routine MPI_Isend */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Isend = PMPI_Isend
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Isend  MPI_Isend
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Isend as PMPI_Isend
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Isend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
              MPI_Comm comm, MPI_Request *request) __attribute__((weak,alias("PMPI_Isend")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Isend
#define MPI_Isend PMPI_Isend

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Isend
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@
    MPI_Isend - Begins a nonblocking send

Input Parameters:
+ buf - initial address of send buffer (choice) 
. count - number of elements in send buffer (integer) 
. datatype - datatype of each send buffer element (handle) 
. dest - rank of destination (integer) 
. tag - message tag (integer) 
- comm - communicator (handle) 

Output Parameters:
. request - communication request (handle) 

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_COMM
.N MPI_ERR_COUNT
.N MPI_ERR_TYPE
.N MPI_ERR_TAG
.N MPI_ERR_RANK
.N MPI_ERR_EXHAUSTED

@*/
int MPI_Isend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
	      MPI_Comm comm, MPI_Request *request)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPID_Request *request_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_ISEND);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPID_MPI_PT2PT_FUNC_ENTER_FRONT(MPID_STATE_MPI_ISEND);

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
	    MPIR_ERRTEST_ARGNULL(request,"request",mpi_errno);

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
    
    mpi_errno = MPID_Isend(buf, count, datatype, dest, tag, comm_ptr,
			   MPID_CONTEXT_INTRA_PT2PT, &request_ptr);
    if (mpi_errno != MPI_SUCCESS) goto fn_fail;

    MPIR_SENDQ_REMEMBER(request_ptr,dest,tag,comm_ptr->context_id);

    /* return the handle of the request to the user */
    /* MPIU_OBJ_HANDLE_PUBLISH is unnecessary for isend, lower-level access is
     * responsible for its own consistency, while upper-level field access is
     * controlled by the completion counter */
    *request = request_ptr->handle;

    /* ... end of body of routine ... */
    
  fn_exit:
    MPID_MPI_PT2PT_FUNC_EXIT(MPID_STATE_MPI_ISEND);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;
    
  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_isend",
	    "**mpi_isend %p %d %D %i %t %C %p", buf, count, datatype, dest, tag, comm, request);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

/* Added by Abu Naser, an16e@my.fsu.edu june 19 2018 */
/*Variable nonce*/
int MPI_SEC_Isend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request)
{

    int mpi_errno = MPI_SUCCESS;
    MPI_Status status;
    MPI_Request req;
	int rank;
	
    unsigned long ciphertext_len=0;
    int  sendtype_sz;           
    
    MPI_Type_size(datatype, &sendtype_sz);         
    unsigned long   max_out_len = (unsigned long) (16 + (sendtype_sz*count));

    /* Set the nonce in send_ciphertext */
    RAND_bytes(&Iciphertext[isendCounter][0], 12); // 12 bytes of nonce
    /*nonceCounter++;
    memset(&Iciphertext[isendCounter][0], 0, 8);
    Iciphertext[isendCounter][8] = (nonceCounter >> 24) & 0xFF;
    Iciphertext[isendCounter][9] = (nonceCounter >> 16) & 0xFF;
    Iciphertext[isendCounter][10] = (nonceCounter >> 8) & 0xFF;
    Iciphertext[isendCounter][11] = nonceCounter & 0xFF;*/

    if(!EVP_AEAD_CTX_seal(ctx, &Iciphertext[isendCounter][12],
                        &ciphertext_len, max_out_len,
                        &Iciphertext[isendCounter][0], 12,
                        buf,  (unsigned long)(count * sendtype_sz),
                        NULL, 0)){
                printf("Error in encryption\n");
                fflush(stdout);    
            }
         
    mpi_errno = MPI_Isend(&Iciphertext[isendCounter][0], ciphertext_len+12, MPI_CHAR, dest, tag, comm, &req);
    * request = req;
    isendCounter++;
	//MPI_Comm_rank(comm, & rank);
	//printf("[rank = %d] isendCounter =%d \n",rank, isendCounter-1);fflush(stdout);
    if(isendCounter == (NON_BLOCKING_SEND_RECV_SIZE-1))
        isendCounter=0;

    return mpi_errno;
}

/*Fixed nonce*/
#if 0
int MPI_SEC_Isend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request)
{

    int mpi_errno = MPI_SUCCESS;
    MPI_Status status;
    MPI_Request req;

    unsigned long ciphertext_len=0;
    int  sendtype_sz;           
    
    MPI_Type_size(datatype, &sendtype_sz);         
    unsigned long   max_out_len = (unsigned long) (16 + (sendtype_sz*count));

    if(!EVP_AEAD_CTX_seal(ctx, &Iciphertext[isendCounter][0],
                        &ciphertext_len, max_out_len,
                        nonce, 12,
                        buf,  (unsigned long)(count * sendtype_sz),
                        NULL, 0)){
                printf("Error in encryption\n");
                fflush(stdout);    
            }
         
    mpi_errno = MPI_Isend(&Iciphertext[isendCounter][0], ciphertext_len, MPI_CHAR, dest, tag, comm, &req);
    * request = req;
    isendCounter++;
    if(isendCounter == (20000-1))
        isendCounter=0;

    return mpi_errno;
}
#endif
/* End of Added by abu naser. */

/* This implementation is for Pre-CTR Mode */
#if 0
int MPI_PreCtr_Isend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, 
                     MPI_Comm comm, MPI_Request *request,MPI_Status *status)
{
    int mpi_errno = MPI_SUCCESS;
	int i,len,segments,pre_fin,enc_start;		
	int rmd, sendtype_sz, msg_rmd;
    MPI_Request req;
   

	enc_dest[psend_counter] = dest;
	ctr_swait_flag[dest]=0;

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
			memcpy(&Iciphertext[isendCounter][0],&IV[dest][0],16);
			EVP_EncryptInit_ex(ctx_enc, NULL, NULL, key, &IV[dest][0]);
			EVP_EncryptUpdate(ctx_enc, &Iciphertext[isendCounter][16], &len, buf, blocktype_send);
			iv_counter[dest] = (blocktype_send-1)/16+1;
			pre_end[dest] =0;
			ctr_swait_flag[dest]=1;
			ctr_rwait_flag[dest]=1;
			first_flag[dest] =0;
			mpi_errno=MPI_Isend(&Iciphertext[isendCounter][0],blocktype_send+16, MPI_CHAR, dest, tag, comm, &req);
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
        		Iciphertext[isendCounter][i] = (unsigned char )(pre_calculator[dest][enc_start+i] ^ *((unsigned char *)(buf+i)));	
			}

			//2.Msg_rmd
			msg_rmd = blocktype_send-rmd;
			if(msg_rmd>=66560){
				/* For msg size >=66560, use CTR mode */
				memcpy(&enc_iv[dest][0],&IV[dest][0],16);
				IV_Count(&enc_iv[dest][0],iv_counter[dest]);
				EVP_EncryptInit_ex(ctx_enc, NULL, NULL, key, &enc_iv[dest][0]);
				EVP_EncryptUpdate(ctx_enc, &Iciphertext[isendCounter][rmd], &len, buf+rmd, msg_rmd);
				iv_counter[dest] += (msg_rmd-1)/16+1; 
				pre_start[dest] = 0;
				pre_end[dest] =0;
				ctr_swait_flag[dest]=1;
				ctr_rwait_flag[dest]=1;
				mpi_errno=MPI_Isend(&Iciphertext[isendCounter][0],blocktype_send, MPI_CHAR, dest, tag, comm, &req);
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
        			Iciphertext[isendCounter][rmd+i] = (unsigned char )(pre_calculator[dest][i] ^ *((unsigned char *)(buf+rmd+i)));
	    		}
				pre_start[dest] = blocktype_send-rmd;
				//printf("\n[ISEND]:Enc_Start=%d ,Enc_End=%d Enc_Counter=%d\n",pre_start[dest], pre_end[dest],iv_counter[dest]);
				mpi_errno=MPI_Isend(&Iciphertext[isendCounter][0],blocktype_send, MPI_CHAR, dest, tag, comm, &req);
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
	   		memcpy(&Iciphertext[isendCounter][0],&IV[dest][0],16);
			//printf("\n[ISEND]:Enc_Start=%d ,Enc_End=%d Enc_Counter=%d\n",pre_start[dest], pre_end[dest],iv_counter[dest]);
	  	    for(i=0; i< blocktype_send; i++){
		  			Iciphertext[isendCounter][i+16] = (unsigned char )(pre_calculator[dest][i] ^ *((unsigned char *)(buf+i)));
	  		}
	   		first_flag[dest] =0;
	   		pre_start[dest] +=blocktype_send;
			
	   
	   		mpi_errno=MPI_Isend(&Iciphertext[isendCounter][0],blocktype_send+16, MPI_CHAR, dest, tag, comm, &req);
		}else{
			//printf("\n[ISEND]:Enc_Start=%d ,Enc_End=%d Enc_Counter=%d\n",pre_start[dest], pre_end[dest],iv_counter[dest]);
			for(i=0; i< blocktype_send; i++){
			enc_start = pre_start[dest];
        	Iciphertext[isendCounter][i] = (unsigned char )(pre_calculator[dest][enc_start+i] ^ *((unsigned char *)(buf+i)));
	   		}
			pre_start[dest] +=blocktype_send;
			
			mpi_errno=MPI_Isend(&Iciphertext[isendCounter][0],blocktype_send, MPI_CHAR, dest, tag, comm, &req);
		}			
	}else{
		   //Encryption
   			if (first_flag[dest] ==1){
	   			memcpy(&Iciphertext[isendCounter][0],&IV[dest][0],16);
				//printf("\n[ISEND]:Enc_Start=%d ,Enc_End=%d Enc_Counter=%d\n",pre_start[dest], pre_end[dest],iv_counter[dest]);
	  	    	for(i=0; i< blocktype_send; i++){
		  			 Iciphertext[isendCounter][i+16] = (unsigned char )(pre_calculator[dest][i] ^ *((unsigned char *)(buf+i)));
	  			 }
	   			first_flag[dest] =0;
	   			pre_start[dest] +=blocktype_send;
	   
	   			mpi_errno=MPI_Isend(&Iciphertext[isendCounter][0],blocktype_send+16, MPI_CHAR, dest, tag, comm, &req);
			}else{
				//printf("\n[ISEND]:Enc_Start=%d ,Enc_End=%d Enc_Counter=%d\n",pre_start[dest], pre_end[dest],iv_counter[dest]);
				for(i=0; i< blocktype_send; i++){
				enc_start = pre_start[dest];
        		Iciphertext[isendCounter][i] = (unsigned char )(pre_calculator[dest][enc_start+i] ^ *((unsigned char *)(buf+i)));
	   		 }
			pre_start[dest] +=blocktype_send;
			
			mpi_errno=MPI_Isend(&Iciphertext[isendCounter][0],blocktype_send, MPI_CHAR, dest, tag, comm, &req);
			}
	
	}
	* request = req;
	isendCounter++;
	if(isendCounter == (500-1))
       	isendCounter=0;
	return mpi_errno;
}
#endif