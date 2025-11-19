# System Programming Lab 11 Multiprocessing
Implementation was done by creating a for loop that would create multiple forks for as many processes you wish.
Each process was given a process number, using this and the number of processes total a workload is given
for each process that is a range of images it must generate.

## Runtime Analysis

| Processes | Time   |
|-----------|--------|
| 1         | 2m 43s |
| 2         | 1m 30s |
| 5         | 1m 12s |
| 10        | 0m 49s |
| 20        | 0m 42s |
| 30        | 0m 41s |
| 40        | 0m 38s |
| 50        | 0m 35s |

![Graph of results](Results.png)

## Analysis
Every workload has a limit where adding more processes will not stop decreasing time.
As the graph and data shows, the closer the count gets to 50 processes the less and less
time that is saved by using that many processes. It should also be noted that when the process
count hits the number of cores in the CPU the parallelism no longer works as processes will start
being assigned to the same core, where each core is quickly switching between all the threads
it has running on it (async).
