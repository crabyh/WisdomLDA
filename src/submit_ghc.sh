export PATH=$PATH:/usr/lib64/openmpi/bin
make clean
make ghc
mpirun -np 5 ./lda ../20news.csv ../output 60056 18774 20 0.1 0.1 100 25 0
