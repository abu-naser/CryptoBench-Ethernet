#!/bin/bash
sudo rm -rf install
make clean
make libcryptopp.a libcryptopp.so cryptest.exe
mkdir install
sudo make install PREFIX=/home/mpiuser/cryptopp/install
sudo cp install/lib/* /usr/local/lib
