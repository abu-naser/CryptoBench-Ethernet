#!/bin/bash
rm -f gcm3header.o  gcm3.o  gcm3
g++ -c gcm3header.cpp -o gcm3header.o -I/home/mpiuser/cryptopp -L/home/mpiuser/cryptopp/install/lib -lcryptopp
gcc -c wrapper_time_cryptopp.c -o wrapper_time_cryptopp.o -I/home/mpiuser/cryptopp -L/home/mpiuser/cryptopp/install/lib -lcryptopp
g++ gcm3header.o wrapper_time_cryptopp.o -o wrapper_time_cryptopp -I/home/mpiuser/cryptopp -L/home/mpiuser/cryptopp -lcryptopp
./wrapper_time_cryptopp
