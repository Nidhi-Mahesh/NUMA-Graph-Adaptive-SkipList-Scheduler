/**
 * Graph scheduling benchmark.
 * Tests the graph_sched layer built on top of the lock-free priority queue.
 *
 * Usage: ./graph_perf_meas <n_tasks> <edges_per_task>
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "gc/gc.h"
#include "common.h"
#include "graph_sched.h"

static void
usage(FILE *out, const char *argv0)
{
    fprintf(out, "Usage: %s <n_tasks> <edges_per_task>\n", argv0);
    fprintf(out, "\n");
    fprintf(out, "  n_tasks         Number of tasks in the DAG\n");
    fprintf(out, "  edges_per_task  Average number of outgoing edges per task\n");
}

int
main(int argc, char **argv)
{
    int n_tasks, edges_per_task;
    graph_sched_t *gs;
    struct timespec start, end, elapsed;
    int executed = 0;
    int task_id;
    double dt;
    
    // Parse command-line arguments
    if (argc != 3) {
        usage(stderr, argv[0]);
        exit(EXIT_FAILURE);
    }
    
    n_tasks = atoi(argv[1]);
    edges_per_task = atoi(argv[2]);
    
    if (n_tasks <= 0 || edges_per_task < 0) {
        fprintf(stderr, "Error: Invalid arguments\n");
        usage(stderr, argv[0]);
        exit(EXIT_FAILURE);
    }
    
    // Initialize garbage collection subsystem
    _init_gc_subsystem();
    
    // Create the task graph
    gs = graph_sched_create_random(n_tasks, edges_per_task);
    if (gs == NULL) {
        fprintf(stderr, "Error: Failed to create graph scheduler\n");
        exit(EXIT_FAILURE);
    }
    
    // Initialize the ready queue with all indegree-0 tasks
    graph_sched_init_ready(gs);
    
    // Start timing
    gettime(&start);
    
    // Main execution loop: extract and execute tasks in topological order
    while (1) {
        task_id = graph_sched_extract_min_topo(gs);
        
        if (task_id < 0) {
            // No more runnable tasks
            break;
        }
        
        // Simulate task execution (minimal work)
        // In a real scheduler, this would be actual computation
        executed++;
        
        // Mark task as completed and update dependencies
        graph_sched_task_completed(gs, task_id);
    }
    
    // End timing
    gettime(&end);
    
    // Calculate elapsed time
    elapsed = timediff(start, end);
    dt = elapsed.tv_sec + (double)elapsed.tv_nsec / 1000000000.0;
    
    // Print statistics
    printf("Tasks:      %d\n", n_tasks);
    printf("Edges/task: %d\n", edges_per_task);
    printf("Executed:   %d\n", executed);
    printf("Total time: %.6f s\n", dt);
    
    // Verify correctness
    if (executed != n_tasks) {
        fprintf(stderr, "Warning: Executed %d tasks, expected %d\n", 
                executed, n_tasks);
    }
    
    // Cleanup
    graph_sched_destroy(gs);
    _destroy_gc_subsystem();
    
    return 0;
}
