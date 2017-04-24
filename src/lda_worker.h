//
// Created by Ye Qi on 20/04/2017.
//

#ifndef WISDOMLDA_LDA_WORKER_H
#define WISDOMLDA_LDA_WORKER_H

#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include "lda_model.h"

#define MASTER 0

using namespace std;

class LdaWorker {
public:
    int world_size_;
    int world_rank_;

    string data_file_;
    string output_dir_;
    int num_words_;
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

    LdaWorker(int world_size, int world_rank,
              const string &data_file, const string &output_dir,
              int num_words, int num_topics,
              double alpha, double beta,
              int num_iters, int num_clocks_per_iter, int staleness);

public:
    void Run();

    void Setup();

    void Load();

private:
    void InitTables();

    double LogDirichlet(vector<double> alpha);

    double LogDirichlet(double alpha, int k);

    vector<double> GetRows(vector<vector<int>> matrix, int column_id);

    vector<double> GetColumns(vector<vector<int>> matrix, int row_id);

    double GetLogLikelihood();
};

inline LdaWorker::LdaWorker(int world_size, int world_rank,
                            const string &data_file, const string &output_dir,
                            int num_words, int num_topics,
                            double alpha, double beta,
                            int num_iters, int num_clocks_per_iter,
                            int staleness) : world_size_(world_size),
                                             world_rank_(world_rank),
                                             data_file_(data_file),
                                             output_dir_(output_dir),
                                             num_words_(num_words),
                                             num_topics_(num_topics),
                                             alpha_(alpha), beta_(beta),
                                             num_iters_(num_iters),
                                             num_clocks_per_iter_(
                                                     num_clocks_per_iter),
                                             staleness_(staleness) {}

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
