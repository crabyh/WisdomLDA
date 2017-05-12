export PATH=$PATH:/usr/lib64/openmpi/bin
make clean
make ghc
mpirun -np $1 ./lda ../nytimes.data ../output 102660 299752 20 0.1 0.1 100 10000 0 0
