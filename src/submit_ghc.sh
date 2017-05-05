make clean
make
mpirun -np 8 ./lda ../20news.csv ../output 18773 60056 20 0.1 0.1 100 25 0
