cmake_minimum_required(VERSION 3.7)
project(WisdomLDA)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES
        src/main.cpp
        src/lda_worker.cpp
        src/lda_worker.h)

add_executable(WisdomLDA ${SOURCE_FILES})