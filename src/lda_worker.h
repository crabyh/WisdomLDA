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
#include <math.h>

#include "dense_model.h"
#include "sparse_model.h"

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
    int num_documents_per_sync;
    int staleness_;

    double *log_likelihoods_;
    double *wall_secs_;
    double total_wall_secs_;

    int **doc_topic_table_;
    int *doc_length_;
    int **w;
    int **z;

    SparseModel global_table_;

public:
    void Setup();
    void Run();

    LdaWorker(int world_size, int world_rank,
              const string &data_file, const string &output_dir,
              int num_words, int num_docs, int num_topics,
              double alpha, double beta, int num_iters,
              int num_clocks_per_iter, int staleness);

private:
    void LoadAll(string dataFile);
    void LoadPartial(string dataFile);
    void InitTables();

    double LogDirichlet(double *alpha, int length);
    double LogDirichlet(double alpha, int k);
    double *DocTopicTableCols(int rowId);
    double GetLogLikelihood();
    int SampleMultinomial(double *p, double norm);

};



#endif //WISDOMLDA_LDA_WORKER_H
