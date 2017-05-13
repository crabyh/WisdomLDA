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
|   $N$  |    Corpus size (number of all the terms)   | 15M+      |
|   $W$  | Vocabulary size (number of distinct terms) | 15K+      |
|   $K$  |           Number of latent topics          | 10 - 3000 |
|   $D$  |             Number of documents            | 30K+      |
|   $T$  |             Number of iterations           | 1K+       |


#### Input and Output

The brief idea of Collapsed Gibbs sampling (CGS), known as the solver for LDA, is shown in the graphs [2] as follows :

![Synchronized LDA]({{ site.github.proposal_url }}graph3.png)

This unsupervised generative algorithm starts off by taking a collection of documents as input, in which each document is represented as a stream of word tokens. The output is a word-topic distribution table and a document-topic distribution table that could be used to predict the topic of a given document.

#### Key Data Structure

The data structure involved here are fairly straightforward:

- Document-topic distribution table: a two-dimensional array of size $D \times K$
- Document-word-topic assignment table: a two-dimensional array of size $D \times W$
- Word-topic distribution table: a two-dimensional array of size $W \times K$
- Topic distribution table: a one-dimensional array of size $K$ 

According to the common range of each parameter, the topic distribution table is trivial to store. It's the document-topic distribution and the document-word-topic assignment table, as well as the word-topic distribution table the real killers to traverse and to communicate with. The upper bound of $W$ is limited by the Zipf's Law in English languages. However, $D$ could reach a much higher value, especially in large-scale data analysis.

#### Operations

The algorithm essentially iterates over all the word occurrences in all the documents and updates the word-topic, document-topic and topic count tables in the meanwhile. The pseudocode is as follows:

~~~python
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

As we noticed that the topic assignment of a word depends more on the topic of other words in the same documents ($n_{d,k}$) than those elsewhere in the corpus (word-topic distribution $n_{k,w}$ and topic distribution $n_k$) because these probabilities, as calculated following, only change slowly.

$p(z = k|\cdot) = \frac{n_{k,w}+\beta}{n_k+\beta{W}} (n_{d,k}+\alpha)$ 

Thus, it's possible to release the strict dependency and approximate this sampling process by randomly assigning documents to $p$ processors, then have each processor performed local sampling process and merge updates after a period of time. Newman, et al [3] have already investigated the effects of deferred update, and they found it will net correctness of the Gibbs sampling, but only results in slightly slower convergence.

![Synchronized LDA]({{ site.github.proposal_url }}ad-lda.jpg)

#### Locality Analysis

Identifying locality is crucial in a parallel program. However, the randomicity of Gibbs sampling prevents us from utilizing SIMD operations or any elaborate caching strategies. 

The sampling for a single word w takes $O(K)$ time, but the actual time could not be easily predicted. This divergence in instruction stream results in weak SIMD utilization.

As for data structure access pattern, both word-topic table and document-topic table are accessed row by row. However, the columns inside each row are accessed randomly submit to a multinomial distribution. 

If the vocabulary size is sufficiently greater than the number of processors, the probability that two processors access the same word entry is small, so false sharing is not likely to happen in our case.

#### Compuationally Expensive Part

This algorithm is computationally intensive when $K$ is not too small. For each inner loop, there are 6 loads and stores and $6 + 6 * K + O(K)$ computations. And a single loop will be repeated for $N * T$ times.   

### Parallelism Description

In this project, we leveraged the data-parallel model to partition the document collection into p subsets and had them sent to p workers. A worker could be either a process on a machine or a machine on a cluster. p workers could perform Gibbs sampling concurrently. Since document-word-topic assignment table and document-topic distribution table are only related to the documents lied on this worker, there's no need to exchange them. Topic distribution table and word-topic distribution table, however, should be considered shared across all the workers and require synchronization.

As a result,the time complexity and space complexity become:

|       |  Sequential  |               Parallel              |
|:-----:|:------------:|:-----------------------------------:|
| Space |  $N+K(D+W)$  | $\frac{N+K \times D}{P}+K \times W$ |
|  Time | $N \times K$ |     $\frac{NK}{P}+K \times W+C$     |

$K \times W+C$ is considered the communication overhead. We tried our best efforts to minimize this overhead while maintaining reasonable converge rate.

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

First, we map a worker or master to a process. A worker is mapped to a process who does the real work in the LDA algorithm we talked above -- Gibbs Sampling; A master is also mapped to a process to coordinate the communication amongst workers, maintains the per-word topic assignment and topic parameter in the up chart as well as calculate the log-likelyhood after each iteration to evaluate the convergence of the algorithm.  Since the process can be assigned to the same machine or different machine and so as MPI, our algorithm can be easily distributed to multiple machines with minor or no modification. Second, we separate the total documents randomly to all the workers. In each iteration, the worker performs gibbs sampling on their local copy of the parameter tables and records the updates. They Synchronize these local parameters tables with the master every certain time.


### Approximation Algorithm

The algorithm has strict-sequential dependency for every step of the Gibbs Sampling. However, due to the size of the table space and the characteristic of the algorithm, there is a very low chance of conflict. And more importantly, the conflict of the updates may not impact the correctness of the algorithm. Because the sampling itself is a random process, staleness of the parameters only leads to slower convergence. This effect has even trial influence when there are large topic number (e.g. 1000), so we decide to make it hard for our algorithm and limit our experiments to 20 topics.


### Proposed Method

We adopted the java sequential version of the LDA code from course 10605 homework 7 _Gibbs Sampling LDA using a Parameter Server_.

#### Synchronized LDA
All the workers and the master synchronized with each other at the checkpoint (after certain documents). When they are synchronizing, all the workers are blocked until all of them have the most up-to-dated parameter tables. Below is the chart illustrating this workflow:

![Synchronized LDA]({{ site.github.proposal_url }}graph1.png)

#### Asynchronized LDA
All the workers and the master synchronized with each other at the checkpoint (after certain documents). However, instead of block waiting for the sync to complete, worker will allocate a buffer for the incoming update while continuing perform gibbs sampling on the next trunk of the documents using the current parameter tables. Below is the chart illustrating this workflow:

![Asynchronized LDA]({{ site.github.proposal_url }}graph2.png)

### Speed-up Tricks
1. All reduce in the synchronized version.
2. Using OpenMP in the Gibbs Sampling part to use the available sources. (it is turned off while running on the GHC machine)

### Trials (A lot of!!!) and Analysis
1. Instead of transferring the raw word topic delta tables every time. We uses some sparse matrix representation to compress the delta table, which is intended to reduce the communication overhead. However, due to the density of the word topic table varies a lot for different datasets and parameters(especially topic number K), this trick works well for larger topic number while is not so helpful in our experiment scenario (K=20).

2. We tried using a delta table to communicate with workers and master, i.e. workers send their delta table since last synchronization to the master and master merges all the delta tables of the workers and send the merged(global) one back to all the workers. The advantages of this approach is the delta table can be compressed to reduce the message size for sparse delta parameters tables. However, it need an extra iteration of the word topic table to apply the global update to the local parameters table when the worker receive the global delta table.

3. We've also tried using a asynchronized way to handle the receiving global update table for the workers. The original intention for this approach is to hide the latency of the synchronization. I.e. instead of block waiting for the master to process the merge and send the global table back, worker can still process the Gibbs Sampling with its current parameter table. But the price we pay for this is we need another piece of memory to store the incoming global tables and has to check if the incoming tables have been received regularly. Again, this methods should provide better performance for larger word topic table (larger K and vocabulary size) since the communication time is longer and worth to be hidden, but has trivial impact on our current settings. 
 


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

We use Log

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


https://en.wikipedia.org/wiki/Message_Passing_Interface
