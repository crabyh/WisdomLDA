//
// Created by Ye Qi on 10/05/2017.
//

#include "sparse_model.h"


#define MASTER 0
#define MPI_SEND_LIMIT (1 << 20)

#define TOPIC_TABLE 10
#define WORD_TOPIC_TABLE 11

#include "sparse_model.h"


void SparseModel::Sync() {
    DebugPrint("Before SyncTopicTable()");
//    SyncTopicTable();
    DebugPrint("Before SyncWordTopicTable()");
    SyncWordTopicTable();
}


void SparseModel::SyncTopicTable() {
    int *global_topic_table_delta = new int[num_topics_];
    MPI_Allreduce(topic_table_delta_, global_topic_table_delta, num_topics_, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
    for (int k = 0; k < num_topics_; k++) {
        topic_table_[k] = topic_table_[k] + global_topic_table_delta[k] - topic_table_delta_[k];
        topic_table_delta_[k] = 0;
    }

//    DebugPrint("Topic Table After Sync()");
//    for (int k = 0; k < num_topics_; ++k) {
//        cout << topic_table_[k] << " ";
//    }
//    cout << endl;

    delete[] global_topic_table_delta;
}

void SparseModel::SyncWordTopicTable() {

    if (world_rank_ != MASTER) {

        MPI_Request send_reqs;
        int send_size = (int) word_topic_table_delta_.size() * 2;
        int *send_buf = new int[send_size];
        int cur_idx = 0;
        int sum = 0;
        for (auto const &item : word_topic_table_delta_) {
            send_buf[cur_idx] = item.first;
            send_buf[cur_idx+1] = item.second;
            word_topic_table_[item.first >> TOPIC_WIDTH][item.first & TOPIC_MASK] -= item.second;
            sum += item.second;
            cur_idx += 2;
        }
        word_topic_table_delta_.clear();
        EvaluatePrint("Send buf size = " + to_string(send_size));
        MPI_Isend(send_buf, send_size, MPI_INT, MASTER, WORD_TOPIC_TABLE, MPI_COMM_WORLD, &send_reqs);

        MPI_Status probe_status;
        MPI_Status recv_status;
        MPI_Probe(MASTER, WORD_TOPIC_TABLE, MPI_COMM_WORLD, &probe_status);
        int recv_size = 0;
        MPI_Get_count(&probe_status, MPI_INT, &recv_size);
        int *recv_buf = new int[recv_size];
        MPI_Recv(recv_buf, recv_size, MPI_INT, probe_status.MPI_SOURCE, probe_status.MPI_TAG, MPI_COMM_WORLD,
                 &recv_status);
        for (int n = 0; n < recv_size; n += 2) {
            word_topic_table_[recv_buf[n] >> TOPIC_WIDTH][recv_buf[n] & TOPIC_MASK] += recv_buf[n+1];
//            cout << world_rank_ << ": word_topic_table_[" << (recv_buf[n] >> TOPIC_WIDTH) << "]["
//                 << (recv_buf[n] & TOPIC_MASK) << "] += " << recv_buf[n+1] << " -> "
//                 << word_topic_table_[recv_buf[n] >> TOPIC_WIDTH][recv_buf[n] & TOPIC_MASK] << endl;
        }

        delete[] send_buf;
        delete[] recv_buf;

    } else {

        unordered_map<int, int> global_word_topic_table_delta;

        for (int i = 1; i < world_size_; i++) {
            MPI_Status probe_status;
            MPI_Status recv_status;
            MPI_Probe(i, WORD_TOPIC_TABLE, MPI_COMM_WORLD, &probe_status);
            int recv_size = 0;
            MPI_Get_count(&probe_status, MPI_INT, &recv_size);
            int *recv_buf = new int[recv_size];
            MPI_Recv(recv_buf, recv_size, MPI_INT, probe_status.MPI_SOURCE,
                     probe_status.MPI_TAG, MPI_COMM_WORLD, &recv_status);
            for (int n = 0; n < recv_size; n += 2) {
                global_word_topic_table_delta[recv_buf[n]] += recv_buf[n+1];
            }
            delete[] recv_buf;
        }

        int send_size = (int) global_word_topic_table_delta.size() * 2;
        int *send_buf = new int[send_size];
        int cur_idx = 0;
        for (auto const &item : global_word_topic_table_delta) {
            send_buf[cur_idx] = item.first;
            send_buf[cur_idx+1] = item.second;
            cur_idx += 2;
        }

        MPI_Request* send_reqs = new MPI_Request[world_size_];
        for (int i = 1; i < world_size_; i++) {
            MPI_Isend(send_buf, send_size, MPI_INT, i, WORD_TOPIC_TABLE, MPI_COMM_WORLD, &send_reqs[i]);
        }

        for (auto const &item : global_word_topic_table_delta) {
            word_topic_table_[item.first >> TOPIC_WIDTH][item.first & TOPIC_MASK] += item.second;
        }
        global_word_topic_table_delta.clear();

        for (int i = 1; i < world_size_; i++) {
            MPI_Status status;
            MPI_Wait(&send_reqs[i], &status);
        }

        delete[] send_buf;

    }

}


void SparseModel::Async() {
    if (world_rank_ != MASTER) {
        while (!word_topic_synced_) {
            DebugPrint("Not synced before next iter!!");
            TestWordTopicSync();
        }
    }
    DebugPrint("Before SyncTopicTable()");
    SyncTopicTable();
    DebugPrint("Before AsyncWordTopicTable()");
//    AsyncWordTopicTable();
}


//void SparseModel::AsyncWordTopicTable(){
//    int *global_word_topic_table_delta = new int[num_words_ * num_topics_]();
//    int *global_word_topic_table_delta_ptr = global_word_topic_table_delta;
//
//    if (world_rank_ == MASTER) {
//
//        MPI_Status probe_status;
//        int *partial_word_topic_table_delta = new int[num_words_ * num_topics_]();
//        int *partial_word_topic_table_delta_ptr;
//
//        for (int w = 0; w < num_words_; w++, global_word_topic_table_delta_ptr += num_topics_) {
//            for (int k = 0; k < num_topics_; k++) {
//                global_word_topic_table_delta_ptr[k] = word_topic_table_delta_[w][k];
////                word_topic_table_[w][k] -= word_topic_table_delta_[w][k];
//                word_topic_table_delta_[w][k] = 0;
//            }
//        }
//
//        for (int i = 1; i < world_size_; i++) {
//            DebugPrint("AsyncWordTopicTable() Before Recv " + to_string(i));
//            MPI_Recv(partial_word_topic_table_delta, num_words_ * num_topics_, MPI_INT, i, epoch, MPI_COMM_WORLD,
//                     &probe_status);
//            DebugPrint("AsyncWordTopicTable() After Recv " + to_string(i));
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
//            MPI_Isend(global_word_topic_table_delta, num_words_ * num_topics_, MPI_INT, i, epoch, MPI_COMM_WORLD,
//                      &send_reqs[i]);
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
//        DebugPrint("AsyncWordTopicTable() Before send ");
//
//        MPI_Request send_req;
//        MPI_Isend(*word_topic_table_delta_, num_words_ * num_topics_, MPI_INT, MASTER, epoch, MPI_COMM_WORLD,
//                  &send_req);
//        DebugPrint("AsyncWordTopicTable() After send ");
//
//        word_topic_synced_ = false;
//        MPI_Irecv(*word_topic_table_delta_buffer_, num_words_ * num_topics_, MPI_INT, MASTER, epoch, MPI_COMM_WORLD,
//                  &word_topic_request_);
//        DebugPrint("AsyncWordTopicTable() After recv ");
//
////        SyncWordTopic();
//
//        // Reset word_topic_table_delta_
//        for (int w = 0; w < num_words_; w++) {
//            for (int k = 0; k < num_topics_; k++) {
//                word_topic_table_[w][k] -= word_topic_table_delta_[w][k];
//                word_topic_table_delta_[w][k] = 0;
//            }
//        }
//    }
//
//    delete[] global_word_topic_table_delta;
//}

void SparseModel::TestWordTopicSync() {
    if (word_topic_synced_) return;
    MPI_Test (&word_topic_request_, &word_topic_synced_, MPI_STATUS_IGNORE);
    if (word_topic_synced_) {
        WordTopicMerge();
    }
}

void SparseModel::SyncWordTopic(){
    MPI_Wait (&word_topic_request_, MPI_STATUS_IGNORE);
    word_topic_synced_ = 1;
    WordTopicMerge();
};

void SparseModel::WordTopicMerge() {
    int *global_word_topic_table_delta_ptr = *word_topic_table_delta_buffer_;
    for (int w = 0; w < num_words_; w++, global_word_topic_table_delta_ptr += num_topics_) {
        for (int k = 0; k < num_topics_; k++) {
            word_topic_table_[w][k] += global_word_topic_table_delta_ptr[k];
        }
    }
}