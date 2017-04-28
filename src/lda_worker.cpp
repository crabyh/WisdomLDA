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

                    double *p = new double[num_topics_];
                    double norm = 0.0;
                    for (int k = 0; k < num_topics_; k++) {
                        double ak = docTopicTable[d][k] + alpha_;
                        double bk = (global_model_.GetWordTopicTable(word, k) +
                                     beta_) /
                                    ((double) global_model_.GetTopicTable(k) +
                                     num_words_ * beta_);
                        double pk = ak * bk;
                        p[k] = pk;
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
                    delete[] p;

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

void load(string dataFile) {
    w = new int*[numDocs];
    docLength = new int[numDocs]();
    string line;
    ifstream file (dataFile);
    if (file.is_open()) {
        int doc = 0;
        while (getline(file, line)) {
            for (unsigned long i = 0; i < line.length(); i++)
                if (line[i] == ',') docLength[doc] += 1;
            int *w_col = new int[docLength[doc]];
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
    docTopicTable = new int*[numDocs];
    for (int i = 0; i < numDocs; i++) {
        int *docLength_col = new int[numTopics]();
        docTopicTable[i] = docLength_col;
    }
    wordTopicTable = new int*[numWords];
    for (int i = 0; i < numWords; i++) {
        int *numWords_col = new int[numTopics]();
        wordTopicTable[i] = numWords_col;
    }
    topicTable = new int[numTopics]();
}

void init_tables() {
    z = new int*[numDocs];
    for (int d = 0; d < numDocs; d++) {
        int *z_col = new int[docLength[d]];
        for (int i = 0; i < docLength[d]; i++) {
            int word = w[d][i];
            int topic = rand() % numTopics;
            z_col[i] = topic;
            docTopicTable[d][topic] += 1;
            wordTopicTable[word][topic] += 1;
            topicTable[topic] += 1;
        }
        z[d] = z_col;
    }
}


double logDirichlet(double *alpha, int length) {
    double sumLogGamma = 0.0;
    double logSumGamma = 0.0;
    for (int i = 0; i < length; i++) {
        sumLogGamma += lgamma(alpha[i]);
        logSumGamma += alpha[i];
    }
    return sumLogGamma - lgamma(logSumGamma);
}

double logDirichlet(double alpha, int k) {
    return k * lgamma(alpha) - lgamma(k * alpha);
}

double *wordTopicTableRows(int columnId) {
    double *rows = new double[numWords];
    for (int i = 0; i < numWords; i++) {
        rows[i] = (double) wordTopicTable[i][columnId];
    }
    return rows;
}

double *docTopicTableCols(int rowId) {
    double *cols = new double[numTopics];
    for (int i = 0; i < numTopics; i++) {
        cols[i] = (double) docTopicTable[rowId][i];
    }
    return cols;
}


double getLogLikelihood() {
    double lik = 0.0;
    for (int k = 0; k < numTopics; k++) {
        double *temp = wordTopicTableRows(k);
        for (int w = 0; w < numWords; w++) {
            temp[w] += alpha;
        }
        lik += logDirichlet(temp, numWords);
        lik -= logDirichlet(beta, numWords);
        delete[] temp;
    }
    for (int d = 0; d < numDocs; d++) {
        double *temp = docTopicTableCols(d);
        for (int k = 0; k < numTopics; k++) {
            temp[k] += alpha;
        }
        lik += logDirichlet(temp, numTopics);
        lik -= logDirichlet(alpha, numTopics);
        delete[] temp;
    }
    return lik;
}