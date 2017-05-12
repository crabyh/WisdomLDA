//
// Created by Ye Qi on 29/04/2017.
//

#define MASTER 0

#define TOPIC_TABLE_TAG 3
#define WORD_TOPIC_TABLE_TAG 4

#include "dense_model.h"
#include <sys/time.h>


void DenseModel::Sync() {
//    DebugPrint("Before SyncTopicTable()");

    SyncTopicTable();
//    DebugPrint("Before SyncWordTopicTable()");
    SyncWordTopicTable();

    gettimeofday(&t2, NULL);
    sync_time += (t2.tv_sec - t1.tv_sec) + (t2.tv_usec - t1.tv_usec) / 1000000.0;
//    if (sync_time > 10 && world_rank_ != MASTER)
//        DebugPrint("Communication time: " + to_string(sync_time));
}


void DenseModel::SyncTopicTable() {
    MPI_Reduce(topic_table_, global_topic_table_, num_topics_, MPI_INT, MPI_SUM, MASTER, MPI_COMM_WORLD);

    gettimeofday(&t1, NULL);
    if (world_rank_ == MASTER) {
        for (int k = 0; k < num_topics_; k++) {
            topic_table_[k] = global_topic_table_[k] - (world_size_ - 1) * topic_table_[k];
        }
    }
    gettimeofday(&t2, NULL);
    marshing_time += (t2.tv_sec - t1.tv_sec) + (t2.tv_usec - t1.tv_usec) / 1000000.0;
    MPI_Bcast(topic_table_, num_topics_, MPI_INT, MASTER, MPI_COMM_WORLD);
}

void DenseModel::SyncWordTopicTable() {
    gettimeofday(&t1, NULL);
    MPI_Reduce(*word_topic_table_, global_word_topic_table_, num_words_ * num_topics_, MPI_INT, MPI_SUM, MASTER,
               MPI_COMM_WORLD);
    if (world_rank_ == MASTER) {
        int *global_word_topic_table_ptr = global_word_topic_table_;
        for (int w = 0; w < num_words_; w++, global_word_topic_table_ptr += num_topics_) {
            for (int k = 0; k < num_topics_; k++) {
                word_topic_table_[w][k] = global_word_topic_table_ptr[k] - (world_size_ - 1) * word_topic_table_[w][k];
            }
        }
    }
    MPI_Bcast(*word_topic_table_, num_words_ * num_topics_, MPI_INT, MASTER, MPI_COMM_WORLD);
    gettimeofday(&t2, NULL);
    marshing_time += (t2.tv_sec - t1.tv_sec) + (t2.tv_usec - t1.tv_usec) / 1000000.0;
//    if (marshing_time > 2)
//        EvaluatePrint("Communicate time: " + to_string(sync_time));
//    MPI_Barrier(MPI_COMM_WORLD);
}


//void DenseModel::Async() {
//    if (world_rank_ != MASTER) {
//        while (!word_topic_synced_) {
////            DebugPrint("Not synced before next iter!!");
//            TestAsyncSync();
//        }
//    }
////    DebugPrint("Before SyncTopicTable()");
//    SyncTopicTable();
////    DebugPrint("Before AsyncWordTopicTable()");
//    AsyncWordTopicTable();
////    int sum = 0;
////    for (int i = 0; i < num_words_; i++) {
////        for (int j = 0; j < num_topics_; j++) {
////            sum += word_topic_table_[i][j];
////        }
////    }
////    cout << "sum: " << sum << endl;
//}
//


void DenseModel::Async(){
    DebugPrint("Async()");

    if (world_rank_ == MASTER) {

        for (int i = 0; i < 2 * (world_size_ - 1); i++) {
            DebugPrint("AsyncWordTopicTable() Done");

            MPI_Status status;

            MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

            if (status.MPI_TAG == TOPIC_TABLE_TAG) {
                MPI_Recv(partial_topic_table_, num_topics_, MPI_INT, status.MPI_SOURCE, TOPIC_TABLE_TAG, MPI_COMM_WORLD,
                         MPI_STATUS_IGNORE);
                for (int k = 0; k < num_topics_; k++) {
                    topic_table_[k] += partial_topic_table_[k];
                }
                MPI_Send(topic_table_, num_topics_, MPI_INT, status.MPI_SOURCE, TOPIC_TABLE_TAG, MPI_COMM_WORLD);
            }

            else if (status.MPI_TAG == WORD_TOPIC_TABLE_TAG) {
                MPI_Recv(*partial_word_topic_table_, num_words_ * num_topics_, MPI_INT, status.MPI_SOURCE,
                         WORD_TOPIC_TABLE_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                for (int w = 0; w < num_words_; w++) {
                    for (int k = 0; k < num_topics_; k++) {
                        word_topic_table_[w][k] += partial_word_topic_table_[w][k];
                    }
                }

                MPI_Send(*word_topic_table_, num_words_ * num_topics_, MPI_INT, status.MPI_SOURCE,
                         WORD_TOPIC_TABLE_TAG, MPI_COMM_WORLD);

                DebugPrint("AsyncWordTopicTable() Done");
            }
        }
    }

    else {
        DebugPrint("AsyncWordTopicTable() Before send ");

        MPI_Send(topic_table_delta_, num_topics_, MPI_INT, MASTER, TOPIC_TABLE_TAG, MPI_COMM_WORLD);

        for (int k = 0; k < num_topics_; k++)
            topic_table_delta_[k] = 0;
        topic_synced_ = false;

        MPI_Recv(topic_table_, num_topics_, MPI_INT, MASTER, TOPIC_TABLE_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);//, &topic_request_);

        MPI_Send(*word_topic_table_delta_, num_words_ * num_topics_, MPI_INT, MASTER, WORD_TOPIC_TABLE_TAG,
                  MPI_COMM_WORLD);
        DebugPrint("AsyncWordTopicTable() After send ");

        for (int w = 0; w < num_words_; w++) {
            for (int k = 0; k < num_topics_; k++) {
                word_topic_table_delta_[w][k] = 0;
            }
        }
        word_topic_synced_ = false;
        MPI_Recv(*word_topic_table_, num_words_ * num_topics_, MPI_INT, MASTER, WORD_TOPIC_TABLE_TAG,
                  MPI_COMM_WORLD, MPI_STATUS_IGNORE);//, &word_topic_request_);
        DebugPrint("AsyncWordTopicTable() After recv ");

//        AsynSynTables();

    }
}

void DenseModel::TestAsyncSync() {
    if (!topic_synced_) {
        MPI_Test(&topic_request_, &topic_synced_, MPI_STATUS_IGNORE);
        if (topic_synced_) {
            for (int k = 0; k < num_topics_; k++)
                topic_table_[k] = topic_table_buffer_[k];
        }
    }
    if (!word_topic_synced_) {
        MPI_Test(&word_topic_request_, &word_topic_synced_, MPI_STATUS_IGNORE);
        if (word_topic_synced_) {
            WordTopicMerge();
        }
    }
}

void DenseModel::AsynSynTables(){
    MPI_Wait (&topic_request_, MPI_STATUS_IGNORE);
    topic_synced_ = true;
    for (int k = 0; k < num_topics_; k++)
        topic_table_[k] = topic_table_buffer_[k];
    MPI_Wait (&word_topic_request_, MPI_STATUS_IGNORE);
    word_topic_synced_ = true;
    WordTopicMerge();
};

void DenseModel::WordTopicMerge() {
    DebugPrint("WordTopicMerge ");
    for (int w = 0; w < num_words_; w++) {
        for (int k = 0; k < num_topics_; k++) {
            word_topic_table_[w][k] = word_topic_table_buffer_[w][k];
        }
    }
}


