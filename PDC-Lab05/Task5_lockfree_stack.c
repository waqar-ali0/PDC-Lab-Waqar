/* lockfree_stack.c - Task 5 (optional): lock-free stack using compare-and-swap
   usage: ./lockfree_stack [threads] [nodes_per_thread]     default: 4 50000 */
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sched.h>
#include <stdatomic.h>
#include <time.h>

#define MAX_THREADS 8

typedef struct node {
    int value;
    struct node *next;
} node_t;

_Atomic(node_t *) head = NULL;  /* top of the stack */
node_t *pool;                   /* all nodes, never freed during the test */
int *seen;                      /* how many times each value was popped */
int num_threads = 4;
int nodes_per_thread = 50000;

double now_ms()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
}

void push(node_t *n)
{
    node_t *old = atomic_load(&head);
    do {
        n->next = old;
    } while (!atomic_compare_exchange_weak(&head, &old, n));
}

node_t *pop()
{
    node_t *old = atomic_load(&head);
    while (old != NULL && !atomic_compare_exchange_weak(&head, &old, old->next)) {
        /* CAS failed, old now has the new head, try again */
    }
    return old;
}

void *worker(void *arg)
{
    long id = (long)arg;
    for (int i = 0; i < nodes_per_thread; i++) {
        node_t *n = &pool[id * nodes_per_thread + i];
        n->value = id * nodes_per_thread + i;
        push(n);
    }
    for (int i = 0; i < nodes_per_thread; i++) {
        node_t *n;
        while ((n = pop()) == NULL)
            sched_yield();              /* stack empty for now, wait a bit */
        __atomic_fetch_add(&seen[n->value], 1, __ATOMIC_RELAXED);
    }
    return NULL;
}

int main(int argc, char *argv[])
{
    if (argc > 1) num_threads = atoi(argv[1]);
    if (argc > 2) nodes_per_thread = atoi(argv[2]);
    int total = num_threads * nodes_per_thread;
    pool = calloc(total, sizeof(node_t));
    seen = calloc(total, sizeof(int));

    pthread_t threads[MAX_THREADS];
    double start = now_ms();
    for (long i = 0; i < num_threads; i++)
        pthread_create(&threads[i], NULL, worker, (void *)i);
    for (int i = 0; i < num_threads; i++)
        pthread_join(threads[i], NULL);
    double elapsed = now_ms() - start;

    long lost = 0, duplicated = 0;
    for (int i = 0; i < total; i++) {
        if (seen[i] == 0) lost++;
        else if (seen[i] > 1) duplicated += seen[i] - 1;
    }
    int empty = (atomic_load(&head) == NULL);
    printf("[lock-free stack] threads=%d nodes=%d\n", num_threads, total);
    printf("lost=%ld duplicated=%ld stack_empty=%s correct=%s time=%.2f ms\n",
           lost, duplicated, empty ? "YES" : "NO",
           (lost == 0 && duplicated == 0 && empty) ? "YES" : "NO", elapsed);

    free(pool);
    free(seen);
    return 0;
}
