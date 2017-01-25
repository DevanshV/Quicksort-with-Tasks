# Quicksort

This project includes a Quicksort implementation that generates seperate tasks for sorting each recursive partition.

### Quicksort Steps

* Choose any element of the array to be the pivot.
* Divide all other elements (except the pivot) into two partitions.
* All elements less than the pivot must be in the first partition.
* All elements greater than the pivot must be in the second partition.
* Use recursion to sort both partitions. 
* Join the first sorted partition, the pivot, and the second sorted partition.

The code has two separate implementations of quicksort, that differ in the way synchronization is achieved

`quicksort_sem();` Uses semaphores to synchronize child tasks (forced serialization)

`quicksort();` Uses task priorities for synchronization (increases priority for children tasks)
