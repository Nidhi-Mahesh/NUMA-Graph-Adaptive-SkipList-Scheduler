#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include "numa_prioq.h"
#include "common.h"

/* Pseudo-NUMA helper: maps thread to a node ID based on pthread_self() */
static int get_pseudo_node_id(int num_nodes) {
    pthread_t tid = pthread_self();
    unsigned long x = (unsigned long)tid;
    return (int)(x % num_nodes);
}

numa_prioq_t *numa_priq_init(int num_nodes, int max_offset) {
    numa_prioq_t *q;
    
    /* Clamp num_nodes to valid range */
    if (num_nodes < 1) num_nodes = 1;
    if (num_nodes > MAX_NUMA_NODES) num_nodes = MAX_NUMA_NODES;
    
    /* Allocate wrapper structure */
    E_NULL(q = (numa_prioq_t *)malloc(sizeof(numa_prioq_t)));
    memset(q, 0, sizeof(numa_prioq_t));
    
    q->num_nodes = num_nodes;
    
    /* Initialize one priority queue per NUMA node */
    for (int i = 0; i < num_nodes; i++) {
        q->queues[i] = pq_init(max_offset);
        if (q->queues[i] == NULL) {
            /* Cleanup on failure */
            for (int j = 0; j < i; j++) {
                pq_destroy(q->queues[j]);
            }
            free(q);
            return NULL;
        }
    }
    
    return q;
}

void numa_priq_destroy(numa_prioq_t *q) {
    if (q == NULL) return;
    
    /* Destroy all per-node queues */
    for (int i = 0; i < q->num_nodes; i++) {
        if (q->queues[i] != NULL) {
            pq_destroy(q->queues[i]);
        }
    }
    
    free(q);
}

void numa_priq_insert(numa_prioq_t *q, pkey_t key, pval_t value) {
    int node = get_pseudo_node_id(q->num_nodes);
    insert(q->queues[node], key, value);
}

pval_t numa_priq_delete_min(numa_prioq_t *q) {
    int node = get_pseudo_node_id(q->num_nodes);
    pval_t result;
    
    /* Try local queue first */
    result = deletemin(q->queues[node]);
    if (result != NULL) {
        return result;
    }
    
    /* If local queue is empty, try other queues */
    for (int i = 0; i < q->num_nodes; i++) {
        if (i == node) continue; /* Already tried local queue */
        
        result = deletemin(q->queues[i]);
        if (result != NULL) {
            return result;
        }
    }
    
    /* All queues are empty */
    return NULL;
}
