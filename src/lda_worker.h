//
// Created by Ye Qi on 20/04/2017.
//

#ifndef WISDOMLDA_LDA_WORKER_H
#define WISDOMLDA_LDA_WORKER_H

#include <string>
#include <vector>

#include "lda_model.h"

#define MASTER 0

class LdaWorker {
public:
    int world_size;
    int world_rank;

    string data_file;
    string output_dir;
    int num_words;
    int num_topics;
    double alpha;
    double beta;
    int num_iters;
    int num_clocks_per_iter;
    int staleness;

    DistModel model;
    std::vector<std::vector<int>> documents;

    double *log_likelihood;
    double *wall_secs;
    double total_wall_secs;

    LdaWorker(int _world_size, int _world_rank,
              const string &_data_file, const string &_output_dir,
              int _num_words, int _num_topics,
              double _alpha, double _beta,
              int _num_iters, int _num_clocks_per_iter, int _staleness);

public:
    void run();

private:
    void setup();
    void load();
    void init_tables();
    double logDirichlet(vector<double> alpha);
    double logDirichlet(double alpha, int k);
    vector<double> getRows(vector<vector<int>> matrix, int columnId);
    vector<double> getColumns(vector<vector<int>> matrix, int rowId);
    double getLogLikelihood();
};

inline LdaWorker::LdaWorker(int _world_size, int _world_rank,
                            const string &_data_file, const string &_output_dir,
                            int _num_words, int _num_topics,
                            double _alpha, double _beta,
                            int _num_iters, int _num_clocks_per_iter,
                            int _staleness) : world_size(_world_size),
                                              world_rank(_world_rank),
                                              data_file(_data_file),
                                              output_dir(_output_dir),
                                              num_words(_num_words),
                                              num_topics(_num_topics),
                                              alpha(_alpha), beta(_beta),
                                              num_iters(_num_iters),
                                              num_clocks_per_iter(
                                                      _num_clocks_per_iter),
                                              staleness(_staleness) {}

inline void LdaWorker::setup() {
    if (world_rank == MASTER) {
        log_likelihood = new double[num_iters];
        wall_secs = new double[num_iters];
        total_wall_secs = 0;
    }
}

#endif //WISDOMLDA_LDA_WORKER_H
