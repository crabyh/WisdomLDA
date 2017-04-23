//
// Created by Ye Qi on 20/04/2017.
//

#ifndef WISDOMLDA_LDA_MODEL_H
#define WISDOMLDA_LDA_MODEL_H

#include <mpi.h>
#include <vector>

using namespace std;

class DistModel {
public:
    vector<vector<int>> w;
    vector<vector<int>> z;
    vector<vector<int>> docTopicTable;
    vector<vector<int>> wordTopicTable;
    vector<int> topicTable;

    DistModel(){}
};


#endif //WISDOMLDA_LDA_MODEL_H
