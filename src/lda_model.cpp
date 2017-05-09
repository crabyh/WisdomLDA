//
// Created by Ye Qi on 29/04/2017.
//

#include "lda_model.h"
#define MASTER 0


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

    if (world_rank_ == MASTER) {

        MPI_Status probe_status;
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
                    if (i == 1) global_word_topic_table_delta_ptr[k] = partial_word_topic_table_delta_ptr[k];
                    else global_word_topic_table_delta_ptr[k] += partial_word_topic_table_delta_ptr[k];
                }
            }
        }

        for (int i = 1; i < world_size_; i++) {
            MPI_Isend(global_word_topic_table_delta, num_words_ * num_topics_, MPI_INT, i, epoch, MPI_COMM_WORLD,
                    &send_reqs[i]);
        }

        for (int i = 1; i < world_size_; i++) {
            MPI_Status status;
            MPI_Wait(&send_reqs[i], &status);
        }


    } else {

//        while(!word_topic_synced) {
//            cout << "Last word_topic_synced not updated before the next Sync!!!" << endl;
//            MPI_Wait (&word_topic_request, MPI_STATUS_IGNORE);
//            word_topic_synced = 1;
//            if (word_topic_synced) {
//                int *global_word_topic_table_delta_ptr = (int *)word_topic_table_delta_buffer;
//                for (int w = 0; w < num_words_; w++, global_word_topic_table_delta_ptr += num_topics_) {
//                    for (int k = 0; k < num_topics_; k++) {
//                        word_topic_table_[w][k] = word_topic_table_[w][k] + global_word_topic_table_delta_ptr[k] -
//                                                  word_topic_table_delta_[w][k];
//                    }
//                }
//            }
//        }
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
//        word_topic_synced = false;
//        MPI_Irecv(word_topic_table_delta_buffer, num_words_ * num_topics_, MPI_INT, MASTER, epoch, MPI_COMM_WORLD,
//                  &word_topic_request);
//        DebugPrint("SyncWordTopicTable() After recv ");
//
//        TestWordTopicSync();
//        // Reset word_topic_table_delta_
//        for (int w = 0; w < num_words_; w++) {
//            for (int k = 0; k < num_topics_; k++) {
//                word_topic_table_delta_[w][k] = 0;
//            }
//        }
    }
}

void GlobalTable::SyncService() {}

void GlobalTable::TestWordTopicSync() {
//    if (word_topic_synced) return;
//    MPI_Wait (&word_topic_request, MPI_STATUS_IGNORE);
//    word_topic_synced = 1;
////    MPI_Test (&word_topic_request, &word_topic_synced, MPI_STATUS_IGNORE);
//    if (word_topic_synced) {
//        int *global_word_topic_table_delta_ptr = (int *)word_topic_table_delta_buffer;
//        for (int w = 0; w < num_words_; w++, global_word_topic_table_delta_ptr += num_topics_) {
//            for (int k = 0; k < num_topics_; k++) {
//                word_topic_table_[w][k] = word_topic_table_[w][k] + global_word_topic_table_delta_ptr[k] -
//                                          word_topic_table_delta_[w][k];
//            }
//        }
//    }
}


