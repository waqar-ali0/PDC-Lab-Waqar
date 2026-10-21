/* false_sharing.c - Task 3: false sharing with adjacent counters, fixed with padding
   build: gcc -O2 -fopenmp -std=gnu11 false_sharing.c -o false_sharing
   run:   OMP_NUM_THREADS=4 ./false_sharing */
#include <stdio.h>
#include <omp.h>

#define ITERS       50000000L   /* loop iterations per thread */
#define REPS        3           /* timed repetitions per version, average is printed */
#define MAX_THREADS 64
#define CACHE_LINE  64          /* from: getconf LEVEL1_DCACHE_LINESIZE */

/* volatile so that -O2 really writes the counter to memory in every iteration
   (otherwise the compiler keeps it in a register and the effect disappears) */
volatile long adjacent[MAX_THREADS];               /* 8 counters share one cache line */

typedef struct {
    volatile long value;
    char pad[CACHE_LINE - sizeof(long)];            /* push the next counter to a new line */
} padded_counter;

padded_counter padded[MAX_THREADS] __attribute__((aligned(CACHE_LINE)));

long results[MAX_THREADS];      /* local-accumulate version writes its result here once */
long expected[MAX_THREADS];

/* every thread walks its own random sequence and counts how often bit 16 is set,
   so the three versions do exactly the same amount of work */
static inline unsigned int next_rand(unsigned int x)
{
    return x * 1103515245u + 12345u;
}

void adjacent_version()
{
    #pragma omp parallel
    {
        int tid = omp_get_thread_num();
        unsigned int x = tid + 1;
        adjacent[tid] = 0;
        for (long i = 0; i < ITERS; i++) {
            x = next_rand(x);
            adjacent[tid] += (x >> 16) & 1;
        }
    }
}

void padded_version()
{
    #pragma omp parallel
    {
        int tid = omp_get_thread_num();
        unsigned int x = tid + 1;
        padded[tid].value = 0;
        for (long i = 0; i < ITERS; i++) {
            x = next_rand(x);
            padded[tid].value += (x >> 16) & 1;
        }
    }
}

void local_version()
{
    #pragma omp parallel
    {
        int tid = omp_get_thread_num();
        unsigned int x = tid + 1;
        long count = 0;                 /* private, lives in a register */
        for (long i = 0; i < ITERS; i++) {
            x = next_rand(x);
            count += (x >> 16) & 1;
        }
        results[tid] = count;           /* one shared write per thread */
    }
}

/* runs a version REPS times, returns the average time in ms */
double time_version(void (*fn)())
{
    double total = 0.0;
    for (int r = 0; r < REPS; r++) {
        double t0 = omp_get_wtime();
        fn();
        total += omp_get_wtime() - t0;
    }
    return total / REPS * 1000.0;
}

int main()
{
    int nthreads = omp_get_max_threads();

    /* serial reference value for every thread's counter */
    for (int t = 0; t < nthreads; t++) {
        unsigned int x = t + 1;
        long count = 0;
        for (long i = 0; i < ITERS; i++) {
            x = next_rand(x);
            count += (x >> 16) & 1;
        }
        expected[t] = count;
    }

    double t_adj = time_version(adjacent_version);
    double t_pad = time_version(padded_version);
    double t_loc = time_version(local_version);

    int ok = 1;
    for (int t = 0; t < nthreads; t++)
        if (adjacent[t] != expected[t] || padded[t].value != expected[t] || results[t] != expected[t])
            ok = 0;

    printf("threads=%d  iters/thread=%ld  sizeof(padded_counter)=%zu bytes\n",
           nthreads, ITERS, sizeof(padded_counter));
    printf("adjacent         %9.2f ms\n", t_adj);
    printf("padded           %9.2f ms\n", t_pad);
    printf("local-accumulate %9.2f ms\n", t_loc);
    printf("slowdown adjacent/padded = %.2fx   counters correct=%s\n",
           t_adj / t_pad, ok ? "YES" : "NO");
    return 0;
}
