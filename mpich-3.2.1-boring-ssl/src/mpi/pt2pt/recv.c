/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
/* added by abu naser */
//#include <sodium.h> // added by abu naser
//#include "/home/abu/study/research/DICE_project/boringssl/include/openssl/evp.h"
//#include "/home/abu/study/research/DICE_project/boringssl/include/openssl/aes.h"
//#include "/home/abu/study/research/DICE_project/boringssl/include/openssl/err.h"
#include <openssl/evp.h>
#include <openssl/aes.h>
#include <openssl/err.h>
#include <openssl/aead.h>
unsigned char deciphertext[4194304+18];
 //int ADDITIONAL_DATA_LEN1=6;
  //char nonce1[12] = {'1','2','3','4','5','6','7','8','9','0','1','2'};
  //char ADDITIONAL_DATA1[6] = {'1','2','3','4','5','6'};
/* end of add */


/*Pre-CTR*/
#if 0
unsigned char ciphertext_recv[419+400];
EVP_CIPHER_CTX *ctx_dec;
unsigned char Recv_IV[1024][16],dec_iv[1024][16], dec_calculator[1][6];
int dec_start[1024], dec_end[1024],dec_flag[1024],dec_amount;
int dec_counter[1024];
#endif


/* -- Begin Profiling Symbol Block for routine MPI_Recv */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Recv = PMPI_Recv
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Recv  MPI_Recv
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Recv as PMPI_Recv
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag,
             MPI_Comm comm, MPI_Status *status) __attribute__((weak,alias("PMPI_Recv")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Recv
#define MPI_Recv PMPI_Recv

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Recv

/*@
    MPI_Recv - Blocking receive for a message

Output Parameters:
+ buf - initial address of receive buffer (choice) 
- status - status object (Status) 

Input Parameters:
+ count - maximum number of elements in receive buffer (integer) 
. datatype - datatype of each receive buffer element (handle) 
. source - rank of source (integer) 
. tag - message tag (integer) 
- comm - communicator (handle) 

Notes:
The 'count' argument indicates the maximum length of a message; the actual 
length of the message can be determined with 'MPI_Get_count'.  

.N ThreadSafe

.N Fortran

.N FortranStatus

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_COMM
.N MPI_ERR_TYPE
.N MPI_ERR_COUNT
.N MPI_ERR_TAG
.N MPI_ERR_RANK

@*/
int MPI_Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag,
	     MPI_Comm comm, MPI_Status *status)
{
    static const char FCNAME[] = "MPI_Recv";
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPID_Request * request_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_RECV);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPID_MPI_PT2PT_FUNC_ENTER_BACK(MPID_STATE_MPI_RECV);
    
    /* Validate handle parameters needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_COMM(comm, mpi_errno);
	    /* NOTE: MPI_STATUS_IGNORE != NULL */
	    MPIR_ERRTEST_ARGNULL(status, "status", mpi_errno);
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
	    MPIR_ERRTEST_RECV_RANK(comm_ptr, source, mpi_errno);
	    MPIR_ERRTEST_RECV_TAG(tag, mpi_errno);
	    
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

    /* MT: Note that MPID_Recv may release the SINGLE_CS if it
       decides to block internally.  MPID_Recv in that case will
       re-aquire the SINGLE_CS before returnning */
    mpi_errno = MPID_Recv(buf, count, datatype, source, tag, comm_ptr, 
			  MPID_CONTEXT_INTRA_PT2PT, status, &request_ptr);
    if (mpi_errno != MPI_SUCCESS) goto fn_fail;

    if (request_ptr == NULL)
    {
	goto fn_exit;
    }
    
    /* If a request was returned, then we need to block until the request is 
       complete */
    if (!MPID_Request_is_complete(request_ptr))
    {
	MPID_Progress_state progress_state;
	    
	MPID_Progress_start(&progress_state);
        while (!MPID_Request_is_complete(request_ptr))
	{
	    /* MT: Progress_wait may release the SINGLE_CS while it
	       waits */
	    mpi_errno = MPID_Progress_wait(&progress_state);
	    if (mpi_errno != MPI_SUCCESS)
	    { 
		/* --BEGIN ERROR HANDLING-- */
		MPID_Progress_end(&progress_state);
		goto fn_fail;
		/* --END ERROR HANDLING-- */
	    }

            if (unlikely(MPIR_CVAR_ENABLE_FT &&
                        !MPID_Request_is_complete(request_ptr) &&
                        MPID_Request_is_anysource(request_ptr) &&
                        !MPID_Comm_AS_enabled(request_ptr->comm))) {
                /* --BEGIN ERROR HANDLING-- */
                MPID_Cancel_recv(request_ptr);
                MPIR_STATUS_SET_CANCEL_BIT(request_ptr->status, FALSE);
                MPIR_ERR_SET(request_ptr->status.MPI_ERROR, MPIX_ERR_PROC_FAILED, "**proc_failed");
                mpi_errno = request_ptr->status.MPI_ERROR;
                goto fn_fail;
                /* --END ERROR HANDLING-- */
            }
	}
	MPID_Progress_end(&progress_state);
    }

    mpi_errno = request_ptr->status.MPI_ERROR;
    MPIR_Request_extract_status(request_ptr, status);
    MPID_Request_release(request_ptr);

    if (mpi_errno != MPI_SUCCESS) goto fn_fail;

    /* ... end of body of routine ... */
    
  fn_exit:
    MPID_MPI_PT2PT_FUNC_EXIT_BACK(MPID_STATE_MPI_RECV);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_recv",
	    "**mpi_recv %p %d %D %i %t %C %p", buf, count, datatype, source, tag, comm, status);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

/*Added by Abu Naser, june 11 */
/* Variable nonce */
int MPI_SEC_Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag,
	     MPI_Comm comm, MPI_Status *status)
{
    int mpi_errno = MPI_SUCCESS;

    unsigned long  ciphertext_len;
    unsigned long decrypted_len=0;

    int  recvtype_sz;           
    MPI_Type_size(datatype, &recvtype_sz);         
    
    ciphertext_len = (unsigned long)((count * recvtype_sz) + 16); 
    
    /* Receive additional 12 bytes for the nonce and 16 bytes for the tag */
    mpi_errno = MPI_Recv(deciphertext, ciphertext_len+12, MPI_CHAR, source, tag, comm, status);
   
    /* First 12 bytes of the deciphertext is nonce. So ciphertext */ 
    /* begins from deciphertext+12. And nonce from deciphertext   */   
	if(!EVP_AEAD_CTX_open(ctx, buf,
                        &decrypted_len, (ciphertext_len),
                        deciphertext, 12,
                        (deciphertext+12), ciphertext_len,
                        NULL, 0)){
                    printf("Decryption error: recv\n");fflush(stdout);        
            }
           
    return mpi_errno;
}

/* Fixed nonce implementation */
#if 0
int MPI_SEC_Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag,
	     MPI_Comm comm, MPI_Status *status)
{
    int mpi_errno = MPI_SUCCESS;

    unsigned long  ciphertext_len;
    unsigned long decrypted_len=0;

    int  recvtype_sz;           
    MPI_Type_size(datatype, &recvtype_sz);         
    
    ciphertext_len = (unsigned long)((count * recvtype_sz) + 16); 
    mpi_errno = MPI_Recv(deciphertext, ciphertext_len, MPI_CHAR, source, tag, comm, status);
   
	if(!EVP_AEAD_CTX_open(ctx, buf,
                        &decrypted_len, ciphertext_len,
                        nonce, 12,
                        deciphertext, ciphertext_len,
                        NULL, 0)){
                    printf("Decryption error\n");fflush(stdout);        
            }
           
    return mpi_errno;
}
#endif
/*End of adding */

#if 0
int MPI_PreCtr_Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Status *status)
{    
    int mpi_errno = MPI_SUCCESS;
    int i,len,segments,dec_begin,dec_fin,rmd,cipher_rmd,recvtype_sz,x,y;         
    MPI_Type_size(datatype, &recvtype_sz);
    unsigned long long recv_sz= (unsigned long long) recvtype_sz*count;

    if (dec_flag[source]==1){
	   mpi_errno=MPI_Recv(ciphertext_recv, recv_sz+16, MPI_CHAR, source, tag, comm,status);
    }else{
       mpi_errno=MPI_Recv(ciphertext_recv, recv_sz, MPI_CHAR, source, tag, comm,status);
    }

    if (dec_flag[source]==1){
        memcpy(&Recv_IV[source][0],ciphertext_recv,16);
        if (recv_sz <=1040){
		   EVP_EncryptInit_ex(ctx_enc, NULL, NULL, key, &Recv_IV[source][0]);
	       EVP_EncryptUpdate(ctx_enc, &dec_calculator[source][0], &len, p, 1024);
		   dec_counter[source] = 64;
		   dec_start[source] = 0;
		   dec_end [source]= 1024;
           for(i=0; i< recv_sz-16; i++){
		        *((char *)(buf+i)) = (char )(dec_calculator[source][i] ^ (ciphertext_recv[i+16]));
	        }
	       dec_flag[source] =0;
	       dec_start[source] = recv_sz-16;
		}else if (recv_sz < 66576){
		   segments =((recv_sz-1)/16)*16+16;//upper integer multi of 16
		   EVP_EncryptInit_ex(ctx_enc, NULL, NULL, key, &Recv_IV[source][0]);
		   EVP_EncryptUpdate(ctx_enc, &dec_calculator[source][0], &len, p, segments); 
		   dec_counter[source] = (segments/16); //upper integer
		   dec_end[source]= segments;
		   dec_start[source] = 0;

           for(i=0; i< recv_sz-16; i++){
		        *((char *)(buf+i)) = (char )(dec_calculator[source][i] ^ (ciphertext_recv[i+16]));
	        }
	       dec_flag[source] =0;
	       dec_start[source] = recv_sz-16;
		}else{
    
            EVP_DecryptInit_ex(ctx_dec, NULL, NULL, key, &Recv_IV[source][0]);
            //EVP_EncryptUpdate(ctx_enc, &dec_calculator[source][0], &len, &Ideciphertext[waitCounter][16], recv_sz-16); 
             EVP_DecryptUpdate(ctx_dec, buf, &len, &ciphertext_recv[16], recv_sz-16);
             dec_counter[source] += (recv_sz-1)/16+1;
             dec_flag[source] =0;
        }	  
	}else{
		if (recv_sz >= 66560-dec_start[source]){
            //1. Use the rest of the Array
            segments = 66560-dec_end[source];
            if(segments !=0){
                memcpy(&dec_iv[source][0],&Recv_IV[source][0],16);
                IV_Count(&dec_iv[source][0],dec_counter[source]);
                dec_fin = dec_end[source];
                EVP_EncryptInit_ex(ctx_enc, NULL, NULL, key, &dec_iv[0][0]);
			    EVP_EncryptUpdate(ctx_enc, &dec_calculator[source][dec_fin], &len, p, segments); 
                dec_counter[source] += (segments-1)/16 +1;
            }
            rmd = 66560-dec_start[source];
	        dec_begin =dec_start[source];
            //printf("\n[IRECV]:Dec_Start=%d ,Dec_End=%d Dec_Counter=%d\n",dec_start[source], dec_end[source],dec_counter[source]);
	        for(i=0; i< rmd; i++){
		        *((char *)(buf+i)) = (char )(dec_calculator[source][dec_begin+i] ^ (ciphertext_recv[i]));   
            }

            //2. Cipher_rmd
            cipher_rmd=recv_sz-rmd;
            if(cipher_rmd >=66560){
                /* For ciphertext size >=66560, use CTR mode */
                memcpy(&dec_iv[source][0],&Recv_IV[source][0],16);
                IV_Count(&dec_iv[source][0],dec_counter[source]);
                EVP_DecryptInit_ex(ctx_dec, NULL, NULL, key, &dec_iv[source][0]);	
                EVP_DecryptUpdate(ctx_dec, buf+rmd, &len, &ciphertext_recv[rmd], cipher_rmd);
                dec_counter[source] += (cipher_rmd-1)/16+1;
                dec_end[source] = 0;
                dec_start[source] =0;
            }else{
                /*Recycle*/
                //printf("\n[3333-Irecv-3333]: dec_counter=%d, rmd=%d\n",dec_counter[source],rmd);
                memcpy(&dec_iv[source][0],&Recv_IV[source][0],16);
                IV_Count(&dec_iv[source][0],dec_counter[source]);
                segments =((recv_sz-rmd-1)/16)*16+16;
                EVP_EncryptInit_ex(ctx_enc, NULL, NULL, key, &dec_iv[source][0]);
                EVP_EncryptUpdate(ctx_enc, &dec_calculator[source][0], &len, p, segments);
                dec_counter[source] += (segments-1)/16+1;
                dec_end[source] = segments;
                //printf("\n[IRECV]:Dec_Start=%d ,Dec_End=%d Dec_Counter=%d\n",dec_start[source], dec_end[source],dec_counter[source]);
                for(i=0; i< recv_sz-rmd; i++){
		            *((char *)(buf+i+rmd)) = (char )(dec_calculator[source][i] ^ (ciphertext_recv[rmd+i]));
	            }
                dec_start[source]=recv_sz-rmd;
            }
		}else if(recv_sz > dec_amount){
			//Add more dec-ctr blocks
			memcpy(&dec_iv[source][0],&Recv_IV[source][0],16);
		    IV_Count(&dec_iv[source][0],dec_counter[source]);
			segments =((recv_sz-dec_amount-1)/16)*16+16;
			dec_fin = dec_end[source];
			EVP_EncryptInit_ex(ctx_enc, NULL, NULL, key, &dec_iv[0][0]);
			EVP_EncryptUpdate(ctx_enc, &dec_calculator[source][dec_fin], &len, p, segments); 
			dec_counter[source] += (segments/16); //upper integer
			dec_end[source] += segments;

            //Decryption	
	        dec_begin =dec_start[source];
            //printf("\n[IRECV]:Dec_Start=%d ,Dec_End=%d Dec_Counter=%d\n",dec_start[source], dec_end[source],dec_counter[source]);
	        for(i=0; i< recv_sz; i++){
		            *((char *)(buf+i)) = (char )(dec_calculator[source][dec_begin+i] ^ (ciphertext_recv[i]));
	        }
	        dec_start[source] += recv_sz;
	    }else{
            //Decryption	
	        dec_begin =dec_start[source];
            //printf("\n[IRECV]:Dec_Start=%d ,Dec_End=%d Dec_Counter=%d\n",dec_start[source], dec_end[source],dec_counter[source]);
	        for(i=0; i< recv_sz; i++){
		            *((char *)(buf+i)) = (char )(dec_calculator[source][dec_begin+i] ^ (ciphertext_recv[i]));
	        }
	        dec_start[source] += recv_sz;
        }
	  
	}
    //Generationg new pre-ctr blocks
    dec_amount= dec_end[source] - dec_start[source];
	if(dec_amount<128){ 
		if(dec_end[source]<66560){
            x=66560-dec_end[source];
            y= ((x) < (128)) ? (x) : (128);
            memcpy(&dec_iv[source][0],&Recv_IV[source][0],16);  

            IV_Count(&dec_iv[source][0],dec_counter[source]);
			dec_fin = dec_end[source];
			EVP_EncryptInit_ex(ctx_enc, NULL, NULL, key, &dec_iv[source][0]);
			EVP_EncryptUpdate(ctx_enc, &dec_calculator[source][dec_fin], &len, p, y);
			dec_counter[source] += (y/16);
			dec_end[source] += y;
                        
		}
	} 

    return mpi_errno;
}
#endif

int MPI_CTR_Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Status *status , int max_pack)
{    
    #if 0
	const unsigned char gcm_key[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

	int i;
	int recvtype_sz;
	MPI_Type_size(datatype, &recvtype_sz);
	unsigned long long blocktype_recv= (unsigned long long) recvtype_sz*count;
	
	//unsigned char * ciphertext_recv;
	unsigned long long next, src;
	
	int mpi_errno = MPI_SUCCESS;
	
	mpi_errno=MPI_Recv(ciphertext_recv, blocktype_recv+16, MPI_CHAR, source, tag, comm,status);
	
	// printf("Ciphertext @ Receiver:\n");
	// BIO_dump_fp(stdout, ciphertext, count+16);
	
	EVP_DecryptInit_ex(ctx_dec, NULL, NULL, gcm_key, ciphertext_recv);	
	EVP_DecryptUpdate(ctx_dec, buf, &outlen_dec, ciphertext_recv+16, blocktype_recv);
	
	//MPIU_Free(ciphertext_recv);
	
	return mpi_errno;
    #endif
}