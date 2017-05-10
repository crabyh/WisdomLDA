//
// Created by Ye Qi on 29/04/2017.
//

#define MASTER 0
#define MPI_SEND_LIMIT (2 << 20)

#define TOPIC_TABLE 10
#define WORD_TOPIC_TABLE 11

#include "lda_model.h"


void GlobalTable::Sync() {
    DebugPrint("Before SyncTopicTable()");
    SyncTopicTable();
    DebugPrint("Before SyncWordTopicTable()");
    SyncWordTopicTable();
}


void GlobalTable::SyncTopicTable() {
    int *global_topic_table_delta = new int[num_topics_];
    MPI_Allreduce(topic_table_delta_, global_topic_table_delta, num_topics_, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
    for (int k = 0; k < num_topics_; k++) {
        topic_table_[k] = topic_table_[k] + global_topic_table_delta[k] - topic_table_delta_[k];
        topic_table_delta_[k] = 0;
    }
    delete[] global_topic_table_delta;
}

void GlobalTable::SyncWordTopicTable() {
    int *global_word_topic_table_delta = new int[num_words_ * num_topics_];
    MPI_Allreduce(*word_topic_table_delta_, global_word_topic_table_delta, num_words_ * num_topics_, MPI_INT, MPI_SUM,
                  MPI_COMM_WORLD);
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


void GlobalTable::Async() {
    if (world_rank_ != MASTER) {
        while (!word_topic_synced_) {
            DebugPrint("Not synced before next iter!!");
            TestWordTopicSync();
        }
    }
    DebugPrint("Before SyncTopicTable()");
    SyncTopicTable();
    DebugPrint("Before AsyncWordTopicTable()");
    AsyncWordTopicTable();
//    int sum = 0;
//    for (int i = 0; i < num_words_; i++) {
//        for (int j = 0; j < num_topics_; j++) {
//            sum += word_topic_table_[i][j];
//        }
//    }
//    cout << "sum: " << sum << endl;
}

//void GlobalTable::SyncTopicTable() {
//    int *global_topic_table_delta = new int[num_topics_]();
//
//    if (world_rank_ == MASTER) {
//
//        for (int k = 0; k < num_topics_; k++) {
//            global_topic_table_delta[k] = topic_table_delta_[k];
//            topic_table_[k] -= topic_table_delta_[k];
//            topic_table_delta_[k] = 0;
//        }
//
//        MPI_Status probe_status;
//        int *partial_topic_table_delta = new int[num_topics_]();
//        for (int i = 1; i < world_size_; i++) {
//            DebugPrint("SyncTopicTable() Fuckkkk" + to_string(i));
//            MPI_Recv(partial_topic_table_delta, num_topics_, MPI_INT, i, TOPIC_TABLE, MPI_COMM_WORLD, &probe_status);
//            DebugPrint("SyncTopicTable() Fuckkkk" + to_string(i));
//            for (int k = 0; k < num_topics_; k++) {
//                global_topic_table_delta[k] += partial_topic_table_delta[k];
//            }
//        }
//        delete[] partial_topic_table_delta;
//
//        MPI_Request* send_reqs = new MPI_Request[world_size_];
//        for (int i = 1; i < world_size_; i++) {
//            MPI_Isend(global_topic_table_delta, num_topics_, MPI_INT, i, TOPIC_TABLE, MPI_COMM_WORLD, &send_reqs[i]);
//        }
//
//        for (int k = 0; k < num_topics_; k++) {
//            topic_table_[k] += global_topic_table_delta[k];
//        }
//
//        for (int i = 1; i < world_size_; i++) {
//            MPI_Status status;
//            MPI_Wait(&send_reqs[i], &status);
//        }
//        delete[] send_reqs;
//
//    } else {
//
//        MPI_Request send_req;
//        DebugPrint("SyncTopicTable() Before send ");
//
//        MPI_Isend(topic_table_delta_, num_topics_, MPI_INT, MASTER, TOPIC_TABLE, MPI_COMM_WORLD, &send_req);
//        DebugPrint("SyncTopicTable() After send ");
//
//        MPI_Recv(global_topic_table_delta, num_topics_, MPI_INT, MASTER, TOPIC_TABLE, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
//        for (int k = 0; k < num_topics_; k++) {
//            topic_table_[k] = topic_table_[k] + global_topic_table_delta[k] - topic_table_delta_[k];
//            topic_table_delta_[k] = 0;
//        }
//
//    }
//
//    DebugPrint("Topic Table After Sync()");
//    for (int k = 0; k < num_topics_; ++k) {
//        cout << topic_table_[k] << " ";
//    }
//    cout << endl;
//
//    delete[] global_topic_table_delta;
//}


//void GlobalTable::SyncWordTopicTable() {
//    int *global_word_topic_table_delta = new int[num_words_ * num_topics_]();
//    int *global_word_topic_table_delta_ptr = global_word_topic_table_delta;
//
//    int total_elements = num_words_ * num_topics_;
//
//    if (world_rank_ == MASTER) {
//
//        int *partial_word_topic_table_delta = new int[num_words_ * num_topics_]();
//        int *partial_word_topic_table_delta_ptr = partial_word_topic_table_delta;
//
//        for (int w = 0; w < num_words_; w++, global_word_topic_table_delta_ptr += num_topics_) {
//            for (int k = 0; k < num_topics_; k++) {
//                global_word_topic_table_delta_ptr[k] = word_topic_table_delta_[w][k];
//                word_topic_table_[w][k] -= word_topic_table_delta_[w][k];
//                word_topic_table_delta_[w][k] = 0;
//            }
//        }
//
//        for (int i = 1; i < world_size_; i++) {
//
//            int recv_chunk_total = (total_elements + MPI_SEND_LIMIT - 1) / MPI_SEND_LIMIT;
//            DebugPrint("SyncWordTopicTable() recv_chunk_total " + to_string(recv_chunk_total));
//            MPI_Request *recv_reqs = new MPI_Request[recv_chunk_total];
//            for (int recv_chunk = 0; recv_chunk < recv_chunk_total; recv_chunk++,
//                    partial_word_topic_table_delta_ptr += MPI_SEND_LIMIT) {
//                int recv_elements_count = min(MPI_SEND_LIMIT, total_elements - recv_chunk * MPI_SEND_LIMIT);
//                DebugPrint("recv_chunk " + to_string(recv_chunk) + " count " + to_string(recv_elements_count));
//                MPI_Irecv(partial_word_topic_table_delta_ptr, recv_elements_count, MPI_INT, i, WORD_TOPIC_TABLE,
//                          MPI_COMM_WORLD, &recv_reqs[recv_chunk]);
//            }
//
//            for (int recv_chunk = 0; recv_chunk < recv_chunk_total; recv_chunk++) {
//                DebugPrint("Waiting for chunk " + to_string(recv_chunk));
//                MPI_Status status;
//                MPI_Wait(&recv_reqs[i], &status);
//            }
//
//            delete[] recv_reqs;
//
////            MPI_Recv(partial_word_topic_table_delta, num_words_ * num_topics_, MPI_INT, i, epoch, MPI_COMM_WORLD,
////                    &probe_status);
//            DebugPrint("SyncWordTopicTable() After Recv " + to_string(i));
//
//            global_word_topic_table_delta_ptr = global_word_topic_table_delta;
//            partial_word_topic_table_delta_ptr = partial_word_topic_table_delta;
//
//            for (int w = 0; w < num_words_; w++, global_word_topic_table_delta_ptr += num_topics_,
//                                                 partial_word_topic_table_delta_ptr += num_topics_) {
//                for (int k = 0; k < num_topics_; k++) {
//                    global_word_topic_table_delta_ptr[k] += partial_word_topic_table_delta_ptr[k];
//                }
//            }
//
//        }
//
//        MPI_Request* send_reqs = new MPI_Request[world_size_];
//        for (int i = 1; i < world_size_; i++) {
//            MPI_Isend(global_word_topic_table_delta, num_words_ * num_topics_, MPI_INT, i, WORD_TOPIC_TABLE,
//                      MPI_COMM_WORLD, &send_reqs[i]);
//        }
//
//        global_word_topic_table_delta_ptr = global_word_topic_table_delta;
//        for (int w = 0; w < num_words_; w++, global_word_topic_table_delta_ptr += num_topics_) {
//            for (int k = 0; k < num_topics_; k++) {
//                word_topic_table_[w][k] += global_word_topic_table_delta_ptr[k];
//            }
//        }
//
//        for (int i = 1; i < world_size_; i++) {
//            MPI_Status status;
//            MPI_Wait(&send_reqs[i], &status);
//        }
//
//        delete[] partial_word_topic_table_delta;
//        delete[] send_reqs;
//
//    } else {
//
//        DebugPrint("SyncWordTopicTable() Before send ");
//
//        int send_chunk_total = (total_elements + MPI_SEND_LIMIT - 1) / MPI_SEND_LIMIT;
//        MPI_Request *send_reqs = new MPI_Request[send_chunk_total];
//        for (int send_chunk = 0; send_chunk < send_chunk_total; send_chunk++) {
//            DebugPrint("Offset " + to_string(send_chunk));
//            int sum = 0;
//            for (int n = 0; n < min(MPI_SEND_LIMIT, total_elements - send_chunk * MPI_SEND_LIMIT); ++n) {
//                sum += (*word_topic_table_delta_ + send_chunk * MPI_SEND_LIMIT)[n];
//            }
//            DebugPrint(to_string(sum));
//            MPI_Isend((*word_topic_table_delta_ + send_chunk * MPI_SEND_LIMIT),
//                      min(MPI_SEND_LIMIT, total_elements - send_chunk * MPI_SEND_LIMIT), MPI_INT, MASTER,
//                      WORD_TOPIC_TABLE, MPI_COMM_WORLD, &send_reqs[send_chunk]);
//        }
//
////        MPI_Isend(*word_topic_table_delta_, num_words_ * num_topics_, MPI_INT, MASTER, epoch, MPI_COMM_WORLD,
////                  &send_req);
//
//        DebugPrint("SyncWordTopicTable() After send ");
//
//        for (int send_chunk = 0; send_chunk < send_chunk_total; send_chunk++) {
//            MPI_Status status;
//            MPI_Wait(&send_reqs[send_chunk], &status);
//        }
//        delete[] send_reqs;
//
//        MPI_Recv(global_word_topic_table_delta, num_words_ * num_topics_, MPI_INT, MASTER, WORD_TOPIC_TABLE,
//                 MPI_COMM_WORLD, MPI_STATUS_IGNORE);
//        DebugPrint("SyncWordTopicTable() After recv ");
//
//
//        for (int w = 0; w < num_words_; w++, global_word_topic_table_delta_ptr += num_topics_) {
//            for (int k = 0; k < num_topics_; k++) {
//                word_topic_table_[w][k] = word_topic_table_[w][k] + global_word_topic_table_delta_ptr[k] -
//                                          word_topic_table_delta_[w][k];
//                word_topic_table_delta_[w][k] = 0;
//            }
//        }
//
//
//    }
//
//    delete[] global_word_topic_table_delta;
//}


void GlobalTable::AsyncWordTopicTable(){
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

void GlobalTable::TestWordTopicSync() {
    if (word_topic_synced_) return;
    MPI_Test (&word_topic_request_, &word_topic_synced_, MPI_STATUS_IGNORE);
    if (word_topic_synced_) {
        WordTopicMerge();
    }
}

void GlobalTable::SyncWordTopic(){
    MPI_Wait (&word_topic_request_, MPI_STATUS_IGNORE);
    word_topic_synced_ = 1;
    WordTopicMerge();
};

void GlobalTable::WordTopicMerge() {
    int *global_word_topic_table_delta_ptr = *word_topic_table_delta_buffer_;
    for (int w = 0; w < num_words_; w++, global_word_topic_table_delta_ptr += num_topics_) {
        for (int k = 0; k < num_topics_; k++) {
            word_topic_table_[w][k] += global_word_topic_table_delta_ptr[k];
        }
    }
}


