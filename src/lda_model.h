//
// Created by Ye Qi on 20/04/2017.
//

#ifndef WISDOMLDA_LDA_MODEL_H
#define WISDOMLDA_LDA_MODEL_H

#include <mpi.h>
#include <vector>
#include <unordered_map>

using namespace std;

class DistModel {
private:
    vector<vector<int>> word_topic_table_;
    vector<int> topic_table_;
    unordered_map<pair, int> word_topic_table_delta_;
    vector<int> topic_table_delta_;

public:
    DistModel(){}

    void IncWordTopicTable(int word, int topic, int delta);

    void IncTopicTable(int topic, int delta);

    int GetWordTopicTable(int word, int topic);
    int GetTopicTable(int topic);
    void Sync();
};

inline void DistModel::IncWordTopicTable(int word, int topic, int delta) {
    word_topic_table_[word][topic] += delta;
    word_topic_table_delta_[make_pair(word, topic)] += delta;
}

inline void DistModel::IncTopicTable(int topic, int delta) {
    topic_table_[topic] += delta;
    topic_table_delta_[topic] += delta;
}

inline int DistModel::GetWordTopicTable(int word, int topic) {
    return word_topic_table_[word][topic];
}

inline int DistModel::GetTopicTable(int topic) {
    return topic_table_[topic];
}

inline void DistModel::Sync() {}

#endif //WISDOMLDA_LDA_MODEL_H
