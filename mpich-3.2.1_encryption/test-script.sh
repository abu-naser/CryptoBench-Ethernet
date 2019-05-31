#!/bin/bash
MPICC="/home/mpiuser/mpich-3.2.1_encryption/mpich-3.2.1_encryption_install/bin/MPICH3.2.1_ENCRYPTION_mpicc"
MPIEXEC="/home/mpiuser/mpich-3.2.1_encryption/mpich-3.2.1_encryption_install/bin/MPICH3.2.1_ENCRYPTION_mpiexec" 
export LD_LIBRARY_PATH=/home/mpiuser/mpich-3.2.1_encryption/mpich-3.2.1_encryption_install/lib:/home/mpiuser/libsodium-stable/libsodium_install/lib
printf "\n"
echo "******************************************"
echo "Testing alltoallv...."
printf "\n"

cd examples/alltoallv-test-files
rm i-neg f-neg
$MPICC -o i-neg alltoallv-int-negative.c
$MPICC  -o f-neg  alltoallv-float-negative.c  
ls -lah

$MPIEXEC -n 4 ./i-neg 
$MPIEXEC -n 4 ./f-neg 

echo "Testing alltoallv done........"
echo "******************************************"
printf "\n"
echo "Testing alltoall...."
printf "\n"
cd ../all-to-all-test-files
rm i i-neg f-neg
$MPICC  -o i     all-to-all-int.c
$MPICC  -o i-neg all-to-all-int-negative.c
$MPICC  -o f-neg all-to-all-float-negative.c   
ls -lah
$MPIEXEC -n 4 ./i
$MPIEXEC -n 4 ./i-neg 
$MPIEXEC -n 4 ./f-neg 

echo "Testing alltoall done........"
echo "******************************************"
printf "\n"
echo "Testing allgather...."
printf "\n"
cd ../all-gather-test-files
rm i i-neg f-neg
$MPICC  -o i     all-gather-int.c
$MPICC  -o i-neg all-gather-int-negative.c
$MPICC  -o f-neg all-gather-float-negative.c   
ls -lah

$MPIEXEC -n 4 ./i
$MPIEXEC -n 4 ./i-neg 
$MPIEXEC -n 4 ./f-neg 

echo "\n\n"
echo "Testing allgather done........"
echo "******************************************"
printf "\n"
echo "Testing bcast...."
printf "\n"
cd ../bcast
rm  i-neg f-neg

$MPICC -o i-neg  bcast-int-negative.c
$MPICC -o f-neg bcast-float-negative.c 
ls -lah

$MPIEXEC -n 4 ./i-neg 
$MPIEXEC -n 4 ./f-neg 

echo "Testing bcast done........"