#ifndef NUMA_PRIOQ_H
#define NUMA_PRIOQ_H

#include "prioq.h"

#define MAX_NUMA_NODES 8

typedef struct {
    int      num_nodes;
    pq_t    *queues[MAX_NUMA_NODES];
} numa_prioq_t;

numa_prioq_t *numa_priq_init(int num_nodes, int max_offset);
void          numa_priq_destroy(numa_prioq_t *q);

void numa_priq_insert(numa_prioq_t *q, pkey_t key, pval_t value);
pval_t numa_priq_delete_min(numa_prioq_t *q);

#endif
