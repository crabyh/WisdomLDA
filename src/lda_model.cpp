//
// Created by Ye Qi on 29/04/2017.
//

#include "lda_model.h"
#define MASTER 0


void GlobalTable::Sync() {
//    cout << world_rank_ << ": Before SyncTopicTable()" << endl;
    SyncTopicTable();
//    cout << world_rank_ << ": Before SyncWordTopicTable()" << endl;
    SyncWordTopicTable();
}


void GlobalTable::SyncTopicTable() {
    int *global_topic_table_delta = new int[num_topics_]();

    if (world_rank_ == MASTER) {

        MPI_Status probe_status;
        int *partial_topic_table_delta = new int[num_topics_]();
        for (int i = 1; i < world_size_; i++) {
            cout << world_rank_ << ": SyncTopicTable() Fuckkkk" << i << endl;

            MPI_Recv(partial_topic_table_delta, num_topics_, MPI_INT, i, epoch, MPI_COMM_WORLD, &probe_status);
            cout << world_rank_ << ": SyncTopicTable() Fuckkkk" << i << endl;

            for (int k = 0; k < num_topics_; k++) {
                global_topic_table_delta[k] += partial_topic_table_delta[k];
            }
        }
        delete[] partial_topic_table_delta;


        MPI_Request* send_reqs = new MPI_Request[world_size_];
        for (int i = 1; i < world_size_; i++) {
            MPI_Isend(global_topic_table_delta, num_topics_, MPI_INT, i, epoch, MPI_COMM_WORLD, &send_reqs[i]);
        }
        for (int i = 1; i < world_size_; i++) {
            MPI_Status status;
            MPI_Wait(&send_reqs[i], &status);
        }
        delete[] send_reqs;

    } else {

        MPI_Request send_req;
        cout << world_rank_ << ": SyncTopicTable() Before send " << endl;

        MPI_Isend(topic_table_delta_, num_topics_, MPI_INT, MASTER, epoch, MPI_COMM_WORLD, &send_req);
        cout << world_rank_ << ": SyncTopicTable() After send " << endl;

        MPI_Recv(global_topic_table_delta, num_topics_, MPI_INT, MASTER, epoch, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        for (int k = 0; k < num_topics_; k++) {
            topic_table_[k] = topic_table_[k] + global_topic_table_delta[k] - topic_table_delta_[k];
            topic_table_delta_[k] = 0;
        }

    }

    delete[] global_topic_table_delta;
}

void GlobalTable::SyncWordTopicTable() {
    int *global_word_topic_table_delta = new int[num_words_ * num_topics_]();

    if (world_rank_ == MASTER) {

        MPI_Status probe_status;
        int *partial_word_topic_table_delta = new int[num_words_ * num_topics_]();
        for (int i = 1; i < world_size_; i++) {
            MPI_Recv(partial_word_topic_table_delta, num_words_ * num_topics_, MPI_INT, i, epoch, MPI_COMM_WORLD,
                    &probe_status);
            for (int w = 0; w < num_words_; w++) {
                for (int k = 0; k < num_topics_; k++) {
                    (global_word_topic_table_delta + w)[k] += (partial_word_topic_table_delta + w)[k];
                }
            }
        }

        MPI_Request* send_reqs = new MPI_Request[world_size_];
        for (int i = 1; i < world_size_; i++) {
            MPI_Isend(global_word_topic_table_delta, num_words_ * num_topics_, MPI_INT, i, epoch, MPI_COMM_WORLD,
                    &send_reqs[i]);
        }
        for (int i = 1; i < world_size_; i++) {
            MPI_Status status;
            MPI_Wait(&send_reqs[i], &status);
        }
        delete[] send_reqs;

    } else {

        MPI_Request send_req;
        MPI_Isend(word_topic_table_delta_, num_words_ * num_topics_, MPI_INT, MASTER, epoch, MPI_COMM_WORLD, &send_req);
        MPI_Recv(global_word_topic_table_delta, num_words_ * num_topics_, MPI_INT, MASTER, epoch, MPI_COMM_WORLD,
                 MPI_STATUS_IGNORE);
        for (int w_k = 0; w_k < num_words_ * num_topics_; w_k++) {
            word_topic_table_[w_k] = word_topic_table_[w_k] + global_word_topic_table_delta[w_k] -
                    topic_table_delta_[w_k];
            topic_table_delta_[w_k] = 0;
        }

    }

    delete[] global_word_topic_table_delta;
}


