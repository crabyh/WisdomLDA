//
// Created by Ye Qi on 10/05/2017.
//

#ifndef WISDOMLDA_SPARSE_MODEL_H
#define WISDOMLDA_SPARSE_MODEL_H

#include <iostream>
#include <vector>
#include <sstream>

#include "mpi.h"

using namespace std;

class GlobalTable {
private:
    int **word_topic_table_;
    int *topic_table_;
    int **word_topic_table_delta_;
    int **word_topic_table_delta_buffer_;
    int *topic_table_delta_;
    int epoch;
    int word_topic_synced_;
    MPI_Request word_topic_request_;

public:
    int world_size_;
    int world_rank_;
    int num_words_;
    int num_topics_;

public:
    GlobalTable(int world_size, int world_rank, int num_words, int num_topics);

    void IncWordTopicTable(int word, int topic, int delta);
    void IncTopicTable(int topic, int delta);
    int GetWordTopicTable(int word, int topic);
    double *GetWordTopicTableRows(int column_id);
    int GetTopicTable(int topic);
    void Sync();
    void Async();
    void SyncTopicTable();
    void SyncWordTopicTable();
    void TestWordTopicSync();
    void WordTopicMerge();
    void SyncWordTopic();
    void AsyncWordTopicTable();

    // for debugging
    void DebugPrint(const string &s);
    void DebugPrintTable();
};

inline GlobalTable::GlobalTable(int world_size, int world_rank, int num_words, int num_topics) :
        world_size_(world_size), world_rank_(world_rank), num_words_(num_words), num_topics_(num_topics) {
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

    word_topic_table_delta_buffer_ = new int*[num_words_];
    int *word_topic_pools_buffer_delta = new int[num_words_ * num_topics_]();
    for (int i = 0; i < num_words_; i++, word_topic_pools_buffer_delta += num_topics_) {
        word_topic_table_delta_buffer_[i] = word_topic_pools_buffer_delta;
    }

    topic_table_ = new int[num_topics_]();
    topic_table_delta_ = new int[num_topics_]();

    word_topic_synced_ = 1;
}

inline void GlobalTable::IncWordTopicTable(int word, int topic, int delta) {
    word_topic_table_[word][topic] += delta;
    word_topic_table_delta_[word][topic] += delta;
}

inline void GlobalTable::IncTopicTable(int topic, int delta) {
    topic_table_[topic] += delta;
    topic_table_delta_[topic] += delta;
}

inline int GlobalTable::GetWordTopicTable(int word, int topic) {
    return word_topic_table_[word][topic];
}

inline double *GlobalTable::GetWordTopicTableRows(int column_id) {
    double *rows = new double[num_words_];
    for (int i = 0; i < num_words_; i++) {
        rows[i] = (double) word_topic_table_[i][column_id];
    }
    return rows;
}

inline int GlobalTable::GetTopicTable(int topic) {
    return topic_table_[topic];
}

inline void GlobalTable::DebugPrint(const string &s) {
//    cout << world_rank_ << ": " << s << endl;
}

inline void GlobalTable::DebugPrintTable() {
//    int sum = 0;
//    for (int w = 0; w < num_words_; w++) {
//        for (int k = 0; k < num_topics_; k++) {
//            sum += word_topic_table_[w][k];
//        }
//    }
//    cout << world_rank_ << ": Sum = " << sum << endl;
}

template<typename T>
string ToString(T input) {
    stringstream ss;
    ss << input;
    string str = ss.str();
    return str;
}


#endif //WISDOMLDA_SPARSE_MODEL_H
