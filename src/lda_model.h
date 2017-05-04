//
// Created by Ye Qi on 29/04/2017.
//

#ifndef WISDOMLDA_LDA_MODEL_H
#define WISDOMLDA_LDA_MODEL_H

#include <mpi.h>
#include <vector>
#include <unordered_map>
#include "lda_worker.h"

using namespace std;

class GlobalTable {
private:
    int **word_topic_table_;
    int *topic_table_;
    unordered_map<pair, int> word_topic_table_delta_;
    int *topic_table_delta_;

public:
    int world_size_;
    int world_rank_;
    int num_words_;
    int num_topics_;

public:
    GlobalTable(int world_size, int world_rank, int num_words, int num_topics);

    void Init();
    void IncWordTopicTable(int word, int topic, int delta);
    void IncTopicTable(int topic, int delta);
    int GetWordTopicTable(int word, int topic);
    double *GetWordTopicTableRows(int column_id);
    int GetTopicTable(int topic);
    void Sync();
};

inline GlobalTable::GlobalTable(int world_size, int world_rank, int num_words, int num_topics) :
        world_size_(world_size), world_rank_(world_rank), num_words_(num_words), num_topics_(num_topics) {
    word_topic_table_ = new int*[num_words_];
    for (int i = 0; i < num_words_; i++) {
        word_topic_table_[i] = new int[num_topics_];
    }
    topic_table_ = new int[num_topics_];
    if (world_rank != MASTER) {
        topic_table_delta_ = new int[num_topics];
    }
}


inline void GlobalTable::IncWordTopicTable(int word, int topic, int delta) {
    word_topic_table_[word][topic] += delta;
    pair word_topic_pair = make_pair(word, topic);
    if (word_topic_table_delta_.count(word_topic_pair))
        word_topic_table_delta_[word_topic_pair] += delta;
    else
        word_topic_table_delta_[word_topic_pair] = delta;
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


#endif //WISDOMLDA_LDA_MODEL_H
