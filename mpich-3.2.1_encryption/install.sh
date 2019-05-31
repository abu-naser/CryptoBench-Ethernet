make clean
make V=1 2>&1 | tee m.txt && make install 2>&1 | tee mi.txt
