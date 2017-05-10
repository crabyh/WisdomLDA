//
// Created by Ye Qi on 09/05/2017.
//

#ifndef WISDOMLDA_DENSE_MODEL_H
#define WISDOMLDA_DENSE_MODEL_H

#include "lda_model.h"

class DenseTable : public GlobalTable {
private:
    int **word_topic_table_;
    int **word_topic_table_delta_;
    int **word_topic_table_delta_buffer_;
    int *topic_table_delta_;

public:
    DenseTable(int world_size, int world_rank, int num_words, int num_topics) :
            GlobalTable(world_size, world_rank, num_words, num_topics) {
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
    }

    void IncWordTopicTable(int word, int topic, int delta);
    int GetWordTopicTable(int word, int topic);
    double *GetWordTopicTableRows(int column_id);

    void SyncTopicTable();
    void SyncWordTopicTable();
    void AsyncWordTopicTable();
    void WordTopicMerge();

};


inline void DenseTable::IncWordTopicTable(int word, int topic, int delta) {
    word_topic_table_[word][topic] += delta;
    word_topic_table_delta_[word][topic] += delta;
}

inline int DenseTable::GetWordTopicTable(int word, int topic) {
    return word_topic_table_[word][topic];
}

inline double *DenseTable::GetWordTopicTableRows(int column_id) {
    double *rows = new double[num_words_];
    for (int i = 0; i < num_words_; i++) {
        rows[i] = (double) word_topic_table_[i][column_id];
    }
    return rows;
}



#endif //WISDOMLDA_DENSE_MODEL_H
