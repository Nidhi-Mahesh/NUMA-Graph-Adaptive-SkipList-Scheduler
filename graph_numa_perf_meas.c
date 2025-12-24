/**
 * Combined Graph + NUMA benchmark.
 * Uses a NUMA-sharded priority queue for task graph scheduling.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "gc/gc.h"
#include "common.h"
#include "graph_sched.h"
#include "numa_prioq.h"

int main(int argc, char **argv) {
    int n_tasks, edges_per_task, nthreads, num_nodes;
    graph_sched_t *gs;
    struct timespec start, end, elapsed;
    long tasks_executed = 0;
    long pq_ops = 0;
    double dt;

    if (argc != 5) {
        fprintf(stderr, "Usage: %s <threads> <num_nodes> <n_tasks> <edges_per_task>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    nthreads = atoi(argv[1]);
    num_nodes = atoi(argv[2]);
    n_tasks = atoi(argv[3]);
    edges_per_task = atoi(argv[4]);

    _init_gc_subsystem();

    // Create graph with NUMA-sharded queue
    gs = graph_sched_create_random_numa(n_tasks, edges_per_task, num_nodes);
    if (!gs) {
        fprintf(stderr, "Failed to create graph scheduler\n");
        exit(EXIT_FAILURE);
    }

    // Measure initial enqueues
    graph_sched_init_ready(gs);
    for (int i = 0; i < gs->n_tasks; i++) {
        if (gs->tasks[i].indegree == 0) pq_ops++;
    }

    gettime(&start);

    while (1) {
        int id = graph_sched_extract_min_topo(gs);
        if (id < 0) break;

        pq_ops++; // For the delete_min itself
        
        int enqueued = graph_sched_task_completed(gs, id);
        pq_ops += enqueued;

        tasks_executed++;
    }

    gettime(&end);

    elapsed = timediff(start, end);
    dt = elapsed.tv_sec + (double)elapsed.tv_nsec / 1000000000.0;

    printf("Threads:        %d\n", nthreads);
    printf("NUMA nodes:     %d\n", num_nodes);
    printf("Tasks:          %d\n", n_tasks);
    printf("Edges/task:     %d\n", edges_per_task);
    printf("Tasks executed: %ld\n", tasks_executed);
    printf("Total time:     %.6f s\n", dt);
    printf("Tasks/s:        %.0f\n", tasks_executed / dt);
    printf("PQ ops:         %ld\n", pq_ops);
    printf("PQ ops/s:       %.0f\n", pq_ops / dt);

    if (tasks_executed != n_tasks) {
        fprintf(stderr, "Warning: Executed %ld tasks, expected %d\n", tasks_executed, n_tasks);
    }

    graph_sched_destroy(gs);
    _destroy_gc_subsystem();

    return 0;
}
