make clean
make ghc
mpirun -np 4 ./lda ../20news.csv ../output 60056 18774 20 0.1 0.1 100 10 0
