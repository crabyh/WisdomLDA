//
// Created by Ye Qi on 09/05/2017.
//

#define MASTER 0

#include "dense_model.h"


void DenseTable::SyncWordTopicTable() {
    int *global_word_topic_table_delta = new int[num_words_ * num_topics_];

    MPI_Allreduce(*word_topic_table_delta_, global_word_topic_table_delta, num_words_ * num_topics_, MPI_INT,
                  MPI_SUM, MPI_COMM_WORLD);
    int *global_word_topic_table_delta_ptr = global_word_topic_table_delta;
    for (int w = 0; w < num_words_; w++, global_word_topic_table_delta_ptr += num_topics_) {
        for (int k = 0; k < num_topics_; k++) {
            word_topic_table_[w][k] = word_topic_table_[w][k] + global_word_topic_table_delta_ptr[k] -
                                      word_topic_table_delta_[w][k];
            word_topic_table_delta_[w][k] = 0;
        }
    }

    delete[] global_word_topic_table_delta;
}


void DenseTable::AsyncWordTopicTable(){
    int *global_word_topic_table_delta = new int[num_words_ * num_topics_]();
    int *global_word_topic_table_delta_ptr = global_word_topic_table_delta;

    if (world_rank_ == MASTER) {

        MPI_Status probe_status;
        int *partial_word_topic_table_delta = new int[num_words_ * num_topics_]();
        int *partial_word_topic_table_delta_ptr;

        for (int w = 0; w < num_words_; w++, global_word_topic_table_delta_ptr += num_topics_) {
            for (int k = 0; k < num_topics_; k++) {
                global_word_topic_table_delta_ptr[k] = word_topic_table_delta_[w][k];
//                word_topic_table_[w][k] -= word_topic_table_delta_[w][k];
                word_topic_table_delta_[w][k] = 0;
            }
        }

        for (int i = 1; i < world_size_; i++) {
            DebugPrint("AsyncWordTopicTable() Before Recv " + to_string(i));
            MPI_Recv(partial_word_topic_table_delta, num_words_ * num_topics_, MPI_INT, i, epoch, MPI_COMM_WORLD,
                     &probe_status);
            DebugPrint("AsyncWordTopicTable() After Recv " + to_string(i));

            global_word_topic_table_delta_ptr = global_word_topic_table_delta;
            partial_word_topic_table_delta_ptr = partial_word_topic_table_delta;

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

        global_word_topic_table_delta_ptr = global_word_topic_table_delta;
        for (int w = 0; w < num_words_; w++, global_word_topic_table_delta_ptr += num_topics_) {
            for (int k = 0; k < num_topics_; k++) {
                word_topic_table_[w][k] += global_word_topic_table_delta_ptr[k];
            }
        }

        for (int i = 1; i < world_size_; i++) {
            MPI_Status status;
            MPI_Wait(&send_reqs[i], &status);
        }

        delete[] partial_word_topic_table_delta;
        delete[] send_reqs;

    } else {
        DebugPrint("AsyncWordTopicTable() Before send ");

        MPI_Request send_req;
        MPI_Isend(*word_topic_table_delta_, num_words_ * num_topics_, MPI_INT, MASTER, epoch, MPI_COMM_WORLD,
                  &send_req);
        DebugPrint("AsyncWordTopicTable() After send ");

        word_topic_synced_ = false;
        MPI_Irecv(*word_topic_table_delta_buffer_, num_words_ * num_topics_, MPI_INT, MASTER, epoch, MPI_COMM_WORLD,
                  &word_topic_request_);
        DebugPrint("AsyncWordTopicTable() After recv ");

//        SyncWordTopic();

        // Reset word_topic_table_delta_
        for (int w = 0; w < num_words_; w++) {
            for (int k = 0; k < num_topics_; k++) {
                word_topic_table_[w][k] -= word_topic_table_delta_[w][k];
                word_topic_table_delta_[w][k] = 0;
            }
        }
    }

    delete[] global_word_topic_table_delta;
}


void DenseTable::WordTopicMerge() {
    int *global_word_topic_table_delta_ptr = *word_topic_table_delta_buffer_;
    for (int w = 0; w < num_words_; w++, global_word_topic_table_delta_ptr += num_topics_) {
        for (int k = 0; k < num_topics_; k++) {
            word_topic_table_[w][k] += global_word_topic_table_delta_ptr[k];
        }
    }
}

