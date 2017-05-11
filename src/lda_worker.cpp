//
// Created by Ye Qi on 20/04/2017.
//

#include <sys/time.h>
#include <iomanip>

#include "lda_worker.h"

using namespace std;

LdaWorker::LdaWorker(int world_size, int world_rank,
                     const string &data_file, const string &output_dir,
                     int num_words, int num_docs, int num_topics,
                     double alpha, double beta,
                     int num_iters, int num_clocks_per_iter, int staleness)
        : world_size_(world_size), world_rank_(world_rank),
          data_file_(data_file), output_dir_(output_dir),
          num_words_(num_words), num_docs_(num_docs), num_topics_(num_topics),
          alpha_(alpha), beta_(beta),
          num_iters_(num_iters), num_documents_per_sync(num_clocks_per_iter), staleness_(staleness),
          global_table_(world_size, world_rank, num_words, num_topics) {

}

void LdaWorker::Run() {

//    global_table_.DebugPrint("Run()");
    struct timeval t1, t2;
    double *p = new double[num_topics_];

    for (int iter = 0; iter < num_iters_; iter++) {
        if (world_rank_ == MASTER) {
            gettimeofday(&t1, NULL);
        }
        for (int begin = 0; begin < num_docs_; begin += num_documents_per_sync) {

            if (world_rank_ == MASTER) {
//                global_table_.Async();
                global_table_.Sync();
            } else {
                if (world_rank_ != MASTER) gettimeofday(&t1, NULL);

                int end = min(begin + num_documents_per_sync, num_docs_);

                // Loop through each document in the current batch.
                for (int d = begin; d < end; d++) {

//                    global_table_.TestWordTopicSync();

                    for (int i = 0; i < doc_length_[d]; i++) {
                        int word = w[d][i];
                        int topic = z[d][i];
                        doc_topic_table_[d][topic] -= 1;
                        global_table_.IncWordTopicTable(word, topic, -1);
                        global_table_.IncTopicTable(topic, -1);

                        double norm = 0.0;
                        for (int k = 0; k < num_topics_; k++) {
                            double bk = (global_table_.GetWordTopicTable(word, k) + beta_) /
                                        ((double) global_table_.GetTopicTable(k) + num_words_ * beta_);
                            double ak = doc_topic_table_[d][k] + alpha_;
                            double pk = bk * ak;
                            p[k] = pk;
                            norm += pk;
                        }

                        int new_topic = SampleMultinomial(p, norm);

                        z[d][i] = new_topic;
                        doc_topic_table_[d][new_topic] += 1;
                        global_table_.IncWordTopicTable(word, new_topic, 1);
                        global_table_.IncTopicTable(new_topic, 1);
                    }
                }
//                global_table_.DebugPrint(": Before Sync in Run()");
                if (world_rank_ != MASTER) {
                    gettimeofday(&t2, NULL);
                    total_wall_secs_ += (t2.tv_sec - t1.tv_sec) + (t2.tv_usec - t1.tv_usec) / 1000000.0;;
                }
//                global_table_.Async();
                global_table_.Sync();
            }
        }
        if (world_rank_ == MASTER) {
            gettimeofday(&t2, NULL);
            wall_secs_[iter] = (t2.tv_sec - t1.tv_sec) + (t2.tv_usec - t1.tv_usec) / 1000000.0;
            total_wall_secs_ += wall_secs_[iter];
            log_likelihoods_[iter] = GetLogLikelihood();
//                cout << std::setprecision(2) << "Iteration time: " << wall_secs_[iter] << endl;
            cout << std::setprecision(4) << total_wall_secs_ << "\t";
            cout << std::setprecision(6) << log_likelihoods_[iter] << endl;
        }
        else
            cout << std::setprecision(4) << "Gibbs Sampling\t" << total_wall_secs_ << endl;
    }
    delete[] p;
}

int LdaWorker::SampleMultinomial(double *p, double norm) {
    double sum_p_up_to_k = 0.0;
    double r = ((double) rand() / (RAND_MAX));
    int topic = 0;
    for (int k = 0; k < num_topics_; k++) {
        sum_p_up_to_k += p[k] / norm;
        if (r <= sum_p_up_to_k) {
            topic = k;
            break;
        }
    }
    return topic;
}


void LdaWorker::LoadPartial(string dataFile) {
    w = new int *[num_docs_];
    doc_length_ = new int[num_docs_];
    string line;
    ifstream file(dataFile);
    if (world_rank_ != MASTER)
        cout << world_rank_ << ": " << num_docs_ << endl;
    else
        return;

    if (file.is_open()) {
        int line_num = 0;
        int doc = 0;
        cout << world_rank_ << ": num_docs_ " << num_docs_ << endl;
        while (getline(file, line)) {
//            if (world_rank_ != MASTER)
//                cout << world_rank_ << ": LoadPartial() " << line << endl;
//            if (line_num % (world_size_) == world_rank_) {
            if (line_num % (world_size_ - 1) == world_rank_ - 1) {
                doc_length_[doc] = 1;
                for (unsigned long i = 0; i < line.length(); i++) {
                    if (line[i] == ',') {
                        doc_length_[doc] += 1;
                    }
                }
                w[doc] = new int[doc_length_[doc]];
                int index = 0;
                int last_i = -1;
                for (int i = 0; i < (int) line.length(); i++) {
                    if (line[i] == ',') {
                        w[doc][index] = (stoi(line.substr(last_i + 1, i - last_i - 1)));
                        index += 1;
                        last_i = i;
                    }
                }
                w[doc][index] = (stoi(line.substr(last_i + 1, line.length() - last_i - 1)));
                doc += 1;
            }
            line_num += 1;
        }

    }


    doc_topic_table_ = new int *[num_docs_];
    for (int i = 0; i < num_docs_; i++) {
        doc_topic_table_[i] = new int[num_topics_]();
    }

    cout << world_rank_ << ": LoadPartial Done " << endl;
}

void LdaWorker::InitTables() {
//    global_table_.DebugPrint(": InitTables()");
    z = new int *[num_docs_];
    for (int d = 0; d < num_docs_; d++) {
        z[d] = new int[doc_length_[d]];
        for (int i = 0; i < doc_length_[d]; i++) {
            int word = w[d][i];
            int topic = rand() % num_topics_;
            z[d][i] = topic;
            doc_topic_table_[d][topic] += 1;
            global_table_.IncWordTopicTable(word, topic, 1);
            global_table_.IncTopicTable(topic, 1);
        }
    }
//    global_table_.DebugPrint(": Before Sync()");
    global_table_.Sync();
}

void LdaWorker::Setup() {
    log_likelihoods_ = new double[num_iters_];
    wall_secs_ = new double[num_iters_];
    total_wall_secs_ = 0;
//    global_table_.DebugPrint(": Finished new arrays");
    LoadPartial(data_file_);
//    global_table_.DebugPrint(": Finished loading document collection");
    InitTables();
//    global_table_.DebugPrint(": Finished initializing table");
}


double LdaWorker::LogDirichlet(double *alpha_, int length) {
    double sumLogGamma = 0.0;
    double logSumGamma = 0.0;
    for (int i = 0; i < length; i++) {
        sumLogGamma += lgamma(alpha_[i]);
        logSumGamma += alpha_[i];
    }
    return sumLogGamma - lgamma(logSumGamma);
}

double LdaWorker::LogDirichlet(double alpha_, int k) {
    return k * lgamma(alpha_) - lgamma(k * alpha_);
}

double *LdaWorker::DocTopicTableCols(int rowId) {
    double *cols = new double[num_topics_];
    for (int i = 0; i < num_topics_; i++) {
        cols[i] = (double) doc_topic_table_[rowId][i];
    }
    return cols;
}

double LdaWorker::GetLogLikelihood() {
    double lik = 0.0;
    for (int k = 0; k < num_topics_; k++) {
        double *temp = global_table_.GetWordTopicTableRows(k);
        for (int w = 0; w < num_words_; w++) {
            temp[w] += alpha_;
        }
        lik += LogDirichlet(temp, num_words_);
        lik -= LogDirichlet(beta_, num_words_);
        delete[] temp;
    }
//    for (int d = 0; d < num_docs_; d++) {
//        double *temp = DocTopicTableCols(d);
//        for (int k = 0; k < num_topics_; k++) {
//            temp[k] += alpha_;
//        }
//        lik += LogDirichlet(temp, num_topics_);
//        lik -= LogDirichlet(alpha_, num_topics_);
//        delete[] temp;
//    }
    return lik;
}
