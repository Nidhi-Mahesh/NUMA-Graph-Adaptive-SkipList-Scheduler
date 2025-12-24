#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "graph_sched.h"
#include "common.h"

// Wrappers for standard priority queue
static void prioq_insert_wrapper(void *q, pkey_t key, pval_t value) {
    insert((pq_t *)q, key, value);
}

static pval_t prioq_delete_min_wrapper(void *q) {
    return deletemin((pq_t *)q);
}

// Wrappers for NUMA-sharded priority queue
static void numa_prioq_insert_wrapper(void *q, pkey_t key, pval_t value) {
    numa_priq_insert((numa_prioq_t *)q, key, value);
}

static pval_t numa_prioq_delete_min_wrapper(void *q) {
    return numa_priq_delete_min((numa_prioq_t *)q);
}

// Internal helper for shared graph construction logic
static graph_sched_t *graph_sched_alloc_and_build(int n_tasks, int edges_per_task) {
    graph_sched_t *gs;
    int i, j, child, edge_count;
    unsigned int seed = (unsigned int)time(NULL);

    E_NULL(gs = (graph_sched_t *)malloc(sizeof(graph_sched_t)));
    gs->n_tasks = n_tasks;
    E_NULL(gs->tasks = (graph_task_t *)calloc(n_tasks, sizeof(graph_task_t)));

    for (i = 0; i < n_tasks; i++) {
        gs->tasks[i].id = i;
        gs->tasks[i].priority = (prio_t)i;
        gs->tasks[i].indegree = 0;
        gs->tasks[i].n_deps = 0;
        gs->tasks[i].deps = NULL;
    }

    for (i = 0; i < n_tasks - 1; i++) {
        if (edges_per_task > 0) {
            E_NULL(gs->tasks[i].deps = (int *)malloc(edges_per_task * sizeof(int)));
        }
        edge_count = 0;
        for (j = 0; j < edges_per_task; j++) {
            int range = n_tasks - i - 1;
            if (range <= 0) break;
            child = i + 1 + (rand_r(&seed) % range);
            
            int duplicate = 0;
            for (int k = 0; k < edge_count; k++) {
                if (gs->tasks[i].deps[k] == child) {
                    duplicate = 1;
                    break;
                }
            }
            if (!duplicate) {
                gs->tasks[i].deps[edge_count++] = child;
                gs->tasks[child].indegree++;
            }
        }
        gs->tasks[i].n_deps = edge_count;
    }
    return gs;
}

graph_sched_t *graph_sched_create_random_prioq(int n_tasks, int edges_per_task) {
    graph_sched_t *gs = graph_sched_alloc_and_build(n_tasks, edges_per_task);
    if (!gs) return NULL;

    gs->qiface.q = (void *)pq_init(32);
    gs->qiface.insert = prioq_insert_wrapper;
    gs->qiface.delete_min = prioq_delete_min_wrapper;
    return gs;
}

graph_sched_t *graph_sched_create_random_numa(int n_tasks, int edges_per_task, int num_nodes) {
    graph_sched_t *gs = graph_sched_alloc_and_build(n_tasks, edges_per_task);
    if (!gs) return NULL;

    gs->qiface.q = (void *)numa_priq_init(num_nodes, 32);
    gs->qiface.insert = numa_prioq_insert_wrapper;
    gs->qiface.delete_min = numa_prioq_delete_min_wrapper;
    return gs;
}

void graph_sched_destroy(graph_sched_t *gs) {
    if (gs == NULL) return;
    for (int i = 0; i < gs->n_tasks; i++) {
        if (gs->tasks[i].deps) free(gs->tasks[i].deps);
    }
    free(gs->tasks);
    // Note: We don't have a generic destroy and the prompt said 
    // "do not invent one unless you see it declared somewhere".
    // However, numa_priq_destroy exists. Standard pq has pq_destroy.
    // We should call the appropriate one if we want to be clean, 
    // but the original code had no destroy for standard pq in the prompt's view.
    // Actually, prioq.h HAS pq_destroy(pq_t *pq).
    // Given the qiface, we'd need a destroy pointer too.
    // But I'll stick to the user's specific logic for now.
    // Since I know pq_destroy and numa_priq_destroy exist, I'll use them if I can.
    // For now, I'll just free the ones I know.
    if (gs->qiface.insert == numa_prioq_insert_wrapper) {
        numa_priq_destroy((numa_prioq_t *)gs->qiface.q);
    } else {
        pq_destroy((pq_t *)gs->qiface.q);
    }
    free(gs);
}

void graph_sched_init_ready(graph_sched_t *gs) {
    for (int i = 0; i < gs->n_tasks; i++) {
        if (gs->tasks[i].indegree == 0) {
            gs->qiface.insert(gs->qiface.q, gs->tasks[i].priority, (pval_t)&gs->tasks[i]);
        }
    }
}

int graph_sched_extract_min_topo(graph_sched_t *gs) {
    while (1) {
        pval_t val = gs->qiface.delete_min(gs->qiface.q);
        if (val == NULL) return -1;
        
        graph_task_t *task = (graph_task_t *)val;
        int task_id = task->id;
        
        if (task_id >= 0 && task_id < gs->n_tasks && gs->tasks[task_id].indegree == 0) {
            return task_id;
        }
    }
}

int graph_sched_task_completed(graph_sched_t *gs, int task_id) {
    int enqueued = 0;
    if (task_id < 0 || task_id >= gs->n_tasks) return 0;
    
    graph_task_t *task = &gs->tasks[task_id];
    for (int i = 0; i < task->n_deps; i++) {
        int child_id = task->deps[i];
        gs->tasks[child_id].indegree--;
        if (gs->tasks[child_id].indegree == 0) {
            gs->qiface.insert(gs->qiface.q, gs->tasks[child_id].priority, (pval_t)&gs->tasks[child_id]);
            enqueued++;
        }
    }
    return enqueued;
}
