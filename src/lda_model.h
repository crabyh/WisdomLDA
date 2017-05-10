//
// Created by Ye Qi on 29/04/2017.
//

#ifndef WISDOMLDA_LDA_MODEL_H
#define WISDOMLDA_LDA_MODEL_H

#include <iostream>
#include <vector>
#include <sstream>

#include "mpi.h"

using namespace std;

class GlobalTable {
protected:
    int *topic_table_;
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
    GlobalTable(int world_size, int world_rank, int num_words, int num_topics) :
            epoch(0), world_size_(world_size), world_rank_(world_rank), num_words_(num_words), num_topics_(num_topics) {
        topic_table_ = new int[num_topics_]();
        topic_table_delta_ = new int[num_topics_]();

        word_topic_synced_ = 1;
    }

    virtual void IncWordTopicTable(int word, int topic, int delta) = 0;
    void IncTopicTable(int topic, int delta);
    virtual int GetWordTopicTable(int word, int topic) = 0;
    virtual double *GetWordTopicTableRows(int column_id) = 0;
    int GetTopicTable(int topic);
    void Exchange();
    void Sync();
    void Async();
    void SyncTopicTable();
    virtual void SyncWordTopicTable() =0;
    void TestWordTopicSync();
    virtual void WordTopicMerge() =0;
    void SyncWordTopic();
    virtual void AsyncWordTopicTable() =0;

    // for debugging
    void DebugPrint(const string &s);
    void DebugPrintTable();
};


inline void GlobalTable::IncTopicTable(int topic, int delta) {
    topic_table_[topic] += delta;
    topic_table_delta_[topic] += delta;
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

#endif //WISDOMLDA_LDA_MODEL_H
