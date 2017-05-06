make clean
make
mpirun -np 1 ./lda ../20news.csv ../output 60056 18774 20 0.1 0.1 100 25 0
