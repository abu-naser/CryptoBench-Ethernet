/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
/* added by abu naser */
int remainsReq = 0;
int currentReq = 0;
/* end of add*/

#if !defined(MPID_REQUEST_PTR_ARRAY_SIZE)
#define MPID_REQUEST_PTR_ARRAY_SIZE 16
#endif

/* -- Begin Profiling Symbol Block for routine MPI_Waitsome */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Waitsome = PMPI_Waitsome
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Waitsome  MPI_Waitsome
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Waitsome as PMPI_Waitsome
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Waitsome(int incount, MPI_Request array_of_requests[], int *outcount,
                 int array_of_indices[], MPI_Status array_of_statuses[]) __attribute__((weak,alias("PMPI_Waitsome")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Waitsome
#define MPI_Waitsome PMPI_Waitsome

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Waitsome

/*@
    MPI_Waitsome - Waits for some given MPI Requests to complete

Input Parameters:
+ incount - length of array_of_requests (integer) 
- array_of_requests - array of requests (array of handles) 

Output Parameters:
+ outcount - number of completed requests (integer) 
. array_of_indices - array of indices of operations that 
completed (array of integers) 
- array_of_statuses - array of status objects for 
    operations that completed (array of Status).  May be 'MPI_STATUSES_IGNORE'.

Notes:
  The array of indicies are in the range '0' to 'incount - 1' for C and 
in the range '1' to 'incount' for Fortran.  

Null requests are ignored; if all requests are null, then the routine
returns with 'outcount' set to 'MPI_UNDEFINED'.

While it is possible to list a request handle more than once in the
array_of_requests, such an action is considered erroneous and may cause the
program to unexecpectedly terminate or produce incorrect results.

'MPI_Waitsome' provides an interface much like the Unix 'select' or 'poll' 
calls and, in a high qualilty implementation, indicates all of the requests
that have completed when 'MPI_Waitsome' is called.  
However, 'MPI_Waitsome' only guarantees that at least one
request has completed; there is no guarantee that `all` completed requests 
will be returned, or that the entries in 'array_of_indices' will be in 
increasing order. Also, requests that are completed while 'MPI_Waitsome' is
executing may or may not be returned, depending on the timing of the 
completion of the message.  

.N waitstatus

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_REQUEST
.N MPI_ERR_ARG
.N MPI_ERR_IN_STATUS
@*/
int MPI_Waitsome(int incount, MPI_Request array_of_requests[], 
		 int *outcount, int array_of_indices[],
		 MPI_Status array_of_statuses[])
{
    static const char FCNAME[] = "MPI_Waitsome";
    MPID_Request * request_ptr_array[MPID_REQUEST_PTR_ARRAY_SIZE];
    MPID_Request ** request_ptrs = request_ptr_array;
    MPI_Status * status_ptr;
    MPID_Progress_state progress_state;
    int i;
    int n_active;
    int n_inactive;
    int active_flag;
    int rc;
    int disabled_anysource = FALSE;
    int mpi_errno = MPI_SUCCESS;
    MPIU_CHKLMEM_DECL(1);
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_WAITSOME);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPID_MPI_PT2PT_FUNC_ENTER(MPID_STATE_MPI_WAITSOME);

    /* Check the arguments */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_COUNT(incount, mpi_errno);

	    if (incount != 0) {
		MPIR_ERRTEST_ARGNULL(array_of_requests, "array_of_requests", mpi_errno);
		MPIR_ERRTEST_ARGNULL(array_of_indices, "array_of_indices", mpi_errno);
		/* NOTE: MPI_STATUSES_IGNORE != NULL */
		MPIR_ERRTEST_ARGNULL(array_of_statuses, "array_of_statuses", mpi_errno);
	    }
	    MPIR_ERRTEST_ARGNULL(outcount, "outcount", mpi_errno);

	    for (i = 0; i < incount; i++) {
		MPIR_ERRTEST_ARRAYREQUEST_OR_NULL(array_of_requests[i], i, mpi_errno);
	    }
	}
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */
    
    /* ... body of routine ...  */
    
    *outcount = 0;
    
    /* Convert MPI request handles to a request object pointers */
    if (incount > MPID_REQUEST_PTR_ARRAY_SIZE)
    {
	MPIU_CHKLMEM_MALLOC_ORJUMP(request_ptrs, MPID_Request **, incount * sizeof(MPID_Request *), mpi_errno, "request pointers");
    }
    
    n_inactive = 0;
    for (i = 0; i < incount; i++)
    {
	if (array_of_requests[i] != MPI_REQUEST_NULL)
	{
	    MPID_Request_get_ptr(array_of_requests[i], request_ptrs[i]);
	    /* Validate object pointers if error checking is enabled */
#           ifdef HAVE_ERROR_CHECKING
	    {
		MPID_BEGIN_ERROR_CHECKS;
		{
		    MPID_Request_valid_ptr( request_ptrs[i], mpi_errno );
		    if (mpi_errno != MPI_SUCCESS)
		    {
			goto fn_fail;
		    }
		    
		}
		MPID_END_ERROR_CHECKS;
	    }
#           endif	    

            /* If one of the requests is an anysource on a communicator that's
             * disabled such communication, convert this operation to a testall
             * instead to prevent getting stuck in the progress engine. */
            if (unlikely(MPIR_CVAR_ENABLE_FT &&
                        MPID_Request_is_anysource(request_ptrs[i]) &&
                        !MPID_Request_is_complete(request_ptrs[i]) &&
                        !MPID_Comm_AS_enabled(request_ptrs[i]->comm))) {
                disabled_anysource = TRUE;
            }
	}
	else
	{
	    n_inactive += 1;
	    request_ptrs[i] = NULL;
	} 
    }

    if (n_inactive == incount)
    {
	*outcount = MPI_UNDEFINED;
	goto fn_exit;
    }

    if (unlikely(disabled_anysource)) {
        mpi_errno = MPI_Testsome(incount, array_of_requests, outcount, array_of_indices, array_of_statuses);
        goto fn_exit;
    }

    /* Bill Gropp says MPI_Waitsome() is expected to try to make
       progress even if some requests have already completed;  
       therefore, we kick the pipes once and then fall into a loop
       checking for completion and waiting for progress. */ 
    mpi_errno = MPID_Progress_test();
    if (mpi_errno != MPI_SUCCESS)
    {
	/* --BEGIN ERROR HANDLING-- */
	goto fn_fail;
	/* --END ERROR HANDLING-- */
    }

    n_active = 0;
    MPID_Progress_start(&progress_state);
    for(;;)
    {
	mpi_errno = MPIR_Grequest_progress_poke(incount, 
			request_ptrs, array_of_statuses);
	if (mpi_errno != MPI_SUCCESS) goto fn_fail;
	for (i = 0; i < incount; i++)
	{
            if (request_ptrs[i] != NULL)
	    {
                if (MPID_Request_is_complete(request_ptrs[i]))
                {
                    status_ptr = (array_of_statuses != MPI_STATUSES_IGNORE) ? &array_of_statuses[n_active] : MPI_STATUS_IGNORE;
                    rc = MPIR_Request_complete(&array_of_requests[i], request_ptrs[i], status_ptr, &active_flag);
                    if (active_flag)
                    {
                        array_of_indices[n_active] = i;
                        n_active += 1;

                        if (rc == MPI_SUCCESS)
                        {
                            request_ptrs[i] = NULL;
                        }
                        else
                        {
                            mpi_errno = MPI_ERR_IN_STATUS;
                            if (status_ptr != MPI_STATUS_IGNORE)
                            {
                                status_ptr->MPI_ERROR = rc;
                            }
                        }
                    }
                    else
                    {
                        request_ptrs[i] = NULL;
                        n_inactive += 1;
                    }
                } else if (unlikely(MPIR_CVAR_ENABLE_FT &&
                            MPID_Request_is_anysource(request_ptrs[i]) &&
                            !MPID_Comm_AS_enabled(request_ptrs[i]->comm)))
                {
                    mpi_errno = MPI_ERR_IN_STATUS;
                    MPIR_ERR_SET(rc, MPIX_ERR_PROC_FAILED_PENDING, "**failure_pending");
                    status_ptr = (array_of_statuses != MPI_STATUSES_IGNORE) ? &array_of_statuses[n_active] : MPI_STATUS_IGNORE;
                    if (status_ptr != MPI_STATUS_IGNORE) status_ptr->MPI_ERROR = rc;
                }
            }
	}

	if (mpi_errno == MPI_ERR_IN_STATUS)
	{
	    if (array_of_statuses != MPI_STATUSES_IGNORE)
	    { 
		for (i = 0; i < n_active; i++)
		{
		    if (request_ptrs[array_of_indices[i]] == NULL)
		    { 
			array_of_statuses[i].MPI_ERROR = MPI_SUCCESS;
		    }
		}
	    }
	    *outcount = n_active;
	    break;
	}
	else if (n_active > 0)
	{
	    *outcount = n_active;
	    break;
	}
	else if (n_inactive == incount)
	{
	    *outcount = MPI_UNDEFINED;
	    break;
	}

	mpi_errno = MPID_Progress_test();
	if (mpi_errno != MPI_SUCCESS)
	{
	    /* --BEGIN ERROR HANDLING-- */
	    MPID_Progress_end(&progress_state);
	    goto fn_fail;
	    /* --END ERROR HANDLING-- */
	}
    /* Avoid blocking other threads since I am inside an infinite loop */
    MPID_THREAD_CS_YIELD(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    }
    MPID_Progress_end(&progress_state);

    /* ... end of body of routine ... */
    
  fn_exit:
    if (incount > MPID_REQUEST_PTR_ARRAY_SIZE)
    {
	MPIU_CHKLMEM_FREEALL();
    }

    MPID_MPI_PT2PT_FUNC_EXIT(MPID_STATE_MPI_WAITSOME);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
    mpi_errno = MPIR_Err_create_code(
	mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, 
	"**mpi_waitsome", "**mpi_waitsome %d %p %p %p %p",
	incount, array_of_requests, outcount, array_of_indices, 
	array_of_statuses);
#endif
    mpi_errno = MPIR_Err_return_comm(NULL, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


/* added by abu naser */
/* Variable nonce */
int MPI_SEC_Waitsome(int count, MPI_Request req[], int *outcount, int array_of_indices[], MPI_Status sta[]){
    
    int mpi_errno = MPI_SUCCESS;
    int  recv_sz=0; 
    int var,i;
    int index;
    unsigned long dec_count;          
       
    mpi_errno=MPI_Waitsome(count, req, outcount, array_of_indices, sta); 
   // mpi_errno=MPI_Waitall(count, req, sta);
    currentReq += *outcount;
   //printf("waitsome count=%d *outcount=%d currentReq=%d\n", count, *outcount, currentReq); fflush(stdout);
    MPI_Datatype datatype = MPI_CHAR;
   
    for(i=0; i< *outcount; i++){
        //MPI_Get_count(&sta[i], datatype, &recv_sz);
        MPI_Get_count(&sta[array_of_indices[i]], datatype, &recv_sz);
       // printf("array_of_indices[%d]=%d recv_sz=%d\n",i,array_of_indices[i], recv_sz);fflush(stdout);

      /*  if(!EVP_AEAD_CTX_open(ctx, bufptr[waitCounter],
                        &dec_count, (recv_sz-12),
                        &Ideciphertext[waitCounter][0], 12,
                         &Ideciphertext[waitCounter][12], (recv_sz-12),
                        NULL, 0)){
            printf("Decryption error: MPI_SEC_Waitsome\n");
            fflush(stdout);
        }
        else{
            printf("MPI_SEC_Waitsome: decrypted %lu bytes\n",dec_count);
            fflush(stdout);
        }*/
         if(recv_sz > 0){
            index =  waitCounter+array_of_indices[i];
            if(!EVP_AEAD_CTX_open(ctx, bufptr[index],
                        &dec_count, (recv_sz-12),
                        &Ideciphertext[index][0], 12,
                         &Ideciphertext[index][12], (recv_sz-12),
                        NULL, 0)){
                printf("Decryption error: MPI_SEC_Waitsome\n");
                fflush(stdout);
            }
        //else{
         //   printf("MPI_SEC_Waitsome: decrypted %lu bytes\n",dec_count);
         //   fflush(stdout);
         //   }
        }
    } 

     if(currentReq == count){
      waitCounter+=count;
      currentReq = 0;
     // printf("Waitcounter = %d\n", waitCounter); fflush(stdout);
      /* Need to test following condition carefullty, req 
         larger than 3000,                                */
        if(waitCounter > (3000-1)){
            waitCounter= waitCounter % 2999;
        }
     }   

    return mpi_errno;
}
/* End of adding */