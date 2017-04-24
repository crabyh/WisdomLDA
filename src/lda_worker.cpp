//
// Created by Ye Qi on 20/04/2017.
//

#include "lda_worker.h"

using namespace std;

void LdaWorker::Run() {
    Load();
    time_t t1, t2;

    for (int iter = 0; iter < num_iters_; iter++) {
        if (world_rank_ == MASTER) {
            time(&t1);
            // TODO: send the global update to all workers
        } else {
            // TODO: receive the global update and apply it to local parameter tables
        }

        for (int batch = 0; batch < num_clocks_per_iter_; batch++) {
            unsigned long begin = w.size() * batch / num_clocks_per_iter_;
            unsigned long end = w.size() * (batch + 1) / num_clocks_per_iter_;

            // Loop through each document in the current batch.
            for (unsigned long d = begin; d < end; d++) {
                for (int i = 0; i < w[d].size(); i++) {
                    int word = w[d][i];
                    int topic = z[d][i];
                    docTopicTable[d][topic] -= 1;
                    global_model_.IncWordTopicTable(word, topic, -1);
                    global_model_.IncTopicTable(topic, -1);

                    vector<double> p;
                    double norm = 0.0;
                    for (int k = 0; k < num_topics_; k++) {
                        double ak = docTopicTable[d][k] + alpha_;
                        double bk = (global_model_.GetWordTopicTable(word, k) +
                                     beta_) /
                                    ((double) global_model_.GetTopicTable(k) +
                                     num_words_ * beta_);
                        double pk = ak * bk;
                        p.push_back(pk);
                        norm += pk;
                    }

                    int new_topic = 0;
                    double sum_p_up_to_k = 0.0;
                    double r = ((double) rand() / (RAND_MAX));
                    for (int k = 0; k < num_topics_; k++) {
                        sum_p_up_to_k += p[k] / norm;
                        if (r < sum_p_up_to_k) {
                            new_topic = k;
                            break;
                        }
                    }

                    z[d][i] = new_topic;
                    docTopicTable[d][new_topic] += 1;
                    // wordTopicTable[word][new_topic] += 1;
                    global_model_.IncWordTopicTable(word, new_topic, 1);
                    // topicTable[new_topic] += 1;
                    global_model_.IncTopicTable(new_topic, 1);
                }
            }
            global_model_.Sync();
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

void LdaWorker::Load() {
    string line;
    ifstream file(data_file_);
    if (file.is_open()) {
        int num_line = 0;
        while (getline(file, line)) {
            if (num_line % world_size_ == world_rank_) {
                vector<int> fields;
                unsigned long last_idx = 0;
                for (unsigned long i = 0; i < line.length(); i++) {
                    if (line[i] == ',') {
                        fields.push_back(stoi(line.substr(last_idx + 1,
                                                          i - last_idx - 1)));
                        last_idx = i;
                    }
                }
                w.push_back(fields);
            }
            num_line += 1;
        }
    }
    for (int i = 0; i < w.size(); i++) {
        vector<int> vec;
        for (int j = 0; j < num_topics_; j++) {
            vec.push_back(0);
        }
        docTopicTable.push_back(vec);
    }
    for (int i = 0; i < num_words_; i++) {
        vector<int> vec;
        for (int j = 0; j < num_topics_; j++) {
            vec.push_back(0);
        }
        wordTopicTable.push_back(vec);
    }
    for (int i = 0; i < num_topics_; i++) {
        topicTable.push_back(0);
    }
    return w;
}


void LdaWorker::InitTables() {
    for (int i = 0; i < w.size(); i++) {
        vector<int> fields;
        for (int j = 0; j < w[i].size(); j++) {
            fields.push_back(w[i][j]);
        }
        z.push_back(fields);
    }
    for (int d = 0; d < w.size(); d++) {
        for (int i = 0; i < w[d].size(); i++) {
            int word = w[d][i];
            int topic = rand() % num_topics_;
            z[d][i] = topic;
            docTopicTable[d][topic] += 1;
            global_model_.IncWordTopicTable(word, topic, 1);
            global_model_.IncTopicTable(topic, 1);
        }
    }
}


double LdaWorker::LogDirichlet(vector<double> alpha) {
    double sumLogGamma = 0.0;
    double logSumGamma = 0.0;
    for (double value : alpha) {
        sumLogGamma += lgamma(value);
        logSumGamma += value;
    }
    return sumLogGamma - lgamma(logSumGamma);
}


double LdaWorker::LogDirichlet(double alpha, int k) {
    return k * lgamma(alpha) - lgamma(k * alpha);
}


vector<double> LdaWorker::GetRows(vector<vector<int>> matrix, int column_id) {
    vector<double> rows;
    for (int i = 0; i < num_words_; i++) {
        rows.push_back((double) matrix[i][column_id]);
    }
    return rows;
}


vector<double> LdaWorker::GetColumns(vector<vector<int>> matrix, int row_id) {
    vector<double> cols;
    for (int i = 0; i < num_topics_; i++) {
        cols.push_back((double) matrix[row_id][i]);
    }
    return cols;
}


double LdaWorker::GetLogLikelihood() {
    double lik = 0.0;
    for (int k = 0; k < num_topics_; k++) {
        vector<double> temp = GetRows(wordTopicTable, k);
        for (int w = 0; w < num_words_; w++) {
            temp[w] += alpha_;
        }
        lik += LogDirichlet(temp);
        lik -= LogDirichlet(beta_, num_words_);
    }
    for (int d = 0; d < docTopicTable.size(); d++) {
        vector<double> temp = GetColumns(docTopicTable, d);
        for (int k = 0; k < num_topics_; k++) {
            temp[k] += alpha_;
        }
        lik += LogDirichlet(temp);
        lik -= LogDirichlet(alpha_, num_topics_);
    }
    return lik;
}
