#ifndef GRAPH_SCHED_H
#define GRAPH_SCHED_H

#include "prioq.h"
#include "numa_prioq.h"

typedef pkey_t prio_t;

typedef struct graph_task {
    int     id;
    prio_t  priority;
    int     indegree;
    int     n_deps;
    int    *deps;
} graph_task_t;

// Pluggable queue interface
typedef struct graph_queue_iface {
    void *q; // opaque handle to the underlying queue (pq_t* or numa_prioq_t*)
    void (*insert)(void *q, pkey_t key, pval_t value);
    pval_t (*delete_min)(void *q);
} graph_queue_iface_t;

typedef struct graph_sched {
    int                n_tasks;
    graph_task_t      *tasks;
    graph_queue_iface_t qiface;
} graph_sched_t;

// Constructor helpers
graph_sched_t *graph_sched_create_random_prioq(int n_tasks, int edges_per_task);
graph_sched_t *graph_sched_create_random_numa(int n_tasks, int edges_per_task, int num_nodes);

// Core API
void graph_sched_destroy(graph_sched_t *gs);
void graph_sched_init_ready(graph_sched_t *gs);
int  graph_sched_extract_min_topo(graph_sched_t *gs);
int  graph_sched_task_completed(graph_sched_t *gs, int task_id); // Returns number of enqueued children

#endif
