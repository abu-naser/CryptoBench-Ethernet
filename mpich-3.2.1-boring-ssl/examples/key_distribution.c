#include <stdio.h>
#include <mpi.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>

unsigned char symmetric_key[300];
int symmetric_key_size = 16;

int main(){
    int wrank, wsize;
    int mpi_errno ;   
    MPI_Init(NULL, NULL);
    MPI_Comm_rank(MPI_COMM_WORLD, &wrank);
    MPI_Comm_size(MPI_COMM_WORLD, &wsize);


    int keylen, i, ret;
    unsigned char  *root_public_key, *public_key, *all_process_public_key;
    unsigned char  *encrypted_text;
    unsigned char recv_buf[3000];
    unsigned char test[3000];
    int encrypted_len, decrypted_len, pub_key_size, next;
    MPI_Status status;
    BIGNUM *bn;
    BIGNUM *bnPublic = NULL;
    BIGNUM *exponent = NULL;
    BIGNUM *bnPrivate = NULL;

     bnPublic = BN_new();
     exponent = BN_new();
     bnPrivate = BN_new();

    bn = BN_new();
    BN_set_word(bn, RSA_F4);

    RSA *rsaKey, *temprsa, *temprsa_2;
    rsaKey = RSA_new();
    temprsa = RSA_new();
    temprsa_2 = RSA_new();

    /* Generate rsa keypair */
    RSA_generate_key_ex(rsaKey,  2048, bn,  NULL);

    /* Get the public key and exponent */
    RSA_get0_key(rsaKey, &bnPublic, &exponent, &bnPrivate);

  
    all_process_public_key = (unsigned char *)malloc(wsize*256+10);
    encrypted_text = (unsigned char *)malloc(wsize*256+10);

    pub_key_size = BN_num_bytes(bnPublic);
    public_key = (unsigned char *) malloc(256+10);
    ret = BN_bn2bin(bnPublic, public_key);
    
    mpi_errno = MPI_Gather(public_key, 256, MPI_UNSIGNED_CHAR,
               all_process_public_key, 256, MPI_UNSIGNED_CHAR,
               0, MPI_COMM_WORLD);

    
   
    
    if( wrank ==0 ){
        /* Debug: check other publickey */    
         BIGNUM *bnOthPubkey = NULL;
         BIGNUM *bnOthPubkey2 = NULL;

         bnOthPubkey = BN_new();
         bnOthPubkey2 = BN_new();
        /*printf("MPI_Gather done in root process, let's do debug\n");fflush(stdout);
        for(i=0; i<wsize; i++){  
            bnOthPubkey = BN_bin2bn(all_process_public_key+(i*pub_key_size), pub_key_size, NULL );
            printf("[%d]bnOthPubkey of %d:\n",wrank,i);
            BN_print_fp(stdout, bnOthPubkey);
            printf("\n----------------------------------\n");
        }*/
        
        /* Generate a random key */
        //RAND_bytes(symmetric_key, symmetric_key_size);
        for(i=0;i<symmetric_key_size;i++)
            symmetric_key[i] = 'a'+i;
        symmetric_key[symmetric_key_size] = '\0';
        printf("[%d]: symetric key is = %s\n",wrank, symmetric_key);fflush(stdout);

        int next;
        /* Encrypt random key with the public key of other process */
          for(i=1; i<wsize; i++){  
            next = (i*256);
            printf("[%d,1] for %d next %d\n",wrank, i, next); fflush(stdout); 
            bnOthPubkey = BN_bin2bn((unsigned char*)(all_process_public_key+next), 256, NULL );
           // if(i==2)   
            //bnOthPubkey2 = BN_bin2bn((all_process_public_key+next), 256, NULL );
            temprsa = NULL;
            temprsa = RSA_new();
            if(RSA_set0_key(temprsa, bnOthPubkey, exponent, NULL)){ 
                next = i* 256;
               // printf("[%d] for %d next %d\n",wrank, i, next); fflush(stdout);
               //  printf("[%d]bnOthPubkey of %d:\n",wrank,i); fflush(stdout);
               // BN_print_fp(stdout, bnOthPubkey); fflush(stdout);
               // printf("\n----------------------------------\n");
               /* if(i==2){
                  ret = RSA_set0_key(temprsa_2, bnOthPubkey, exponent, NULL);
                  ret = RSA_public_encrypt(16, (unsigned char*)symmetric_key, (unsigned char*)(encrypted_text+next), 
                                        temprsa_2, RSA_PKCS1_PADDING); 
                }*/
                //else{
                    ret = RSA_public_encrypt(16, (unsigned char*)symmetric_key, (unsigned char*)(encrypted_text+next), 
                                        temprsa, RSA_PKCS1_PADDING); 
                //}

                if(i==i){
                     printf("\n[%d]-----------------begin-----------------[%d]\n",i,i); fflush(stdout);
                     printf("key:%s\n",symmetric_key);fflush(stdout);
                //printf("[%d]bnOthPubkey of %d:\n",wrank,i);
                //BN_print_fp(stdout, bnOthPubkey);
                for(int j=next;j<next+256;j++){
                  printf("%02x ",*((unsigned char *)(encrypted_text+j)));
                  fflush(stdout);
                }
                printf("\n[%d]-----------------end-----------------[%d]\n",i,i); fflush(stdout);
            }

                if(ret!=-1){
                    printf("[%d] encrypted %d bytes for %d\n",wrank, ret, i); fflush(stdout);
                }
                else{
                     printf("[%d] encryption failed for for %d ret=%d\n",wrank,  i, ret); 
                     ERR_load_crypto_strings();
                     ERR_error_string(ERR_get_error(), test);
                    fprintf(stderr, "Error decrypting message: %s\n", test);
                     fflush(stdout);   
                }                         
            }
            else{
                printf("RSA_set0_key: Failed in %d for %d\n",wrank, i); fflush(stdout);
            }


        }
        
    }

     mpi_errno = MPI_Scatter(encrypted_text, 256, MPI_UNSIGNED_CHAR, recv_buf, 256, 
                                    MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD);

     if( wrank != 0 ){  

        /* Debug */
        if(wrank == 2){
        printf("[%d]bnPubkey:\n",wrank);
        BN_print_fp(stdout, bnPublic);
        printf("\n----------------------------------\n");
        }

       if(wrank==2){
                 printf("\n[%d]----------------begin------------------[%d]\n",wrank,wrank); fflush(stdout);
                //printf("[%d]bnOthPubkey of %d:\n",wrank,i);
                //BN_print_fp(stdout, bnOthPubkey);
                for(int j=0;j<256;j++){
                  printf("%02x ",recv_buf[j]);
                  fflush(stdout);
                }
                printf("\n[%d]----------------end------------------[%d]\n",wrank,wrank); fflush(stdout);
            }
       

        /* Now decrypt the key */
         ret = RSA_private_decrypt(256, (unsigned char*)recv_buf, (unsigned char*)symmetric_key,
                       rsaKey, RSA_PKCS1_PADDING);

        if(ret!=-1){
            printf("\n[%d] decrypted size is %d\n",wrank, ret); 
            symmetric_key[16] = '\0';
            printf("[%d] symmetric key is: %s\n",wrank, symmetric_key);
            fflush(stdout);
        } 
        else{
                printf("RSA_private_decrypt: Failed in %d ret=%d\n",wrank, ret);
                 ERR_load_crypto_strings();
                     ERR_error_string(ERR_get_error(), test);
                    fprintf(stderr, "Error decrypting message: %s\n", test);
                     
                // RSA_get0_key(rsaKey, &bnPublic, &exponent, &bnPrivate);
               /* if(wrank == 2){
                    printf("[%d]bnPrivate:\n",wrank);
                    BN_print_fp(stdout, bnPrivate);
                    printf("\n----------------------------------\n");
                    fflush(stdout);
                }*/
                fflush(stdout);
                                  

    }
     }
   
    MPI_Barrier(MPI_COMM_WORLD);
    //free(root_public_key);
    free(encrypted_text);
    free(all_process_public_key);
    return 0;
    }