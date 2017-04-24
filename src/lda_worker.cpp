//
// Created by Ye Qi on 20/04/2017.
//

#include "lda_worker.h"

void LdaWorker::run() {
    void run() {
        load(data_file);
        if (world_rank == MASTER) {
            init_tables();
            // TODO: send the whole table to all workers
            time_t t1, t2;
        }
        else {
            // TODO: receive the whole table from the master
        }
        for (int iter = 0; iter < numIterations; iter++) {
            if (world_rank == MASTER) {
                time(&t1);
                // TODO: send the global update to all workers
            }
            else {
                // TODO: receive the global update and apply it to local parameter tables
            }
            for (int batch = 0; batch < numClocksPerIteration; batch++) {
                unsigned long begin = w.size() * batch / numClocksPerIteration;
                unsigned long end = w.size() * (batch + 1) / numClocksPerIteration;

                // Loop through each document in the current batch.
                for (unsigned long d = begin; d < end; d++) {
                    for (int i = 0; i < w[d].size(); i++) {
                        int word = w[d][i];
                        int topic = z[d][i];
                        docTopicTable[d][topic] -= 1;
                        wordTopicTable[word][topic] -= 1;
                        topicTable[topic] -= 1;

                        vector<double> p;
                        double norm = 0.0;
                        for (int k = 0; k < numTopics; k++) {
                            double ak = docTopicTable[d][k] + alpha;
                            double bk = (wordTopicTable[word][k] + beta) /
                                        ((double) topicTable[k] + numWords * beta);
                            double pk = ak * bk;
                            p.push_back(pk);
                            norm += pk;
                        }

                        int new_topic = 0;
                        double sum_p_up_to_k = 0.0;
                        double r = ((double) rand() / (RAND_MAX));
                        for (int k = 0; k < numTopics; k++) {
                            sum_p_up_to_k += p[k] / norm;
                            if (r < sum_p_up_to_k) {
                                new_topic = k;
                                break;
                            }
                        }

                        z[d][i] = new_topic;
                        docTopicTable[d][new_topic] += 1;
                        wordTopicTable[word][new_topic] += 1;
                        topicTable[new_topic] += 1;
                    }
                }
                // TODO: check staleness or sync after each batch
            }
            if (world_rank == MASTER) {
                // TODO: collect all updates from workers
                time(&t2);
                sec[iter] = difftime(t2, t1);
                total_wall_secs += sec[iter];
                log_likelihood[iter] = getLogLikelihood();
                cout << "Iteration time: " << sec[iter] << endl;
                cout << "Log-likelihood: " << llh[iter] << endl;
            }
            else {
                // TODO: send update to master
            }
        }
        if (world_rank == MASTER) {
            ofstream file(outputDir + "/likelihood.csv");
            for (int i = 0; i < numIterations; i++) {
                string s = to_string(i + 1) + "," + to_string(sec[i]) + "," + to_string(llh[i]) + "\n";
                file.write(s.c_str(), s.size());
            }
            file.close();
        }
    }
}

void LdaWorker::load() {
    string line;
    ifstream file (dataFile);
    if (file.is_open()) {
        int num_line = 0;
        while (getline(file, line)) {
            if (num_line % world_size == world_rank) {
                vector<int> fields;
                unsigned long last_idx = 0;
                for (unsigned long i = 0; i < line.length(); i++) {
                    if (line[i] == ',') {
                        fields.push_back(stoi(line.substr(last_idx + 1, i - last_idx - 1)));
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
        for (int j = 0; j < numTopics; j++) vec.push_back(0);
        docTopicTable.push_back(vec);
    }
    for (int i = 0; i < numWords; i++) {
        vector<int> vec;
        for (int j = 0; j < numTopics; j++) vec.push_back(0);
        wordTopicTable.push_back(vec);
    }
    for (int i = 0; i < numTopics; i++) topicTable.push_back(0);
    return w;
}


void LdaWorker::init_tables() {
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
            int topic = rand() % numTopics;
            z[d][i] = topic;
            docTopicTable[d][topic] += 1;
            wordTopicTable[word][topic] += 1;
            topicTable[topic] += 1;
        }
    }
}


double LdaWorker::logDirichlet(vector<double> alpha) {
    double sumLogGamma = 0.0;
    double logSumGamma = 0.0;
    for ( double value : alpha) {
        sumLogGamma += lgamma(value);
        logSumGamma += value;
    }
    return sumLogGamma - lgamma(logSumGamma);
}

double LdaWorker::logDirichlet(double alpha, int k) {
    return k * lgamma(alpha) - lgamma(k * alpha);
}

vector<double> LdaWorker::getRows(vector<vector<int>> matrix, int columnId) {
    vector<double> rows;
    for (int i = 0; i < numWords; i++) {
        rows.push_back((double) matrix[i][columnId]);
    }
    return rows;
}

vector<double> LdaWorker::getColumns(vector<vector<int>> matrix, int rowId) {
    vector<double> cols;
    for (int i = 0; i < numTopics; i++) {
        cols.push_back((double) matrix[rowId][i]);
    }
    return cols;
}


double LdaWorker::getLogLikelihood() {
    double lik = 0.0;
    for (int k = 0; k < numTopics; k++) {
        vector<double> temp = getRows(wordTopicTable, k);
        for (int w = 0; w < numWords; w++) {
            temp[w] += alpha;
        }
        lik += logDirichlet(temp);
        lik -= logDirichlet(beta, numWords);
    }
    for (int d = 0; d < docTopicTable.size(); d++) {
        vector<double> temp = getColumns(docTopicTable, d);
        for (int k = 0; k < numTopics; k++) {
            temp[k] += alpha;
        }
        lik += logDirichlet(temp);
        lik -= logDirichlet(alpha, numTopics);
    }
    return lik;
}
