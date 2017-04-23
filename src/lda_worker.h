//
// Created by Ye Qi on 20/04/2017.
//

#ifndef WISDOMLDA_LDA_WORKER_H
#define WISDOMLDA_LDA_WORKER_H

#include <string>
#include <vector>

#include "lda_model.h"

using namespace std;

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

    LdaWorker(int _world_size, int _world_rank,
              const string &_data_file, const string &_output_dir,
              int _num_words, int _num_topics,
              double _alpha, double _beta,
              int _num_iters, int _num_clocks_per_iter, int _staleness);

    vector<vector<int>> load();

    void run();

    void setup();
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

inline void LdaWorker::setup() {}

#endif //WISDOMLDA_LDA_WORKER_H
