rm -f gcm3header.o  gcm3.o  gcm3
g++ -c gcm3header.cpp -o gcm3header.o -I/home/mpiuser/cryptopp -L/home/mpiuser/cryptopp/install/lib -lcryptopp
gcc -c gcm3.c -o gcm3.o -I/home/mpiuser/cryptopp -L/home/mpiuser/cryptopp/install/lib -lcryptopp
g++ gcm3header.o gcm3.o -o gcm3 -I/home/mpiuser/cryptopp -L/home/mpiuser/cryptopp -lcryptopp
./gcm3

