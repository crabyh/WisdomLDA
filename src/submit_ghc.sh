export PATH=$PATH:/usr/lib64/openmpi/bin
make clean
make ghc
mpirun -np $1 ./lda ../nytimes.data ../output 102660 299752 20 0.1 0.1 100 50000 0 $2
#mpirun -np $1 ./lda ../20news.csv ../output 60056 18774 20 0.1 0.1 100 25 0 1