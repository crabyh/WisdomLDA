main: main.cpp
	mpic++ -O3 -std=c++11 -fopenmp -Wall -g -I../include/ -o lda \
	lda_model.cpp lda_worker.cpp main.cpp

clean:
    rm -rf lda *~ *.*~
