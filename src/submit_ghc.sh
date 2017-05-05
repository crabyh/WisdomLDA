make clean
make
mpirun -np 4 ./lda ../20news.csv ../output 60056 18773 20 0.1 0.1 100 25 0
