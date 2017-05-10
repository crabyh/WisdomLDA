#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <string>
#include <iostream>

#include "lda_worker.h"

#define MASTER 0

void print_usage() {
    std::cerr << "Usage: ./lda <data_file> <output_dir> <num_words> <num_docs> <num_topics> <alpha> <beta> "
            "<num_iters> <num_clocks_per_iter> <staleness>\n";
}

int main(int argc, char **argv) {

    // Initialize the MPI environment
    MPI_Init(&argc, &argv);

    // Get the number of processes
    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    // Get the rank of the process
    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    // Check arguments
    if (argc != 11) {
        if (world_rank == MASTER) {
            print_usage();
        }
        MPI_Finalize();
        exit(1);
    }

    string data_file = argv[1];
    string output_dir = argv[2];
    int num_words = atoi(argv[3]) + 1;
//    int num_docs = atoi(argv[4]) / world_size + ((atoi(argv[4]) % world_size) > world_rank ? 1 : 0);
    // Set num_docs equals to number of documents belongs to that worker.
    int num_docs = atoi(argv[4]) / (world_size - 1) + ((atoi(argv[4]) % (world_size - 1)) > (world_rank + 1) ? 1 : 0);
    if (world_rank == MASTER) num_docs = 0;
    int num_topics = atoi(argv[5]);
    double alpha = atof(argv[6]);
    double beta = atof(argv[7]);
    int num_iters = atoi(argv[8]);
    int num_clocks_per_iter = atoi(argv[9]);
    int staleness = atoi(argv[10]);

    if (world_rank == MASTER) {
        std::cout << num_docs << std::endl;
    }

    LdaWorker lda_worker(world_size, world_rank, data_file, output_dir,
                         num_words, num_docs, num_topics, alpha, beta,
                         num_iters, num_clocks_per_iter, staleness);

    if (world_rank == MASTER) {
        std::cout << "Number of processes = " << world_size << std::endl;
//        std::cout << "Starting loading document collection" << std::endl;
    }
    std::cout << world_rank << ": Hello world!" << std::endl;

//    if (world_rank == MASTER)
//        std::cout << "Done!\n";

    lda_worker.Setup();

    double time_sol = 0.0;

    std::cout << world_rank << ": Before Barrier!" << std::endl;
    MPI_Barrier(MPI_COMM_WORLD);
    std::cout << world_rank << ": After Barrier!" << std::endl;

    double start_time = MPI_Wtime();
    lda_worker.Run();
    MPI_Barrier(MPI_COMM_WORLD);
    double end_time = MPI_Wtime();
    time_sol = (end_time - start_time);

    if (world_rank == MASTER) {
        std::cout << std::setiosflags(std::ios::fixed) << std::setprecision(5);
        std::cout << time_sol << "sec" << std::endl;
    }

    // Finalize the MPI environment.
    MPI_Finalize();
    return 0;
}

