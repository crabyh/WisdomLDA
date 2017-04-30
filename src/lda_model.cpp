//
// Created by Ye Qi on 29/04/2017.
//

#include "lda_model.h"
#define MASTER 0

void GlobalTable::Init() {
    if (world_rank_ == MASTER) {
        // TODO: send word_topic_table_, topic_table_ to all workers
    } else {
        // TODO: receive word_topic_table_, topic_table_ from the master
    }
}

void GlobalTable::Sync() {
    if (world_rank_ == MASTER) {
        // TODO: send the global update to all workers
    } else {
        // TODO: receive the global update and apply it to local parameter tables
    }
}