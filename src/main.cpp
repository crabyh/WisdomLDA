#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <string>
#include <iostream>

#include "lda_worker.h"

#define MASTER 0
#define SILENT "silent"
#define NUM_RUNS 5

void print_usage() {
    std::cerr << "Usage: ./lda <data_file> <output_dir> <num_words> "
            "<num_topics> <alpha> <beta> <num_iters> <num_clocks_per_iter> "
            "<staleness>\n";
}

int main(int argc, char **argv) {

    // Initialize the MPI environment
    MPI_Init(NULL, NULL);

    // Get the number of processes
    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    // Get the rank of the process
    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    // Check arguments
    if (argc != 10) {
        if (world_rank == MASTER) {
            print_usage();
        }
        MPI_Finalize();
        exit(1);
    }

    string data_file = argv[1];
    string output_dir = argv[2];
    int num_words = atoi(argv[3]) + 1;
    int num_topics = atoi(argv[4]);
    double alpha = atof(argv[5]);
    double beta = atof(argv[6]);
    int num_iters = atoi(argv[7]);
    int num_clocks_per_iter = atoi(argv[8]);
    int staleness = atoi(argv[9]);

    LdaWorker lda_worker(world_size, world_rank, data_file, output_dir,
                         num_words, num_topics, alpha, beta,
                         num_iters, num_clocks_per_iter, staleness);

    if (world_rank == MASTER) {
        std::cout << "Number of processes = " << world_size << std::endl;
        std::cout << "Starting loading document collection" << std::endl;
    }
    std::cout << world_rank << ": Hello world!" << std::endl;

    if (world_rank == MASTER)
        std::cout << "Done!\n";

    lda_worker.setup();

    double time_sol = 0.0;

    MPI_Barrier(MPI_COMM_WORLD);
    double start_time = MPI_Wtime();
    lda_worker.run();
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

