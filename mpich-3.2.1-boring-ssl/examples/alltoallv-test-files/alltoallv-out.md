1. int-negative
------------------
rank =0: 1 -9 -19 -29
rank =1: 1 1 -9 -9 -19 -19 -29 -29
rank =2: 1 1 1 -9 -9 -9 -19 -19 -19 -29 -29 -29
rank =3: 1 1 1 1 -9 -9 -9 -9 -19 -19 -19 -19 -29 -29 -29 -29

2. float-negative
-------------------
rank =0: 1.000000 -9.000000 -19.000000 -29.000000
rank =1: 1.000000 1.000000 -9.000000 -9.000000 -19.000000 -19.000000 -29.000000 -29.000000
rank =2: 1.000000 1.000000 1.000000 -9.000000 -9.000000 -9.000000 -19.000000 -19.000000 -19.000000 -29.000000 -29.000000 -29.000000
rank =3: 1.000000 1.000000 1.000000 1.000000 -9.000000 -9.000000 -9.000000 -9.000000 -19.000000 -19.000000 -19.000000 -19.000000 -29.000000 -29.000000 -29.000000 -29.000000

3. alltoallv-example
---------------------
rank =0: 1 11 21 31
rank =1: 11 11 1 1 31 31 21 21 // changed the received index for testing.
rank =2: 1 1 1 11 11 11 21 21 21 31 31 31
rank =3: 1 1 1 1 11 11 11 11 21 21 21 21 31 31 31 31