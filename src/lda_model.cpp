//
// Created by Ye Qi on 29/04/2017.
//

#include "lda_model.h"
#define MASTER 0


void GlobalTable::Init() {
    if (world_rank_ == MASTER) {
        // TODO: send word_topic_table_, topic_table_ to all workers
    } else {
        // TODO: receive word_topic_table_, topic_table_ from the master
    }
}

void GlobalTable::Sync() {
    for (int i = 0; i < world_size_; i++) {
        MPI_Status status;
        if (world_rank_ == MASTER) {
            MPI_Recv(topic_table_delta_, num_topics_, MPI_INT, i, epoch, MPI_COMM_WORLD, &status);
        } else {
            MPI_Request send_req;
            MPI_Isend(topic_table_delta_, num_topics_, MPI_INT, MASTER, epoch, MPI_COMM_WORLD, &send_req);
            MPI_Wait(&send_req, &status);
        }
    }
}