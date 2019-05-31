mpiuser@dhcp16194:~/dockerShared/crypto-throughput$ ./t-cpp 
cryptopp: aes-128 Data size 1  0.147742 MB/s
cryptopp: aes-128 Data size 16  2.423569 MB/s
cryptopp: aes-128 Data size 256  39.729963 MB/s
cryptopp: aes-128 Data size 1024  145.386705 MB/s
cryptopp: aes-128 Data size 4096  396.091057 MB/s
cryptopp: aes-128 Data size 16384  639.687219 MB/s -- best
cryptopp: aes-128 Data size 65536  520.282701 MB/s
cryptopp: aes-128 Data size 262144  552.120141 MB/s
cryptopp: aes-128 Data size 1048576  509.445631 MB/s
cryptopp: aes-128 Data size 2097152  445.326785 MB/s
cryptopp: aes-128 Data size 4194304  465.613836 MB/s
-------------cryptopp 128  done---------------

cryptopp: aes-256 Data size 1  0.159557 MB/s
cryptopp: aes-256 Data size 16  2.418192 MB/s
cryptopp: aes-256 Data size 256  40.942583 MB/s
cryptopp: aes-256 Data size 1024  139.948768 MB/s
cryptopp: aes-256 Data size 4096  362.159281 MB/s
cryptopp: aes-256 Data size 16384  556.941722 MB/s
cryptopp: aes-256 Data size 65536  648.185599 MB/s
cryptopp: aes-256 Data size 262144  689.794083 MB/s -- best
cryptopp: aes-256 Data size 1048576  691.766252 MB/s -- best
cryptopp: aes-256 Data size 2097152  608.124544 MB/s
cryptopp: aes-256 Data size 4194304  428.707268 MB/s
-------------cryptopp 256  done---------------
