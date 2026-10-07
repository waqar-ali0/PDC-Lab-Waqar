/* phase_barrier.c - Task 4: reusable phase barrier for worker threads
   usage: ./phase_barrier [workers] [rounds] [use_barrier]    default: 4 100000 1
   use_barrier = 0 runs the experiment without the barrier */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <time.h>

#define MAX_WORKERS 8

typedef struct {
    pthread_mutex_t m;
    pthread_cond_t cv;
    int parties;                /* threads that must arrive */
    int waiting;                /* threads that have arrived in this phase */
    unsigned long generation;   /* increases every time the barrier opens */
} barrier_t;

barrier_t bar;
uint64_t cells[MAX_WORKERS];    /* shared array */
int num_workers = 4;
int rounds = 100000;
int use_barrier = 1;

void barrier_init(barrier_t *b, int parties)
{
    pthread_mutex_init(&b->m, NULL);
    pthread_cond_init(&b->cv, NULL);
    b->parties = parties;
    b->waiting = 0;
    b->generation = 0;
}

void barrier_wait(barrier_t *b)
{
    pthread_mutex_lock(&b->m);
    unsigned long my_gen = b->generation;
    b->waiting++;
    if (b->waiting == b->parties) {
        /* last thread: open the barrier */
        b->waiting = 0;
        b->generation++;
        pthread_cond_broadcast(&b->cv);
    } else {
        while (my_gen == b->generation)
            pthread_cond_wait(&b->cv, &b->m);
    }
    pthread_mutex_unlock(&b->m);
}

double now_ms()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
}

void *worker(void *arg)
{
    long id = (long)arg;
    for (int r = 0; r < rounds; r++) {
        uint64_t next = cells[id] + cells[(id + 1) % num_workers];  /* read */
        if (use_barrier) barrier_wait(&bar);                        /* all reads done */
        cells[id] = next;                                           /* write */
        if (use_barrier) barrier_wait(&bar);                        /* all writes done */
    }
    return NULL;
}

int main(int argc, char *argv[])
{
    if (argc > 1) num_workers = atoi(argv[1]);
    if (argc > 2) rounds = atoi(argv[2]);
    if (argc > 3) use_barrier = atoi(argv[3]);

    pthread_t threads[MAX_WORKERS];
    uint64_t expected[MAX_WORKERS], tmp[MAX_WORKERS];

    for (int i = 0; i < num_workers; i++) {
        cells[i] = i + 1;
        expected[i] = i + 1;
    }
    barrier_init(&bar, num_workers);

    double start = now_ms();
    for (long i = 0; i < num_workers; i++)
        pthread_create(&threads[i], NULL, worker, (void *)i);
    for (int i = 0; i < num_workers; i++)
        pthread_join(threads[i], NULL);
    double elapsed = now_ms() - start;

    /* same computation done sequentially, used as reference */
    for (int r = 0; r < rounds; r++) {
        for (int i = 0; i < num_workers; i++)
            tmp[i] = expected[i] + expected[(i + 1) % num_workers];
        for (int i = 0; i < num_workers; i++)
            expected[i] = tmp[i];
    }
    int mismatches = 0;
    for (int i = 0; i < num_workers; i++)
        if (cells[i] != expected[i]) mismatches++;

    printf("[%s] workers=%d rounds=%d matches_reference=%s mismatching_cells=%d time=%.2f ms\n",
           use_barrier ? "barrier" : "no barrier", num_workers, rounds,
           mismatches == 0 ? "YES" : "NO", mismatches, elapsed);
    return 0;
}
