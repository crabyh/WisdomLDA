//
// Created by Ye Qi on 20/04/2017.
//

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
          num_iters_(num_iters), num_clocks_per_iter_(num_clocks_per_iter), staleness_(staleness),
          global_table_(world_size, world_rank, num_words, num_topics) {

}

void LdaWorker::Run() {

    time_t t1, t2;
    for (int iter = 0; iter < num_iters_; iter++) {
        if (world_rank_ == MASTER) {
            time(&t1);
        }

        for (int batch = 0; batch < num_clocks_per_iter_; batch++) {
            int begin = num_docs_ * batch / num_clocks_per_iter_;
            int end = num_docs_ * (batch + 1) / num_clocks_per_iter_;

            // Loop through each document in the current batch.
            for (int d = begin; d < end; d++) {
                for (int i = 0; i < doc_length_[d]; i++) {
                    int word = w[d][i];
                    int topic = z[d][i];
                    doc_topic_table_[d][topic] -= 1;
                    global_table_.IncWordTopicTable(word, topic, -1);
                    global_table_.IncTopicTable(topic, -1);

                    double *p = new double[num_topics_];
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
                    delete[] p;

                    z[d][i] = new_topic;
                    doc_topic_table_[d][new_topic] += 1;
                    global_table_.IncWordTopicTable(word, new_topic, 1);
                    global_table_.IncTopicTable(new_topic, 1);
                }
            }

            global_table_.Sync();
        }

        if (world_rank_ == MASTER) {
            // TODO: collect all updates from workers
            time(&t2);
            wall_secs_[iter] = difftime(t2, t1);
            total_wall_secs_ += wall_secs_[iter];
            log_likelihoods_[iter] = GetLogLikelihood();
            cout << "Iteration time: " << wall_secs_[iter] << endl;
            cout << "Log-likelihood: " << log_likelihoods_[iter] << endl;
        } else {
            // TODO: send update to master
        }
    }

    if (world_rank_ == MASTER) {
        ofstream file(output_dir_ + "/likelihood.csv");
        for (int i = 0; i < num_iters_; i++) {
            string s = to_string(i + 1) + "," + to_string(wall_secs_[i]) + "," +
                       to_string(log_likelihoods_[i]) + "\n";
            file.write(s.c_str(), s.size());
        }
        file.close();
    }

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

// Master should load all the data to init global table
void LdaWorker::LoadAll(string dataFile) {
    w = new int*[num_docs_];
    doc_length_ = new int[num_docs_]();
    string line;
    ifstream file (dataFile);
    if (file.is_open()) {
        int doc = 0;
        while (getline(file, line)) {
            for (unsigned long i = 0; i < line.length(); i++)
                if (line[i] == ',') doc_length_[doc] += 1;
            int *w_col = new int[doc_length_[doc]];
            int index = 0;
            unsigned long last_i = 0;
            for (unsigned long i = 0; i < line.length(); i++) {
                if (line[i] == ',') {
                    w_col[index] = (stoi(line.substr(last_i + 1, i - last_i - 1)));
                    last_i = i;
                }
            }
            w[doc] = w_col;
        }
    }
    doc_topic_table_ = new int*[num_docs_];
    for (int i = 0; i < num_docs_; i++) {
        int *doc_length_col = new int[num_topics_]();
        doc_topic_table_[i] = doc_length_col;
    }
}

void LdaWorker::LoadPartial(string data_file) {
    // TODO:
}

void LdaWorker::InitTables() {
    z = new int*[num_docs_];
    for (int d = 0; d < num_docs_; d++) {
        int *z_col = new int[doc_length_[d]];
        for (int i = 0; i < doc_length_[d]; i++) {
            int word = w[d][i];
            int topic = rand() % num_topics_;
            z_col[i] = topic;
            doc_topic_table_[d][topic] += 1;
            global_table_.IncWordTopicTable(word, topic, 1);
            global_table_.IncTopicTable(topic, 1);
        }
        z[d] = z_col;
    }
}

void LdaWorker::Setup() {
    if (world_rank_ == MASTER) {
        LoadAll(data_file_);
        log_likelihoods_ = new double[num_iters_];
        wall_secs_ = new double[num_iters_];
        total_wall_secs_ = 0;
        InitTables();
    } else {
        LoadPartial(data_file_);
    }
    global_table_.Init();
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
    for (int d = 0; d < num_docs_; d++) {
        double *temp = DocTopicTableCols(d);
        for (int k = 0; k < num_topics_; k++) {
            temp[k] += alpha_;
        }
        lik += LogDirichlet(temp, num_topics_);
        lik -= LogDirichlet(alpha_, num_topics_);
        delete[] temp;
    }
    return lik;
}
