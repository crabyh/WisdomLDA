#!/bin/bash

# Move to my $SCRATCH directory.
cd $SCRATCH
rm -rf *

execdir=$HOME/WisdomLDA/src  # Directory containing your hello.sh script
exe=lda             # The executable to run
args="20news.csv output 60056 18774 20 0.1 0.1 100 25 0"

# Copy executable to $SCRATCH.
cp -r ${execdir} lda
cd lda

# Run my executable
./${exe} ${args}
