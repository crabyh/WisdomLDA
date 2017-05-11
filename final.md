#### [Ye Qi](https://www.linkedin.com/in/ye-charlotte-qi/), [Yuhan Mao](https://www.linkedin.com/in/yuhan-mao-a09144a5/)

## SUMMARY
We are going to implement a parallel and distributed version of Latent Dirichlet allocation algorithm on multiple multi-core CPU machines. Basically there will be two levels of optimization: data partition amongst all machines and concurrently Gibbs sampling over local data on each machine.


## BACKGROUND

Latent Dirichlet Allocation (LDA) [1] is the most commonly used topic modeling approach. While leveraging billions of documents and millions of topics drastically improves the expressiveness of the model, the massive collections challenge the scalability of the inference algorithm for LDA. Below is the graphs providing a very brief idea about how this works[2]:

![Synchronized LDA]({{ site.github.proposal_url }}graph3.png)


Collapesed Gibbs sampling (CGS), known as the solver for LDA, requires a word account table to be held during each iteration. This dependency on preceding data restricts the parallelism of LDA.

The first attempt to parallel this method by Newman, et al. [3] essentially partitions the data collections across multiple workers and has each worker process its local partition. A bulk synchronization on the global word table is executed after every worker finishes its own job in each iteration. As work imbalance is common in such task, the slowest worker leads to the idleness of others which lowers the overall CPU utilization. 

Our job is to design and implement a better parallel algorithm that minimizes the work dependency and communication overhead amongst different workers that eventually achieves good speedups against the number of cores.  


## APPROACH

### MAIN IDEA

We designed two methods to perform LDA with OpenMP. First, they share some same ideas in the basic skeleton. We treat every core as a worker or master. We separate the total documents evenly to all the workers. Every iteration, the worker performs gibbs sampling on their local copy of the parameter tables (specifically, the per-word topic assignment and topic parameter in the up chart) and records the local updates. They Synchronize the parameters tables for a certain times every iteration.

1. Synchronized LDA: All the workers and the master synchronized with each other at the checkpoint (after certain documents). When they are synchronizing, they are blocked until all the workers have the most up-to-dated parameter tables. Below is the chart illustrating this workflow:

![Synchronized LDA]({{ site.github.proposal_url }}graph1.png)

2. Asynchronized LDA: All the workers and the master synchronized with each other at the checkpoint (after certain documents). However, instead of block waiting for the sync to complete, worker will allocate a buffer for the incoming update while continuing perform gibbs sampling on the next trunk of the documents using the current parameter tables. Below is the chart illustrating this workflow:

![Asynchronized LDA]({{ site.github.proposal_url }}graph2.png)

### SUPPLEMENTS

## RESULTS




## Reference

[1] Blei, D. M., Ng, A. Y., & Jordan, M. I. (2003). Latent dirichlet allocation. _Journal of machine Learning research, 3_(Jan), 993-1022.

[2] Abdullah Alfadda. (2014). Topic Modeling For Wikipadia Pages. _Spring 2014 ECE 6504 Probabilistic Graphical Models: Class Project_ Virginia Tech

[3] Newman, D., Asuncion, A., Smyth, P., & Welling, M. (2009). Distributed algorithms for topic models. _Journal of Machine Learning Research, 10_(Aug), 1801-1828.

[4] Yu, H. F., Hsieh, C. J., Yun, H., Vishwanathan, S. V. N., & Dhillon, I. S. (2015, May). A scalable asynchronous distributed algorithm for topic modeling. _In Proceedings of the 24th International Conference on World Wide Web (pp. 1340-1350)_. ACM.

[5] Chen, J., Li, K., Zhu, J., & Chen, W. (2016). WarpLDA: a cache efficient O (1) algorithm for latent dirichlet allocation. _Proceedings of the VLDB Endowment, 9_(10), 744-755.