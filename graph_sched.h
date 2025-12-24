#ifndef GRAPH_SCHED_H
#define GRAPH_SCHED_H

#include "prioq.h"   // use the existing lock-free priority queue

typedef pkey_t prio_t;  // alias for compatibility

typedef struct graph_task {
    int     id;        // task ID: 0..n_tasks-1
    prio_t  priority;  // priority key used in the PQ
    int     indegree;  // number of unfinished parents
    int     n_deps;    // number of outgoing edges (children)
    int    *deps;      // array of child IDs
} graph_task_t;

typedef struct graph_sched {
    int           n_tasks;
    graph_task_t *tasks;
    pq_t         *ready_q;    // underlying priority queue
} graph_sched_t;

// Build a random DAG with n_tasks nodes and approx edges_per_task outgoing edges per node.
graph_sched_t *graph_sched_create_random(int n_tasks, int edges_per_task);

// Free all allocated memory in the graph scheduler.
void graph_sched_destroy(graph_sched_t *gs);

// Insert all initial indegree==0 tasks into ready_q.
void graph_sched_init_ready(graph_sched_t *gs);

// Topological deleteMin: get next runnable task ID, or -1 if none.
int graph_sched_extract_min_topo(graph_sched_t *gs);

// Mark task_id as completed, update children indegree, and enqueue children whose indegree becomes 0.
void graph_sched_task_completed(graph_sched_t *gs, int task_id);

#endif
