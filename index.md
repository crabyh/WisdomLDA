#### [Ye Qi](https://www.linkedin.com/in/ye-charlotte-qi/), [Yuhan Mao](https://www.linkedin.com/in/yuhan-mao-a09144a5/)

## SUMMARY
We are going to implement a parallel and distributed version of Latent Dirichlet allocation algorithm on multiple multi-core CPU machines. Basically there will be two levels of optimization: data partition amongst all machines and concurently Gibbs sampling over local data on each machine.


## BACKGROUND

Latent Dirichlet Allocation (LDA) [1] is the most commonly used topic modeling approach. While leveraging billions of documents and millions of topics drastically improves the expressiveness of the model, the massive collections challenge the scalability of the inference algorithm for LDA.

Collapesed Gibbs sampling (CGS), known as the solver for LDA, requires a word account table to be held during each iteration. This dependency on preceding data restricts the parallelism of LDA.

The first attempt to parallel this method by Newman, et al. [2] essentially partitions the data collections across multiple workers and has each worker process its local partition. A bulk synchronization on the global word table is executed after every worker finishes its own job in each iteration. As work imbalance is common in such task, the slowest worker leads to the idleness of others which lowers the overall CPU utilization. 

Our job is to design and implement a better parallel algorithm that minimizes the work dependency and communication overhead amongst different workers that eventually achieves good speedups against the number of cores.  


## THE CHALLENGE

1. **Work Assignment**. Concurrently performing Gibbs sampling is where we are going to parallel the LDA algorithm. However, there are dependencies if the concurrent samples in the same word vector or in the same document which causes the accuracy losing of the algorithm. How to reduce these dependencies when partitioning and thus reduce the communication overhead are the key to improving the efficiency.

2. **Staleness vs. Efficiency**. The original sampling algorithm does not fit in the parallel settings so that we could only approximate the results by redesigning an approximation algorithm. There is a trade-off of the accuracy and communication overhead. The more communication between the workers the fresher the parameters but also the higher communication overhead. Achieving a single best solution for all the cases is hard.

3. **Random Memory Access**. Comparing to other applications, the access pattern of CGS is incredibly random. Thus, it's hard to make use of caching techniques. Further optimization may take into account HW-specific settings and demand analysis on memory behavior. The methods introduced by Chen et al. [4] may be of use.

## RESOURCES

### Code Base

We will mainly start the project from scratch. The codes in homework assignment 3 will be used as reference. 

### Papers

The algorithms and frameworks described in [2] and [3] will guide our implementation.
 

## GOALS AND DELIVERABLES### Plan to AchieveWe are going to use the naive single-thread LDA implementation as the baseline. Our distributed parallel asynchronized version is expected to perform at least 100x faster than the baseline on the Intel cluster.### Hope to AchieveIf we are ahead of schedule, we hope to conduct optimization on the kernel level and potentially leverage GPUs to acceralte the computations.### DemoWe are going to run our program on several large document collections to achieve a certain accuracy and compare the wall time to the naive LDA implementation as well as synchronized parallel LDA implementation.

## PLATFROM CHOICE

We are planning to implement the project in C++ with OpenMPI library on the Intel cluster with 68-core Xeon Phi processors. We may need to  debug our project on the GHC machines. 
 

## SCHEDULE

| Time | Task | Status |
|:----:|:----:|:------:|
| Week 1 | Literature survey | Done |
| Week 2 | Design the system achitecture and implement the naive baseline | Pending |
| Week 3 | Implement the synchronized distributed LDA | Pending |
| Week 4 | Modify the synchronized version to the asynchronized one | Pending |
| Week 5 | Collect benchmarking results and conduct the analysis | Pending |


## Reference

[1] Blei, D. M., Ng, A. Y., & Jordan, M. I. (2003). Latent dirichlet allocation. _Journal of machine Learning research, 3_(Jan), 993-1022.

[2] Newman, D., Asuncion, A., Smyth, P., & Welling, M. (2009). Distributed algorithms for topic models. _Journal of Machine Learning Research, 10_(Aug), 1801-1828.

[3] Yu, H. F., Hsieh, C. J., Yun, H., Vishwanathan, S. V. N., & Dhillon, I. S. (2015, May). A scalable asynchronous distributed algorithm for topic modeling. _In Proceedings of the 24th International Conference on World Wide Web (pp. 1340-1350)_. ACM.

[4] Chen, J., Li, K., Zhu, J., & Chen, W. (2016). WarpLDA: a cache efficient O (1) algorithm for latent dirichlet allocation. _Proceedings of the VLDB Endowment, 9_(10), 744-755.
