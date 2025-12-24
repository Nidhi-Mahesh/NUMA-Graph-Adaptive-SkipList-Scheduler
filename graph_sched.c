#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "graph_sched.h"
#include "common.h"

graph_sched_t *graph_sched_create_random(int n_tasks, int edges_per_task) {
    graph_sched_t *gs;
    int i, j, child, edge_count;
    
    // Allocate graph scheduler structure
    E_NULL(gs = (graph_sched_t *)malloc(sizeof(graph_sched_t)));
    gs->n_tasks = n_tasks;
    
    // Allocate and zero-initialize task array
    E_NULL(gs->tasks = (graph_task_t *)calloc(n_tasks, sizeof(graph_task_t)));
    
    // Initialize each task
    for (i = 0; i < n_tasks; i++) {
        gs->tasks[i].id = i;
        gs->tasks[i].priority = (prio_t)i;  // Simple monotone priorities
        gs->tasks[i].indegree = 0;
        gs->tasks[i].n_deps = 0;
        gs->tasks[i].deps = NULL;
    }
    
    // Build DAG: only create edges from lower ID to higher ID (prevents cycles)
    // Use a simple random number generator
    unsigned int seed = (unsigned int)time(NULL);
    
    for (i = 0; i < n_tasks - 1; i++) {
        // Pre-allocate space for dependencies
        if (edges_per_task > 0) {
            gs->tasks[i].deps = (int *)malloc(edges_per_task * sizeof(int));
            if (gs->tasks[i].deps == NULL) {
                // Cleanup on failure
                for (j = 0; j < i; j++) {
                    if (gs->tasks[j].deps) free(gs->tasks[j].deps);
                }
                free(gs->tasks);
                free(gs);
                return NULL;
            }
        }
        
        edge_count = 0;
        for (j = 0; j < edges_per_task && edge_count < edges_per_task; j++) {
            // Choose a random child with ID > i
            int range = n_tasks - i - 1;
            if (range <= 0) break;
            
            child = i + 1 + (rand_r(&seed) % range);
            
            // Avoid duplicate edges (simple check)
            int duplicate = 0;
            for (int k = 0; k < edge_count; k++) {
                if (gs->tasks[i].deps[k] == child) {
                    duplicate = 1;
                    break;
                }
            }
            
            if (!duplicate) {
                // Add edge i -> child
                gs->tasks[i].deps[edge_count] = child;
                edge_count++;
                gs->tasks[child].indegree++;
            }
        }
        
        gs->tasks[i].n_deps = edge_count;
    }
    
    // Initialize the ready queue
    gs->ready_q = pq_init(32);  // Use default offset
    
    return gs;
}

void graph_sched_destroy(graph_sched_t *gs) {
    int i;
    
    if (gs == NULL) return;
    
    // Free each task's dependency array
    if (gs->tasks) {
        for (i = 0; i < gs->n_tasks; i++) {
            if (gs->tasks[i].deps) {
                free(gs->tasks[i].deps);
            }
        }
        free(gs->tasks);
    }
    
    // Destroy the priority queue
    if (gs->ready_q) {
        pq_destroy(gs->ready_q);
    }
    
    free(gs);
}

void graph_sched_init_ready(graph_sched_t *gs) {
    int i;
    
    // Insert all tasks with indegree == 0 into the ready queue
    for (i = 0; i < gs->n_tasks; i++) {
        if (gs->tasks[i].indegree == 0) {
            // Store the pointer to the task structure to avoid NULL value for ID 0
            insert(gs->ready_q, gs->tasks[i].priority, (pval_t)&gs->tasks[i]);
        }
    }
}

int graph_sched_extract_min_topo(graph_sched_t *gs) {
    pval_t val;
    graph_task_t *task;
    
    // Keep trying to extract from the queue until we find a runnable task
    while (1) {
        val = deletemin(gs->ready_q);
        
        // If queue is empty, return -1
        if (val == NULL) {
            return -1;
        }
        
        // Extract task from the value
        task = (graph_task_t *)val;
        int task_id = task->id;
        
        // Check if this task is actually runnable (indegree == 0)
        if (task_id >= 0 && task_id < gs->n_tasks && gs->tasks[task_id].indegree == 0) {
            return task_id;
        }
    }
}

void graph_sched_task_completed(graph_sched_t *gs, int task_id) {
    int i, child_id;
    graph_task_t *task;
    
    if (task_id < 0 || task_id >= gs->n_tasks) return;
    
    task = &gs->tasks[task_id];
    
    // Iterate over all children of this task
    for (i = 0; i < task->n_deps; i++) {
        child_id = task->deps[i];
        
        if (child_id < 0 || child_id >= gs->n_tasks) continue;
        
        // Decrement child's indegree
        gs->tasks[child_id].indegree--;
        
        // If child becomes ready, insert it into the queue
        if (gs->tasks[child_id].indegree == 0) {
            insert(gs->ready_q, gs->tasks[child_id].priority, 
                   (pval_t)&gs->tasks[child_id]);
        }
    }
}
