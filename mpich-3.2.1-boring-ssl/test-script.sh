#!/bin/bash
export LD_LIBRARY_PATH=/home/mpiuser/mpich-3.2.1-boring-ssl/mpich-3.2.1_install_boringssl/lib:/home/mpiuser/libsodium-stable/libsodium_install/lib:/home/mpiuser/boringssl/build/crypto

echo "******************************************"
echo "Testing alltoallv...."

cd examples/alltoallv-test-files
rm i-neg f-neg
/home/mpiuser/mpich-3.2.1-boring-ssl/mpich-3.2.1_install_boringssl/bin/MPICH_3.2.1_boringssl_mpicc -o i-neg alltoallv-int-negative.c
/home/mpiuser/mpich-3.2.1-boring-ssl/mpich-3.2.1_install_boringssl/bin/MPICH_3.2.1_boringssl_mpicc -o f-neg  alltoallv-float-negative.c  
ls -lah

/home/mpiuser/mpich-3.2.1-boring-ssl/mpich-3.2.1_install_boringssl/bin/MPICH_3.2.1_boringssl_mpiexec -n 4 ./i-neg 
/home/mpiuser/mpich-3.2.1-boring-ssl/mpich-3.2.1_install_boringssl/bin/MPICH_3.2.1_boringssl_mpiexec -n 4 ./f-neg 

echo "Testing alltoallv done........"
echo "******************************************"
echo "Testing alltoall...."
cd ../all-to-all-test-files
rm i i-neg f-neg
/home/mpiuser/mpich-3.2.1-boring-ssl/mpich-3.2.1_install_boringssl/bin/MPICH_3.2.1_boringssl_mpicc -o i     all-to-all-int.c
/home/mpiuser/mpich-3.2.1-boring-ssl/mpich-3.2.1_install_boringssl/bin/MPICH_3.2.1_boringssl_mpicc -o i-neg all-to-all-int-negative.c
/home/mpiuser/mpich-3.2.1-boring-ssl/mpich-3.2.1_install_boringssl/bin/MPICH_3.2.1_boringssl_mpicc -o f-neg all-to-all-float-negative.c   
ls -lah
/home/mpiuser/mpich-3.2.1-boring-ssl/mpich-3.2.1_install_boringssl/bin/MPICH_3.2.1_boringssl_mpiexec -n 4 ./i
/home/mpiuser/mpich-3.2.1-boring-ssl/mpich-3.2.1_install_boringssl/bin/MPICH_3.2.1_boringssl_mpiexec -n 4 ./i-neg 
/home/mpiuser/mpich-3.2.1-boring-ssl/mpich-3.2.1_install_boringssl/bin/MPICH_3.2.1_boringssl_mpiexec -n 4 ./f-neg 

echo "Testing alltoall done........"
echo "******************************************"
echo "Testing allgather...."
cd ../all-gather-test-files
rm i i-neg f-neg
/home/mpiuser/mpich-3.2.1-boring-ssl/mpich-3.2.1_install_boringssl/bin/MPICH_3.2.1_boringssl_mpicc -o i     all-gather-int.c
/home/mpiuser/mpich-3.2.1-boring-ssl/mpich-3.2.1_install_boringssl/bin/MPICH_3.2.1_boringssl_mpicc -o i-neg all-gather-int-negative.c
/home/mpiuser/mpich-3.2.1-boring-ssl/mpich-3.2.1_install_boringssl/bin/MPICH_3.2.1_boringssl_mpicc -o f-neg all-gather-float-negative.c   
ls -lah

/home/mpiuser/mpich-3.2.1-boring-ssl/mpich-3.2.1_install_boringssl/bin/MPICH_3.2.1_boringssl_mpiexec -n 4 ./i
/home/mpiuser/mpich-3.2.1-boring-ssl/mpich-3.2.1_install_boringssl/bin/MPICH_3.2.1_boringssl_mpiexec -n 4 ./i-neg 
/home/mpiuser/mpich-3.2.1-boring-ssl/mpich-3.2.1_install_boringssl/bin/MPICH_3.2.1_boringssl_mpiexec -n 4 ./f-neg 

echo "Testing allgather done........"
echo "******************************************"
echo "Testing bcast...."
cd ../bcast
rm  i-neg f-neg

/home/mpiuser/mpich-3.2.1-boring-ssl/mpich-3.2.1_install_boringssl/bin/MPICH_3.2.1_boringssl_mpicc -o i-neg  bcast-int-negative.c
/home/mpiuser/mpich-3.2.1-boring-ssl/mpich-3.2.1_install_boringssl/bin/MPICH_3.2.1_boringssl_mpicc -o f-neg bcast-float-negative.c 
ls -lah

/home/mpiuser/mpich-3.2.1-boring-ssl/mpich-3.2.1_install_boringssl/bin/MPICH_3.2.1_boringssl_mpiexec -n 4 ./i-neg 
/home/mpiuser/mpich-3.2.1-boring-ssl/mpich-3.2.1_install_boringssl/bin/MPICH_3.2.1_boringssl_mpiexec -n 4 ./f-neg 

echo "Testing bcast done........"