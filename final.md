## TEAM MEMBERS
- Ye Qi (yeq@andrew.cmu.edu)
- Yuhan Mao (yuhanm@andrew.cmu.edu)

## SUMMARY
We implemented a parallel and distributed version of Latent Dirichlet allocation algorithm on multiple multi-core CPU machines. Basically there will be two levels of optimization: data partition amongst all machines and concurrently Gibbs sampling over local data on each machine.


## BACKGROUND
<!--
What are the key data structures?
What are the key operations on these data structures?
What are the algorithm's inputs and outputs?
What is the part that computationally expensive and could benefit from parallelization?
Break down the workload. Where are the dependencies in the program? How much parallelism is there? Is it data-parallel? Where is the locality? Is it amenable to SIMD execution?
-->

### Overview

#### Key Data Structure

#### Operations

#### Input and Output

### Technical Challenges

#### Workload and Depenencies

#### Locality Analysis

#### Compuationally Expensive Part

### Parallelism Description


Latent Dirichlet Allocation (LDA) [1] is the most commonly used topic modeling approach. While leveraging billions of documents and millions of topics drastically improves the expressiveness of the model, the massive collections challenge the scalability of the inference algorithm for LDA. Below is the graphs providing a very brief idea about how this works[2]:

![Synchronized LDA]({{ site.github.proposal_url }}graph3.png)


Collapesed Gibbs sampling (CGS), known as the solver for LDA, requires a word account table to be held during each iteration. This dependency on preceding data restricts the parallelism of LDA.

The first attempt to parallel this method by Newman, et al. [3] essentially partitions the data collections across multiple workers and has each worker process its local partition. A bulk synchronization on the global word table is executed after every worker finishes its own job in each iteration. As work imbalance is common in such task, the slowest worker leads to the idleness of others which lowers the overall CPU utilization. 

Our job is to design and implement a better parallel algorithm that minimizes the work dependency and communication overhead amongst different workers that eventually achieves good speedups against the number of cores.  


## APPROACH

<!--
Describe the technologies used. What language/APIs? What machines did you target?

Describe how you mapped the problem to your target parallel machine(s). IMPORTANT: How do the data structures and operations you described in part 2 map to machine concepts like cores and threads. (or warps, thread blocks, gangs, etc.)

Did you change the original serial algorithm to enable better mapping to a parallel machine?

If your project involved many iterations of optimization, please describe this process as well. What did you try that did not work? How did you arrive at your solution? The notes you've been writing throughout your project should be helpful here. Convince us you worked hard to arrive at a good solution.

If you started with an existing piece of code, please mention it (and where it came from) here.
-->

### Technolies Used

### Problem Mapping

### Approximation Algorithm

### Proposed Method

We designed two methods to perform LDA with OpenMP. First, they share some same ideas in the basic skeleton. We treat every core as a worker or master. We separate the total documents evenly to all the workers. Every iteration, the worker performs gibbs sampling on their local copy of the parameter tables (specifically, the per-word topic assignment and topic parameter in the up chart) and records the local updates. They Synchronize the parameters tables for a certain times every iteration.

#### Synchronized LDA
All the workers and the master synchronized with each other at the checkpoint (after certain documents). When they are synchronizing, they are blocked until all the workers have the most up-to-dated parameter tables. Below is the chart illustrating this workflow:

![Synchronized LDA]({{ site.github.proposal_url }}graph1.png)

#### Asynchronized LDA
All the workers and the master synchronized with each other at the checkpoint (after certain documents). However, instead of block waiting for the sync to complete, worker will allocate a buffer for the incoming update while continuing perform gibbs sampling on the next trunk of the documents using the current parameter tables. Below is the chart illustrating this workflow:

![Asynchronized LDA]({{ site.github.proposal_url }}graph2.png)

### Speed-up Tricks

### Unsuccessfull Trials (A lot of!!!) and Reasons


## RESULTS AND DISCUSSION
<!--
If your project was optimizing an algorithm, please define how you measured performance. Is it wall-clock time? Speedup? An application specific rate? (e.g., moves per second, images/sec)

Please also describe your experimental setup. What were the size of the inputs? How were requests generated?

Provide graphs of speedup or execute time. Please precisely define the configurations being compared. Is your baseline single-threaded CPU code? It is an optimized parallel implementation for a single CPU?
Recall the importance of problem size. Is it important to report results for different problem sizes for your project? Do different workloads exhibit different execution behavior?

IMPORTANT: What limited your speedup? Is it a lack of parallelism? (dependencies) Communication or synchronization overhead? Data transfer (memory-bound or bus transfer bound). Poor SIMD utilization due to divergence? As you try and answer these questions, we strongly prefer that you provide data and measurements to support your conclusions. If you are merely speculating, please state this explicitly. Performing a solid analysis of your implementation is a good way to pick up credit even if your optimization efforts did not yield the performance you were hoping for.

Deeper analysis: Can you break execution time of your algorithm into a number of distinct components. What percentage of time is spent in each region? Where is there room to improve?

Was your choice of machine target sound? (If you chose a GPU, would a CPU have been a better choice? Or vice versa.)
-->

### Performance Measurement

### Experimental Setup

### Baselines

### Experiments: Convergence

### Experiments: Scalability

### Experiments: Communication Overhead

### Learns Learned

### Room to Improve


## WORK BY EACH STUDENT

Equal work was performed by both project members.


## Reference

[1] Blei, D. M., Ng, A. Y., & Jordan, M. I. (2003). Latent dirichlet allocation. _Journal of machine Learning research, 3_(Jan), 993-1022.

[2] Abdullah Alfadda. (2014). Topic Modeling For Wikipadia Pages. _Spring 2014 ECE 6504 Probabilistic Graphical Models: Class Project_ Virginia Tech

[3] Newman, D., Asuncion, A., Smyth, P., & Welling, M. (2009). Distributed algorithms for topic models. _Journal of Machine Learning Research, 10_(Aug), 1801-1828.

[4] Yu, H. F., Hsieh, C. J., Yun, H., Vishwanathan, S. V. N., & Dhillon, I. S. (2015, May). A scalable asynchronous distributed algorithm for topic modeling. _In Proceedings of the 24th International Conference on World Wide Web (pp. 1340-1350)_. ACM.

[5] Chen, J., Li, K., Zhu, J., & Chen, W. (2016). WarpLDA: a cache efficient O (1) algorithm for latent dirichlet allocation. _Proceedings of the VLDB Endowment, 9_(10), 744-755.
