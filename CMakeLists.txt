cmake_minimum_required(VERSION 3.7)
project(WisdomLDA)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES
        src/main.cpp
        src/lda_worker.cpp
        src/lda_worker.h
        src/lda_model.h
        src/lda_model.cpp)

#SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}" -fopenmp)

add_executable(WisdomLDA ${SOURCE_FILES})

find_package(MPI)

include_directories(SYSTEM ${MPI_INCLUDE_PATH})

target_link_libraries(WisdomLDA ${MPI_C_LIBRARIES})

