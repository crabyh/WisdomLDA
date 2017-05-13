## TEAM MEMBERS
- Ye Qi (yeq@andrew.cmu.edu)
- Yuhan Mao (yuhanm@andrew.cmu.edu)

## SUMMARY
We implemented a parallel and distributed version of Latent Dirichlet allocation algorithm on multiple multi-core CPU machines. Basically there will be two levels of optimization: data partition amongst all machines and concurrently Gibbs sampling over local data on each machine.


## BACKGROUND

### Overview

Latent Dirichlet Allocation (LDA) [1] is the most commonly used topic modeling approach. While leveraging billions of documents and millions of topics drastically improves the expressiveness of the model, the massive collections challenge the scalability of the inference algorithm for LDA. 

Below is the conventions used in this report.

| Symbol |                 Description                |   Range   |
|:------:|:------------------------------------------:|:---------:|
|   _N_  |    Corpus size (number of all the terms)   | 15M+      |
|   _W_  | Vocabulary size (number of distinct terms) | 15K+      |
|   _K_  |           Number of latent topics          | 10 - 3000 |
|   _D_  |             Number of documents            | 30K+      |
|   _T_  |             Number of iterations           | 1K+       |


#### Input and Output

The brief idea of Collapsed Gibbs sampling (CGS), known as the solver for LDA, is shown in the graphs [2] as follows :

![Input and Output]({{ site.github.proposal_url }}img/graph3.png)

This unsupervised generative algorithm starts off by taking a collection of documents as input, in which each document is represented as a stream of word tokens. The output is a word-topic distribution table and a document-topic distribution table that could be used to predict the topic of a given document.

#### Key Data Structure

The data structure involved here are fairly straightforward:

- Document-topic distribution table: a two-dimensional array of size _DK_
- Document-word-topic assignment table: a two-dimensional array of size _DW_
- Word-topic distribution table: a two-dimensional array of size _WK_
- Topic distribution table: a one-dimensional array of size _K_

According to the common range of each parameter, the topic distribution table is trivial to store. It's the document-topic distribution and the document-word-topic assignment table, as well as the word-topic distribution table the real killers to traverse and to communicate with. The upper bound of _W_ is limited by the Zipf's Law in English languages. However, _D_ could reach a much higher value, especially in large-scale data analysis.

#### Operations

The algorithm essentially iterates over all the word occurrences in all the documents and updates the word-topic, document-topic and topic count tables in the meanwhile. The pseudocode is as follows:

~~~
for d in document collection:
    for w in d:
        sample t ~ multinomial(1/K)
        stored t to document-word-topic assignment table
        increment corresponding entries of t in the word-topic, document-topic and topic count tables 
while not converge and t <= T:
    for d in document collection:
        for w in document:
            get t from the document-word-topic assignment table
            decrement corresponding entries of t in the word-topic, document-topic and topic count tables 
            calculate posterior p over topic
            sample t' ~ multinomial(p)
            increment corresponding entries of t' in the word-topic, document-topic and topic count tables 
~~~

### Technical Challenges

The largest challenge is that Gibbs sampling by definition is a strictly sequential algorithm in that each update depends on previous updates. In another word, the topic assignment of word w in document d cannot be performed concurrently with that of word w' in document d'. 

#### Workload and Dependencies

As we noticed that the topic assignment of a word depends more on the topic of other words in the same documents (_n\_d,k_) than those elsewhere in the corpus (word-topic distribution _n\_k,w_ and topic distribution _n\_k_) because these probabilities, as calculated following, only change slowly.

![Workload and Dependencies]({{ site.github.proposal_url }}img/formula.jpg)


Thus, it's possible to release the strict dependency and approximate this sampling process by randomly assigning documents to _p_ processors, then have each processor performed local sampling process and merge updates after a period of time. Newman, et al [3] have already investigated the effects of deferred update, and they found it will net correctness of the Gibbs sampling, but only results in slightly slower convergence.

![Workload and Dependencies]({{ site.github.proposal_url }}img/ad-lda.jpg)

#### Locality Analysis

Identifying locality is crucial in a parallel program. However, the randomicity of Gibbs sampling prevents us from utilizing SIMD operations or any elaborate caching strategies. 

The sampling for a single word w takes _O(K)_ time, but the actual time could not be easily predicted. This divergence in instruction stream results in weak SIMD utilization.

As for data structure access pattern, both word-topic table and document-topic table are accessed row by row. However, the columns inside each row are accessed randomly submit to a multinomial distribution. 

If the vocabulary size is sufficiently greater than the number of processors, the probability that two processors access the same word entry is small, so false sharing is not likely to happen in our case.

#### Compuationally Expensive Part

This algorithm is computationally intensive when _K_ is not too small. For each inner loop, there are 6 loads and stores and _6 + 6K + O(K)_ computations. And a single loop will be repeated for _NT_ times.   

### Parallelism Description

In this project, we leveraged the data-parallel model to partition the document collection into p subsets and had them sent to p workers. A worker could be either a process on a machine or a machine on a cluster. p workers could perform Gibbs sampling concurrently. Since document-word-topic assignment table and document-topic distribution table are only related to the documents lied on this worker, there's no need to exchange them. Topic distribution table and word-topic distribution table, however, should be considered shared across all the workers and require synchronization.

As a result,the time complexity and space complexity become:

|       |    Sequential    |       Parallel      |
|:-----:|:----------------:|:-------------------:|
| Space |  _N + K (D + W)_ | _(N + KD) / P + KW_ |
|  Time |      _NK_        |  _NK / P + KW + C_  |

_KW + C_ is considered the communication overhead. We tried our best efforts to minimize this overhead while maintaining reasonable converge rate.

## APPROACH

<!--
Describe the technologies used. What language/APIs? What machines did you target?

Describe how you mapped the problem to your target parallel machine(s). IMPORTANT: How do the data structures and operations you described in part 2 map to machine concepts like cores and threads. (or warps, thread blocks, gangs, etc.)

Did you change the original serial algorithm to enable better mapping to a parallel machine?

If your project involved many iterations of optimization, please describe this process as well. What did you try that did not work? How did you arrive at your solution? The notes you've been writing throughout your project should be helpful here. Convince us you worked hard to arrive at a good solution.

If you started with an existing piece of code, please mention it (and where it came from) here.
-->

### Technologies Used

*MPI:* Message Passing Interface is a standardized and portable message-passing system designed by a group of researchers from academia and industry to function on a wide variety of parallel computing architectures. The standard defines the syntax and semantics of a core of library routines useful to a wide range of users writing portable message-passing programs in C, C++, and Fortran.  

### Problem Mapping

First, we map a worker or master to a process. 

A worker -> a process who does the real work in the LDA algorithm we talked above -- Gibbs Sampling;
A master -> a process to coordinate the communication amongst workers, maintaining the per-word topic assignment and topic parameter in the up chart as well as calculate the log-likelihood after each iteration to evaluate the convergence of the algorithm. 

 Since the process can be assigned to the same machine or different machines and so as MPI, our algorithm can be easily distributed to multiple machines with minor or no modification. Second, we separate the entire documents randomly to all the workers. In each iteration, the worker performs Gibbs sampling on their local copy of the parameter tables and records the updates. They Synchronize these local parameters tables with the master every particular time.


### Approximation Algorithm

The algorithm has a strict-sequential dependency for every step of the Gibbs Sampling. However, due to the size of the table space and the characteristic of the algorithm, there is a very low chance of conflict. And more importantly, the conflict of the updates may not impact the correctness of the algorithm. Because the sampling itself is a random process, staleness of the parameters only leads to slower convergence. This effect has even trial influence when there is a large topic number (e.g. 1000), so we decide to make it hard for our algorithm and limit our experiments to 20 topics.

### Proposed Method

We implemented the distributed and parallel version of LDA from scratch in C++ with the support of MPI. The following graph scatches our pipeline.

![Proposed Method]({{ site.github.proposal_url }}img/plda-diagram.png)

The synchronization is carried out at every checkpoint. Our program supports setting up the checkpoint after processing a certain amount of documents. It could be within the same iteration (i.e. every 1/10 iteration) or after several iterations. This is a parameter to be tuned to achieve a trade-off between efficiency and convergence.

#### Synchronized LDA

![Synchronized LDA]({{ site.github.proposal_url }}img/sync.jpg)

This is a simple algorithm proposed by Newman. In this setting, every worker, along with the master synchronizes their parameters at every checkpoint. When they are sending messages, all the workers are blocked until all of them have the most up-to-dated parameter tables back from the master. Below is the chart illustrating this workflow:

![Synchronized LDA]({{ site.github.proposal_url }}img/graph1.png)

#### Asynchronized LDA

![Synchronized LDA]({{ site.github.proposal_url }}img/async.jpg)

The bottleneck for the synchronized LDA is the synchronization. As the chart shown above, all the workers need to wait the slowest worker to finish its job before stepping into next stage. Because the variation of the machine status and the impossible of distributing work absolute even, some time are wasted. Things become even worse when scaling up, the slowest worker will encumber all the workers.
The solution here is to perform the communication whenever the worker reach its own check point (e.g. perform Gibbs Sampling on a certain amount of documents) it will communicate with the master to perform its update to the delta table and acquiring the most up-to-dated global table. However, because the master no longer knows the worker's state or which iteration the worker is current at, merging parameter tables can only be achieved with delta table (i.e. the change of the parameters table since last communication). Although the up-to-dated global parameter table still can be transferred back as the synchronized version. Thus, compared to synchronized version, workers need to do a little bit extra work -- updating both parameter tables and delta table during the Gibbs Sampling. 


![Asynchronized LDA]({{ site.github.proposal_url }}img/graph2.png)

### Unsuccessful Trials (A lot of!!!)

#### Delta Table

We attempted to use a delta table to communicate with workers and master, i.e. workers send their delta table since the last synchronization to the master and master merges all the delta tables of the workers and send the merged(global) one back to all the workers. The advantages of this approach are the delta table can be compressed to reduce the message size for sparse delta parameters tables. However, it needs an extra iteration of the word topic table to apply the global update to the local parameters table when the worker receives the global delta table.

#### Sparse Matrix Representation

While frequent communications maybe expected sometimes (when checkpoint = every 100 documents), we thought it might be better to transfer only the updated indices instead of the raw word topic tables. We utilized an unordered hashmap to store the updates between each two iterations. The higher 24 bits of the key represents the word while the low 8 bits represents the topic. The value denotes the corresponding word-topic count. This is designed to reduce the communication overhead by compressing the delta table to a smaller size. 

The performance is not ideal because of the power low between corpus size and vocabulary, which is to say even the number of documents is not large, the probability that we see most of the words is still pretty high. When the number of topics is not large, the merged word-topic table is still rather dense because the tables from different workers vary a lot since they are generated from different datasets and parameters. In our setting (K = 20), the overhead for marshaling and unmarshalling counteracts the time saved in transferring data.

This trick hopefully works well for larger topic number (K >= 1000). However, larger topic number requires much more computations during each iteration. The machines available for us cannot be used to conduct long-term training. 

#### Non-blocking Communication

We've also tried using an asynchronized way to handle the receiving global update table for the workers. The original intention for this approach is to hide the latency of the synchronization. I.e. instead of block waiting for the master to process the merge and send the global table back, worker can still process the Gibbs Sampling with its current parameter table. But the price we pay for this is we need another piece of memory to store the incoming global tables and has to check if the incoming tables have been received regularly. Again, this methods should provide better performance for larger word topic table (larger K and vocabulary size) since the communication time is longer and worth to be hidden, but has trivial impact on our current settings. 
 

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

We use Log-likelihood to measure the convergence of the LDA. Since the algorithm is exactly the same as the sequential one, and it always converges to the same place, we focus on improving the speed rather than something else(e.g. accuracy at the converge point). 

### Experimental Setup

<<<<<<< HEAD
The sanity check is against 20news Dataset [6] which includes 18,774 documents with a vocabulary size of 60,056.

The thorough experiments are performed on NYTimes corpus from UCI Machine Learning Repository[7], which consists of 102,660 documents and a vocabulary size of 299,752. The total number of word occurences is around 100,000,000. 

Other setthings for Gibbs sampling are lists as follow:

| Parameter | Value |
|:---------:|:-----:|
|     K     |   20  |
|     α     |  0.1  |
|     β     |  0.1  |
| checkpoint (synchronize every) | 10,000 |

_K_ was set to a relatively small number because 1) we want to explore the parallel Gibbs sampling on rather dense word-topic tables; 2) limit training time so that we could produce comprehensive evaluation.

We ran both the synchronized and asynchrnoized version of LDA program using from 2 cores up to 16 cores on GHC machines, which has 8 physical cores (2 hyper-threads) 3.2 GHz Intel Core i7 processors. 

Though the program also supports distribution across machines on the cluster, we chose to do our experiments only using multi-core configurations because the dataset is not large enough to let the speedup from parallelism overwhelm the latency of transferring large buffer using network.

### Baselines

Our baseline is the sequential version of the LDA program on C++.

#### Convergence

![Convergence]({{ site.github.proposal_url }}img/sync-converge.jpg)

![Convergence]({{ site.github.proposal_url }}img/async-converge.jpg)

#### Scalability

![Scalability]({{ site.github.proposal_url }}img/ghc.jpg)

#### Communication Overhead

![Communication Overhead]({{ site.github.proposal_url }}img/comm.jpg)

### Additional Experiments: AWS

As the reasoning illustrated in the previous section, we decided to try our algorithm on a larger machine to see if our guess holds. We tried submitted multiple time on the Latedays cluster but unfortunately for some reason our jobs were killed before we can get enough experiment results to do the further analysis. We turned to AWS for help. Spent some money, we launched a m4.16xlarge instance with the Intel Xeon E5-2686 v4 CPU (http://ark.intel.com/products/91317/Intel-Xeon-Processor-E5-2699-v4-55M-Cache-2_20-GHz). AWS provided 64 vCPU for this type of instance.

Beside the change of the machine, we also reduced the communication times by increasing the tunable parameter documents per synchronization from 10000 to 50000. One reason is the observation of the increasing communication time ratio when the number of the workers increases. Another reason is in the previous experiments, our algorithm with multiple workers convergences as well as a single worker (equivalent to sequential LDA). So to make it even more scalable, we can sacrifice some convergence along the way.

Here is the results we have:

![Synchronized LDA]({{ site.github.proposal_url }}img/aws.jpg)

By decreasing times of the synchronization, both the synchronized and asynchronized version can achieve a even better speedup compared to the previous experiment with 16 cores or less, which is a almost linear speedup. Besides, the speedup achieved on AWS on 16 cores compared to that on the GHC machine provided our guess that the hyper-threading is the reason for the unsatisfying performance for 16 workers on GHC machine.

And still, when scaling up, asychronized version outperforms the synchronized one because it requires less synchronization time and their synchronization time will not increases with the increasing of the number of the workers.

### Take Away

### Future Work

1. From our observation, stale parameters influence influences more on the initiate stage of the Gibbs Sampling because the parameter tables change much more at the beginning and reach to a stable stage afterwords. Thus, a potential optimization would be decrease the communication frequency over the time to achieve a higher speedup.

2. For non-blocking communication mentioned in previous section, instead of clear the memory after each communication, it is possible to use the pointer-swap trick to avoid the unnecessary memory operation. Though every worker still need extra space to store a copy of the word topic table.

## WORK BY EACH STUDENT

Equal work was performed by both project members.

Yuhan implemented the first version sequential version of LDA as the baseline. Ye referred to a lot of related papers to figure out the direction and did the configuration of run the MPI on both GHC as well as Latedays. And together, we finished our first version of the synchronized LDA using MPI.

While the first experiment turned not satisfying, we worked together to optimize and debug our code. Ye tried the sparse matrix and Yuhan adjusted the synchronized version to asynchronized one. After the coding part is done ,we ran our experiments, did the analysis and wrote the report together.

## Reference

[1] Blei, D. M., Ng, A. Y., & Jordan, M. I. (2003). Latent dirichlet allocation. _Journal of machine Learning research, 3_(Jan), 993-1022.

[2] Abdullah Alfadda. (2014). Topic Modeling For Wikipadia Pages. _Spring 2014 ECE 6504 Probabilistic Graphical Models: Class Project_ Virginia Tech

[3] Newman, D., Asuncion, A., Smyth, P., & Welling, M. (2009). Distributed algorithms for topic models. _Journal of Machine Learning Research, 10_(Aug), 1801-1828.

[4] Yu, H. F., Hsieh, C. J., Yun, H., Vishwanathan, S. V. N., & Dhillon, I. S. (2015, May). A scalable asynchronous distributed algorithm for topic modeling. _In Proceedings of the 24th International Conference on World Wide Web (pp. 1340-1350)_. ACM.

[5] Chen, J., Li, K., Zhu, J., & Chen, W. (2016). WarpLDA: a cache efficient O (1) algorithm for latent dirichlet allocation. _Proceedings of the VLDB Endowment, 9_(10), 744-755.

[6] https://www.open-mpi.org/

[7] http://qwone.com/~jason/20Newsgroups/

[7] https://archive.ics.uci.edu/ml/datasets/bag+of+words

