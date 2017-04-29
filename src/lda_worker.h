//
// Created by Ye Qi on 20/04/2017.
//

#ifndef WISDOMLDA_LDA_WORKER_H
#define WISDOMLDA_LDA_WORKER_H

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>

#include "lda_model.h"

#define MASTER 0

using namespace std;

class LdaWorker {
private:
    int world_size_;
    int world_rank_;

    string data_file_;
    string output_dir_;
    int num_words_;
    int num_docs_;
    int num_topics_;
    double alpha_;
    double beta_;
    int num_iters_;
    int num_clocks_per_iter_;
    int staleness_;

    DistModel global_model_;

    double *log_likelihoods_;
    double *wall_secs_;
    double total_wall_secs_;

    int **word_topic_table_;
    int *topic_table_delta_;
    int *topic_table_;
    int *doc_length_;
    int *w;
    int *z;

    unordered_map<pair, int> word_topic_table_delta_;

public:
    void Run();

private:
    LdaWorker(int world_size, int world_rank, const string &data_file, const string &output_dir, int num_words,
              int num_topics, double alpha, double beta, int num_iters, int num_clocks_per_iter, int staleness);
    void Load(string dataFile);
    void InitTables();

    double logDirichlet(double *alpha, int length);
    double LogDirichlet(double alpha, int k);
    double *WordTopicTableRows(int columnId);
    double *DocTopicTableCols(int rowId);
    double GetLogLikelihood();

    void IncWordTopicTable(int word, int topic, int delta);
    void IncTopicTable(int topic, int delta);
    int GetWordTopicTable(int word, int topic);
    int GetTopicTable(int topic);
    void Sync();
};



inline void LdaWorker::Setup() {
    if (world_rank_ == MASTER) {
        log_likelihoods_ = new double[num_iters_];
        wall_secs_ = new double[num_iters_];
        total_wall_secs_ = 0;
        InitTables();
        // TODO: send the whole table to all workers
    } else {
        // TODO: receive the whole table from the master
    }
}

#endif //WISDOMLDA_LDA_WORKER_H
