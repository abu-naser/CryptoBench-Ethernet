#include "gcm3header.h"
#include <stdlib.h>
#include <iostream>
#include <string>
#include <iomanip>
#include <fstream>
#include <ctime>
#include <vector>
#include <sstream>
#include <utility>
#include <stdio.h>
#include <string.h>
#include "aes.h"
#include "gcm.h"
#include "assert.h"
#include "cryptlib.h"
#include "hex.h"
#include "filters.h"
#include "osrng.h"

//#define key_size 16


extern "C" {

using CryptoPP::Exception;
using CryptoPP::HexEncoder;
using CryptoPP::HexDecoder;
using CryptoPP::StringSink;
using CryptoPP::StringSource;
using CryptoPP::AuthenticatedEncryptionFilter;
using CryptoPP::AuthenticatedDecryptionFilter;
using CryptoPP::AES;
using std::setw;
using namespace std;
using namespace CryptoPP;
using CryptoPP::AutoSeededRandomPool;

void my_print_recv(const void *print_buffer, int count,char s[])	//keep square brackets in signature
{ 
	printf("\n%s=\n",s);fflush(stdout);
	for (int i=0 ; i<count ; i++){
		printf("%2x ",*((unsigned char *)(print_buffer+i)));
		fflush(stdout);
	}
	printf("\n");fflush(stdout);
}


const byte key[32]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
const byte iv[12]= {0,0,0,0,0,0,0,0,0,0,0,0}; 
static const int TAG_SIZE = 12;
static int key_size=16;

void key_cryptopp_128(){
	key_size = 16;
}

void key_cryptopp_256(){
	key_size = 32;
}

int encrypt(unsigned char *buf , unsigned char *ciphertext , int count){
	
	string cipher;
	//string pdata=string(buf,count);
	//*((unsigned char *)buf+count)='\0';
	//printf("\nencrypt 1");
	//*(buf+count)='\0';
	//buf[count]='\0';
 
	try
	{
		GCM< AES >::Encryption e;
		//e.SetKeyWithIV(key, sizeof(key), iv, sizeof(iv));		
		e.SetKeyWithIV(key, key_size, iv, sizeof(iv));		
		//printf("\nencrypt 2");
		StringSource ss1( buf, true,
		//StringSource( pdata, true,
            new AuthenticatedEncryptionFilter( e,
                new StringSink( cipher ), false, TAG_SIZE
            ) // AuthenticatedEncryptionFilter
        ); // StringSource
			//printf("\nencrypt 3");
		count=count+TAG_SIZE;		
		//printf("\nencrypt 4 en count=%d strlen(cipher.c_str())=%d\n",count, strlen(cipher.c_str()));
		//memcpy (ciphertext, cipher.c_str(),count); //strlen(from.c_str())+1
		memcpy (ciphertext, cipher.c_str(),strlen(cipher.c_str())+1);
	   // printf("\nencrypt 4 en2 count=%d\n",count);		
		//printf("\nencrypt 5");	
	}
		
		catch( CryptoPP::InvalidArgument& e )
		{
			cerr << "Caught InvalidArgument..." << endl;
			cerr << e.what() << endl;
			cerr << endl;
		}
		 catch( CryptoPP::Exception& e )
		{
			cerr << "Caught Exception..." << endl;
			cerr << e.what() << endl;
			cerr << endl;
		}
		//printf("\nencrypt 6");
	return count;	
}

int decrypt(unsigned char *ciphertext , unsigned char *buf, int count){
	

string rpdata;
//string cipher=string(ciphertext,count);
//printf("\ndecrypt 1");
//*(ciphertext+count)='\0';
//printf("\ndecrypt 2");
//ciphertext[count]='\0';
 	
	try
	{
		GCM< AES >::Decryption d;
		//d.SetKeyWithIV(key, sizeof(key), iv, sizeof(iv));
		d.SetKeyWithIV(key, key_size, iv, sizeof(iv));
		//printf("\ndecrypt 3");
		AuthenticatedDecryptionFilter df( d,
            new StringSink( rpdata ),
            AuthenticatedDecryptionFilter::DEFAULT_FLAGS,
            TAG_SIZE
        );
		//printf("\nThis is count @ decrypt:%d\n",count);
		
		StringSource ss2 ( ciphertext, true,
		//StringSource( cipher, true,
            new Redirector( df /*, PASS_EVERYTHING */ )
        ); // StringSource
	//printf("\ndecrypt 5");
		bool b = false;
		b = df.GetLastResult();
		assert( true == b );
	
		count=count-TAG_SIZE;
		//printf("\nencrypt 4 decrypt count=%d rpdata=%d\n",count,rpdata.length());
		//std::cout << "The size of str is " << rpdata.length() << " bytes.\n";
		//printf("\ndecrypt 6");
		//memcpy (buf, rpdata.c_str(),count);
	    strcpy((char*)buf, rpdata.c_str());
		//printf("\ndecrypt 7");		
											
	}
	catch( CryptoPP::HashVerificationFilter::HashVerificationFailed& e )
	{
		cerr << "Caught HashVerificationFailed..." << endl;
		cerr << e.what() << endl;
		cerr << endl;
	}
	catch( CryptoPP::InvalidArgument& e )
	{
		cerr << "Caught InvalidArgument..." << endl;
		cerr << e.what() << endl;
		cerr << endl;
	}
	catch( CryptoPP::Exception& e )
	{
		cerr << "Caught Exception..." << endl;
		cerr << e.what() << endl;
		cerr << endl;
	}		
	return count;
	}
}
