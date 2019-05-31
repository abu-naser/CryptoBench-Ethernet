/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <strings.h>

#include "mpiimpl.h"
#include "mpi_init.h"


/* Added by Abu Naser */
#include <openssl/rsa.h>
#include <openssl/pem.h>
void MPI_SEC_Initial_Key_Aggrement();
unsigned char symmetric_key[300];
int symmetric_key_size = 16;
/* End of add */
/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

categories:
    - name        : THREADS
      description : multi-threading cvars

cvars:
    - name        : MPIR_CVAR_ASYNC_PROGRESS
      category    : THREADS
      type        : boolean
      default     : false
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        If set to true, MPICH will initiate an additional thread to
        make asynchronous progress on all communication operations
        including point-to-point, collective, one-sided operations and
        I/O.  Setting this variable will automatically increase the
        thread-safety level to MPI_THREAD_MULTIPLE.  While this
        improves the progress semantics, it might cause a small amount
        of performance overhead for regular MPI operations.  The user
        is encouraged to leave one or more hardware threads vacant in
        order to prevent contention between the application threads
        and the progress thread(s).  The impact of oversubscription is
        highly system dependent but may be substantial in some cases,
        hence this recommendation.

    - name        : MPIR_CVAR_DEFAULT_THREAD_LEVEL
      category    : THREADS
      type        : string
      default     : "MPI_THREAD_SINGLE"
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Sets the default thread level to use when using MPI_INIT. This variable
        is case-insensitive.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

/* -- Begin Profiling Symbol Block for routine MPI_Init */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Init = PMPI_Init
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Init  MPI_Init
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Init as PMPI_Init
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Init(int *argc, char ***argv) __attribute__((weak,alias("PMPI_Init")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Init
#define MPI_Init PMPI_Init

/* Fortran logical values. extern'd in mpiimpl.h */
/* MPI_Fint MPIR_F_TRUE, MPIR_F_FALSE; */

/* Any internal routines can go here.  Make them static if possible */

/* must go inside this #ifdef block to prevent duplicate storage on darwin */
int MPIR_async_thread_initialized = 0;
#endif

#undef FUNCNAME
#define FUNCNAME MPI_Init

/*@
   MPI_Init - Initialize the MPI execution environment

Input Parameters:
+  argc - Pointer to the number of arguments 
-  argv - Pointer to the argument vector

Thread and Signal Safety:
This routine must be called by one thread only.  That thread is called
the `main thread` and must be the thread that calls 'MPI_Finalize'.

Notes:
   The MPI standard does not say what a program can do before an 'MPI_INIT' or
   after an 'MPI_FINALIZE'.  In the MPICH implementation, you should do
   as little as possible.  In particular, avoid anything that changes the
   external state of the program, such as opening files, reading standard
   input or writing to standard output.

Notes for C:
    As of MPI-2, 'MPI_Init' will accept NULL as input parameters. Doing so
    will impact the values stored in 'MPI_INFO_ENV'.

Notes for Fortran:
The Fortran binding for 'MPI_Init' has only the error return
.vb
    subroutine MPI_INIT( ierr )
    integer ierr
.ve

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_INIT

.seealso: MPI_Init_thread, MPI_Finalize
@*/
int MPI_Init( int *argc, char ***argv )
{
    static const char FCNAME[] = "MPI_Init";
    int mpi_errno = MPI_SUCCESS;
    int rc ATTRIBUTE((unused));
    int threadLevel, provided;
    MPID_MPI_INIT_STATE_DECL(MPID_STATE_MPI_INIT);

    rc = MPID_Wtime_init();
#ifdef USE_DBG_LOGGING
    MPIU_DBG_PreInit( argc, argv, rc );
#endif

    MPID_MPI_INIT_FUNC_ENTER(MPID_STATE_MPI_INIT);
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            if (OPA_load_int(&MPIR_Process.mpich_state) != MPICH_PRE_INIT) {
                mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
						  "**inittwice", NULL );
	    }
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ... */

    /* Temporarily disable thread-safety.  This is needed because the
     * mutexes are not initialized yet, and we don't want to
     * accidentally use them before they are initialized.  We will
     * reset this value once it is properly initialized. */
#if defined MPICH_IS_THREADED
    MPIR_ThreadInfo.isThreaded = 0;
#endif /* MPICH_IS_THREADED */

    MPIR_T_env_init();

    if (!strcasecmp(MPIR_CVAR_DEFAULT_THREAD_LEVEL, "MPI_THREAD_MULTIPLE"))
        threadLevel = MPI_THREAD_MULTIPLE;
    else if (!strcasecmp(MPIR_CVAR_DEFAULT_THREAD_LEVEL, "MPI_THREAD_SERIALIZED"))
        threadLevel = MPI_THREAD_SERIALIZED;
    else if (!strcasecmp(MPIR_CVAR_DEFAULT_THREAD_LEVEL, "MPI_THREAD_FUNNELED"))
        threadLevel = MPI_THREAD_FUNNELED;
    else if (!strcasecmp(MPIR_CVAR_DEFAULT_THREAD_LEVEL, "MPI_THREAD_SINGLE"))
        threadLevel = MPI_THREAD_SINGLE;
    else {
        MPL_error_printf("Unrecognized thread level %s\n", MPIR_CVAR_DEFAULT_THREAD_LEVEL);
        exit(1);
    }

    /* If the user requested for asynchronous progress, request for
     * THREAD_MULTIPLE. */
    if (MPIR_CVAR_ASYNC_PROGRESS)
        threadLevel = MPI_THREAD_MULTIPLE;

    mpi_errno = MPIR_Init_thread( argc, argv, threadLevel, &provided );
    if (mpi_errno != MPI_SUCCESS) goto fn_fail;

    if (MPIR_CVAR_ASYNC_PROGRESS) {
        if (provided == MPI_THREAD_MULTIPLE) {
            mpi_errno = MPIR_Init_async_thread();
            if (mpi_errno) goto fn_fail;

            MPIR_async_thread_initialized = 1;
        }
        else {
            printf("WARNING: No MPI_THREAD_MULTIPLE support (needed for async progress)\n");
        }
    }

   

    /* ... end of body of routine ... */
    MPID_MPI_INIT_FUNC_EXIT(MPID_STATE_MPI_INIT);
    
    /* Added by Abu Naser(an16e@my.fsu.edu) */
    printf("ENABLE_SECURE_MPI =%d\n",ENABLE_SECURE_MPI);
 #if ENABLE_SECURE_MPI   
   // MPI_SEC_Initial_Key_Aggrement();
#endif
   /* End of add */
    
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_REPORTING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, 
	    "**mpi_init", "**mpi_init %p %p", argc, argv);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
    return mpi_errno;
    /* --END ERROR HANDLING-- */
}

    /* Added by Abu Naser(an16e@my.fsu.edu) */
    
#if ENABLE_SECURE_MPI   
void MPI_SEC_Initial_Key_Aggrement(){
    printf("key size from macro = %d\n",SYMMETRIC_KEY_SIZE);
    int wrank, wsize;
    int mpi_errno = MPI_SUCCESS;   
    MPI_Comm_rank(MPI_COMM_WORLD, &wrank);
    MPI_Comm_size(MPI_COMM_WORLD, &wsize);

   
    int keylen, i, ret;
    unsigned char  *root_public_key, *public_key, *all_process_public_key;
    unsigned char  *encrypted_text;
    unsigned char recv_buf[3000];
    int encrypted_len, decrypted_len, pub_key_size, next;
    MPI_Status status;
    BIGNUM *bn;
    BIGNUM *bnPublic = BN_new();
    BIGNUM *exponent = BN_new();
    BIGNUM *bnPrivate = BN_new();
 

    bn = BN_new();
    BN_set_word(bn, RSA_F4);

    RSA *rsaKey, *temprsa;
    rsaKey = RSA_new();
    temprsa = RSA_new();

    /* Generate rsa keypair */
    RSA_generate_key_ex(rsaKey,  2048, bn,  NULL);

    /* Get the public key and exponent */
    RSA_get0_key(rsaKey, &bnPublic, &exponent, &bnPrivate);

   
    all_process_public_key = (unsigned char *)MPIU_Malloc(wsize*256+10);
    encrypted_text = (unsigned char *)MPIU_Malloc(wsize*256+10);

    pub_key_size = BN_num_bytes(bnPublic);
    public_key = (unsigned char *) malloc(256+10);
    ret = BN_bn2bin(bnPublic, public_key);

    /* send the public key to root process */ 
    mpi_errno = MPI_Gather(public_key, 256, MPI_UNSIGNED_CHAR,
               all_process_public_key, 256, MPI_UNSIGNED_CHAR,
               0, MPI_COMM_WORLD);

    
    if( wrank ==0 ){  
         BIGNUM *bnOthPubkey = BN_new();
        /* Generate a random key */
        RAND_bytes(symmetric_key, symmetric_key_size);
        //for(i=0;i<symmetric_key_size;i++)
        //    symmetric_key[i] = 'a'+i;
        symmetric_key[symmetric_key_size] = '\0';
        //printf("[%d]: symetric key is = %s\n",wrank, symmetric_key);fflush(stdout);

        int next;
        /* Encrypt random key with the public key of other process */
          for(i=1; i<wsize; i++){  
            next = (i*256);
            //printf("[%d,1] for %d next %d\n",wrank, i, next); fflush(stdout); 
            bnOthPubkey = BN_bin2bn((all_process_public_key+next), 256, NULL );
          
            temprsa = NULL;
            temprsa = RSA_new();
            if(RSA_set0_key(temprsa, bnOthPubkey, exponent, NULL)){  
                next = i* 256;
               // printf("[%d] for %d next %d\n",wrank, i, next); fflush(stdout);  
                ret = RSA_public_encrypt(16, (unsigned char*)symmetric_key, (unsigned char*)(encrypted_text+next), 
                                        temprsa, RSA_PKCS1_OAEP_PADDING); 

                if(ret!=-1){
                    printf("[%d] encrypted %d bytes for %d\n",wrank, ret, i); fflush(stdout);
                }
                else{
                     printf("[%d] encryption failed for for %d\n",wrank,  i); fflush(stdout);   
                }                         
            }
            else{
                printf("RSA_set0_key: Failed in %d for %d\n",wrank, i); fflush(stdout);
            }


        }
        
    }
   
    /* send/recv encrypted symmetric key from/to processes */
    mpi_errno = MPI_Scatter(encrypted_text, 256, MPI_UNSIGNED_CHAR, recv_buf, 256, 
                                    MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD);

     if( wrank != 0 ){
       //  pub_key_size = BN_num_bytes(bnPublic);
       //  root_public_key = (unsigned char *) MPIU_Malloc(pub_key_size+10);

         /* Receive the public key of the root process */
        // mpi_errno = MPI_Bcast( root_public_key, pub_key_size, MPI_CHAR, 0, 
        //       MPI_COMM_WORLD );

        /* Debug: check root publickey */    
       /* BIGNUM *bnRootPubkey = NULL;  
        bnRootPubkey = BN_bin2bn(root_public_key, pub_key_size, NULL );
        printf("[%d]bnRootPubkey:\n",wrank);
        BN_print_fp(stdout, bnRootPubkey);
        printf("\n----------------------------------\n");*/

        /* send own public key to root */
        

        /* Now decrypt the key */
         ret = RSA_private_decrypt(256, (unsigned char*)recv_buf, (unsigned char*)symmetric_key,
                       rsaKey, RSA_PKCS1_OAEP_PADDING);

        if(ret!=-1){
            printf("[%d] decrypted size is %d\n",wrank, ret); 
            symmetric_key[16] = '\0';
            printf("[%d] symmetric key is: %s\n",wrank, symmetric_key);
            fflush(stdout);
        } 
        else{
                printf("RSA_private_decrypt: Failed in %d\n",wrank);
                // RSA_get0_key(rsaKey, &bnPublic, &exponent, &bnPrivate);
               /* if(wrank == 2){
                    printf("[%d]bnPrivate:\n",wrank);
                    BN_print_fp(stdout, bnPrivate);
                    printf("\n----------------------------------\n");
                    fflush(stdout);
                }*/
                fflush(stdout);
            }              
        /*if(!RSA_private_decrypt(pub_key_size, (unsigned char*)recv_buf, symmetric_key,
                       rsaKey, RSA_PKCS1_OAEP_PADDING))
            {
                printf("RSA_private_decrypt: Failed in %d\n",wrank);fflush(stdout);
            }
            else{
                symmetric_key[16] = '\0';
                printf("[%d] symmetric key is: %s\n",wrank, symmetric_key);
                fflush(stdout);
            }  */                              

    }
  
    MPI_Barrier(MPI_COMM_WORLD);
    MPIU_Free(root_public_key);
    MPIU_Free(encrypted_text);
    MPIU_Free(all_process_public_key);
    return;
}
#endif
/* End of add */