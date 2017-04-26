#### [Ye Qi](https://www.linkedin.com/in/ye-charlotte-qi/), [Yuhan Mao](https://www.linkedin.com/in/yuhan-mao-a09144a5/)

## REVISED PLAN
<!-- Break time down into half-week increments. Each increment should have at least one task, and for each task put a person's name on it. -->

| Time | Task | Status |
|:----:|:----:|:------:|
| Apr.5 - Apr.9 | Literature survey | Done |
| Apr.10 - Apr.21 | Design the system architecture and implement the naive baseline | Done |
| Apr.22 - Apr.28 | Implement the synchronized distributed LDA | Doing |
| Apr.29 - May.5 | Modify the synchronized version to the asynchronized one | Pending |
| May.6 - May.8 | Collect benchmarking results | Pending |
| May.9 - May.12 | Conduct the analysis and write the report | Pending |


## COMPLETED WORK
<!-- One to two paragraphs, summarize the work that you have completed so far. -->

By far both team members have gained a good understanding of gibbs sampling and implemented a sequential version in C++, which is considered the baseline later. The commonly used [Bag Of Words Dataset](https://archive.ics.uci.edu/ml/datasets/Bag+of+Words) is selected as our testing dataset. The preliminary experiment on a dataset consisting of 18773 documents, 60056 words and 20 topics exhibits a training speed of around 5 secs/iteration. 

Once the baseline was finished, we moved on to data-parallel LDA implementation as described in the proposal. Currently, we have set up the MPI framework and designed API for major operations. A synchronized, distributed implementation, i.e., AD-LDA proposed by Newman et al., was completed under this framework but has not been tested yet. 


## GOALS AND DELIVERABLES
<!-- Do you still believe you will be able to produce all your deliverables? If not, why? What about the "nice to haves"? In your checkpoint writeup we want a new list of goals that you plan to hit for the Parallelism competition.
What do you plan to show at the parallelism competition? Will it be a demo? Will it be a graph? -->

Currently we are a bit behind the schedule because the tasks themselves require some ramp time and the job was not well assigned before the framework was built up, but overall the project has been on the right track. We started with the goal of speeding up the sequential version by 100x. However, as we learned that Intel cluster is not available for testing and introducing staleness in parameters slows down convergence, it might be hard to approach an approximately linear speed up. We decided to lower our goal to achieve a 32x speedup. To achieve this goal, our asynchronized messaging model design is going to refer to some properties of the parameter server. We are also going to incorporate alias table for faster multinomial sampling.

As for the parallelism competition, we will deliver a library that allows users to quickly conduct topic modeling on specified dataset. Since the program may require particularly long time to train, we plan to present the graphs comparing both training time and convergence rate. 


## ISSUES AND CONCERNS
<!-- Are there any remaining unknowns (things you simply don't know how to solve, or resource you don't know how to get) or is it just a matter of coding and doing the work? If you do not wish to put this information on a public web site you are welcome to email the staff directly. -->
One generic issue for such machine learning techniques is that the program may still function well even if there exists tiny errors. A correctness check against labelled data is required to assure that the system could produce results of acceptible accuracy. 

There is a trade-off between convergence and parallelism. Thus, it's difficult to optimize the performance in terms of the overall running time.



