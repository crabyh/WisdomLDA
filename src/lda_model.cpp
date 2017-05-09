//
// Created by Ye Qi on 29/04/2017.
//

#define MASTER 0

#include "lda_model.h"

void GlobalTable::Sync() {
    DebugPrint("Before SyncTopicTable()");
    SyncTopicTable();
    DebugPrint("Before SyncWordTopicTable()");
    SyncWordTopicTable();
}


void GlobalTable::SyncTopicTable() {
    int *global_topic_table_delta = new int[num_topics_]();

    if (world_rank_ == MASTER) {

        MPI_Status probe_status;
        int *partial_topic_table_delta = new int[num_topics_]();
        for (int i = 1; i < world_size_; i++) {
            DebugPrint("SyncTopicTable() Fuckkkk" + to_string(i));

            MPI_Recv(partial_topic_table_delta, num_topics_, MPI_INT, i, epoch, MPI_COMM_WORLD, &probe_status);
            DebugPrint("SyncTopicTable() Fuckkkk" + to_string(i));

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
        DebugPrint("SyncTopicTable() Before send ");

        MPI_Isend(topic_table_delta_, num_topics_, MPI_INT, MASTER, epoch, MPI_COMM_WORLD, &send_req);
        DebugPrint("SyncTopicTable() After send ");

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
            DebugPrint("SyncWordTopicTable() Before Recv " + to_string(i));
            MPI_Recv(partial_word_topic_table_delta, num_words_ * num_topics_, MPI_INT, i, epoch, MPI_COMM_WORLD,
                    &probe_status);
            DebugPrint("SyncWordTopicTable() After Recv " + to_string(i));
            int *global_word_topic_table_delta_ptr = global_word_topic_table_delta;
            int *partial_word_topic_table_delta_ptr = partial_word_topic_table_delta;
            for (int w = 0; w < num_words_; w++, global_word_topic_table_delta_ptr += num_topics_,
                    partial_word_topic_table_delta_ptr += num_topics_) {
                for (int k = 0; k < num_topics_; k++) {
                    global_word_topic_table_delta_ptr[k] += partial_word_topic_table_delta_ptr[k];
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

        delete[] partial_word_topic_table_delta;
        delete[] send_reqs;

    } else {

        DebugPrint("SyncWordTopicTable() Before send ");

        MPI_Request send_req;
        MPI_Isend(*word_topic_table_delta_, num_words_ * num_topics_, MPI_INT, MASTER, epoch, MPI_COMM_WORLD,
                  &send_req);
        DebugPrint("SyncWordTopicTable() After send ");

        MPI_Recv(global_word_topic_table_delta, num_words_ * num_topics_, MPI_INT, MASTER, epoch, MPI_COMM_WORLD,
                 MPI_STATUS_IGNORE);
        DebugPrint("SyncWordTopicTable() After recv ");

        int *global_word_topic_table_delta_ptr = global_word_topic_table_delta;

        for (int w = 0; w < num_words_; w++, global_word_topic_table_delta_ptr += num_topics_) {
            for (int k = 0; k < num_topics_; k++) {
                word_topic_table_[w][k] = word_topic_table_[w][k] + global_word_topic_table_delta_ptr[k] -
                                      word_topic_table_delta_[w][k];
                word_topic_table_delta_[w][k] = 0;
            }
        }


    }

    delete[] global_word_topic_table_delta;
}


