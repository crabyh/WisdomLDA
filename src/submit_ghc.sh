make clean
make ghc
mpirun -np 3 ./lda ../20news.csv ../output 60056 18774 20 0.1 0.1 100 25 0
