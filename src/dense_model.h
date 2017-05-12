//
// Created by Ye Qi on 29/04/2017.
//

#ifndef WISDOMLDA_DENSE_MODEL_H
#define WISDOMLDA_DENSE_MODEL_H

#define MASTER 0

#include <iostream>
#include <vector>
#include <sstream>

#include "mpi.h"

using namespace std;

class DenseModel {
private:
    int *topic_table_;
    int *topic_table_delta_;
    int *topic_table_buffer_;
    int **word_topic_table_;
    int **word_topic_table_delta_;
    int **word_topic_table_buffer_;
    int *global_topic_table_;
    int *global_word_topic_table_;
    int epoch;
    int word_topic_synced_;
    int topic_synced_;
    int *partial_topic_table_;
    int **partial_word_topic_table_;
    MPI_Request word_topic_request_;
    MPI_Request topic_request_;
    MPI_Request request;
    double marshing_time;
    double sync_time;
    struct timeval t1, t2;
    int async_;

public:
    int world_size_;
    int world_rank_;
    int num_words_;
    int num_topics_;

public:
    DenseModel(int world_size, int world_rank, int num_words, int num_topics, int async);

    void IncWordTopicTable(int word, int topic, int delta);
    void IncTopicTable(int topic, int delta);
    int GetWordTopicTable(int word, int topic);
    double *GetWordTopicTableRows(int column_id);
    int GetTopicTable(int topic);
    void Sync();
    void Async();
    void SyncTopicTable();
    void SyncWordTopicTable();
    void TestAsyncSync();
    void WordTopicMerge();
    void AsynSynTables();
    void AsyncWordTopicTable();

    // for debugging
    void EvaluatePrint(const string &s);
    void DebugPrint(const string &s);
    void DebugPrintTable();
};

inline DenseModel::DenseModel(int world_size, int world_rank, int num_words, int num_topics, int async) :
        async_(async), world_size_(world_size), world_rank_(world_rank), num_words_(num_words), num_topics_(num_topics) {
    epoch = 0;
    word_topic_table_ = new int*[num_words_];
    int *word_topic_pools = new int[num_words_ * num_topics_]();
    for (int i = 0; i < num_words_; i++, word_topic_pools += num_topics_) {
        word_topic_table_[i] = word_topic_pools;
    }

    word_topic_table_delta_ = new int*[num_words_];
    int *word_topic_pools_delta = new int[num_words_ * num_topics_]();
    for (int i = 0; i < num_words_; i++, word_topic_pools_delta += num_topics_) {
        word_topic_table_delta_[i] = word_topic_pools_delta;
    }

    word_topic_table_buffer_ = new int*[num_words_];
    int *word_topic_pools_buffer_delta = new int[num_words_ * num_topics_]();
    for (int i = 0; i < num_words_; i++, word_topic_pools_buffer_delta += num_topics_) {
        word_topic_table_buffer_[i] = word_topic_pools_buffer_delta;
    }

    topic_table_ = new int[num_topics_]();
    if (async_) topic_table_delta_ = new int[num_topics_]();
    if (async_) topic_table_buffer_ = new int[num_topics_]();

    if (world_rank_ == MASTER) {
        global_topic_table_ = new int[num_topics_];
        global_word_topic_table_ = new int [num_words_ * num_topics_]();
    }

    partial_topic_table_ = new int[num_topics_]();
    partial_word_topic_table_ = new int*[num_words_];
    int *partial_word_topic_pools = new int[num_words_ * num_topics_]();
    for (int i = 0; i < num_words_; i++, partial_word_topic_pools += num_topics_) {
        partial_word_topic_table_[i] = partial_word_topic_pools;
    }

    word_topic_synced_ = 1;
    topic_synced_ = 1;
    marshing_time = 0;
    sync_time = 0;
}

inline void DenseModel::IncWordTopicTable(int word, int topic, int delta) {
    word_topic_table_[word][topic] += delta;
    if (async_) word_topic_table_delta_[word][topic] += delta;
}

inline void DenseModel::IncTopicTable(int topic, int delta) {
    topic_table_[topic] += delta;
    if (async_) topic_table_delta_[topic] += delta;
}

inline int DenseModel::GetWordTopicTable(int word, int topic) {
    return word_topic_table_[word][topic];
}

inline double *DenseModel::GetWordTopicTableRows(int column_id) {
    double *rows = new double[num_words_];
    for (int i = 0; i < num_words_; i++) {
        rows[i] = (double) word_topic_table_[i][column_id];
    }
    return rows;
}

inline int DenseModel::GetTopicTable(int topic) {
    return topic_table_[topic];
}

inline void DenseModel::DebugPrint(const string &s) {
//    cout << world_rank_ << ": " << s << endl;
}

inline void DenseModel::EvaluatePrint(const string &s) {
    cout << world_rank_ << ": " << s << endl;
}

inline void DenseModel::DebugPrintTable() {
//    int sum = 0;
//    for (int w = 0; w < num_words_; w++) {
//        for (int k = 0; k < num_topics_; k++) {
//            sum += word_topic_table_[w][k];
//        }
//    }
//    cout << world_rank_ << ": Sum = " << sum << endl;
}

#endif //WISDOMLDA_DENSE_MODEL_H
