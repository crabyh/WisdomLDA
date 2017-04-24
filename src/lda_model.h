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
    vector<vector<int>> word_topic_table;
    vector<int> topic_table;
    unordered_map<pair, int> word_topic_table_delta;
    vector<int> topic_table_delta;

public:
    DistModel(){}

    void inc_word_topic_table(int word, int topic, int delta);

    void inc_topic_table(int topic, int delta);

    void sync();
};

inline void DistModel::inc_word_topic_table(int word, int topic, int delta) {
    word_topic_table[word][topic] += delta;
    word_topic_table_delta[make_pair(word, topic)] += delta;
}

inline void DistModel::inc_topic_table(int topic, int delta) {
    topic_table[topic] += delta;
    topic_table_delta[topic] += delta;
}

inline void DistModel::sync() {}

#endif //WISDOMLDA_LDA_MODEL_H
