/* schedule_bench.c - Task 2 and 4: static, dynamic and guided scheduling
   build: gcc -O2 -fopenmp -std=gnu11 schedule_bench.c -o schedule_bench
          gcc -O2 -fopenmp -std=gnu11 -DUNIFORM schedule_bench.c -o schedule_bench_uniform
   run:   OMP_NUM_THREADS=4 ./schedule_bench
          OMP_NUM_THREADS=4 OMP_SCHEDULE="dynamic,16" ./schedule_bench   (one policy only) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>

#define N         4000      /* loop iterations */
#define COST_STEP 20        /* uneven: iteration i costs (i+1)*COST_STEP steps */
#define RUNS      3         /* each policy is run 3 times, the best run is kept */
#define SERIAL_RUNS 5       /* the single-thread time is noisier on a laptop, so best of 5 */
#define MAX_THREADS 64

double result[N];
double expected[N];
double busy[MAX_THREADS];   /* time each thread spent inside its iterations */

/* how many inner steps iteration i needs */
long cost(int i)
{
#ifdef UNIFORM
    (void)i;                                /* every iteration costs the same */
    return (long)(N + 1) / 2 * COST_STEP;   /* same total work as the uneven case */
#else
    return (long)(i + 1) * COST_STEP;       /* later iterations are much more expensive */
#endif
}

/* the work of one iteration, the result depends only on i.
   noinline so the serial loop and the parallel loop run exactly the same machine code */
__attribute__((noinline)) double work(int i)
{
    long steps = cost(i);
    double x = i * 0.001;
    for (long k = 0; k < steps; k++)
        x = x * 0.999999 + (double)(k & 7);
    return x;
}

typedef struct {
    const char *name;
    omp_sched_t kind;
    int chunk;              /* 0 means the default chunk size */
} policy_t;

/* runs the loop once with the schedule set at run time, returns the wall time in ms */
double run_once(int *nthreads)
{
    double start = omp_get_wtime();
    #pragma omp parallel
    {
        int tid = omp_get_thread_num();
        double t0 = omp_get_wtime();
        #pragma omp for schedule(runtime) nowait
        for (int i = 0; i < N; i++)
            result[i] = work(i);
        busy[tid] = (omp_get_wtime() - t0) * 1000.0;
        #pragma omp single
        *nthreads = omp_get_num_threads();
    }
    return (omp_get_wtime() - start) * 1000.0;
}

int check_result()
{
    for (int i = 0; i < N; i++)
        if (result[i] != expected[i])
            return 0;
    return 1;
}

/* runs one policy RUNS times and prints the best run */
double bench_policy(const char *name, int print_threads)
{
    double best_wall = 1e30, best_busy[MAX_THREADS];
    int nthreads = 1, correct = 1;

    for (int r = 0; r < RUNS; r++) {
        memset(result, 0, sizeof(result));
        double wall = run_once(&nthreads);
        if (!check_result())
            correct = 0;
        if (wall < best_wall) {
            best_wall = wall;
            memcpy(best_busy, busy, sizeof(busy));
        }
    }

    double bmax = best_busy[0], bmin = best_busy[0];
    for (int t = 1; t < nthreads; t++) {
        if (best_busy[t] > bmax) bmax = best_busy[t];
        if (best_busy[t] < bmin) bmin = best_busy[t];
    }
    printf("%-11s wall=%8.2f ms  busy max=%8.2f  min=%8.2f  imbalance=%5.2f  correct=%s\n",
           name, best_wall, bmax, bmin, bmax / bmin, correct ? "YES" : "NO");
    if (print_threads) {
        printf("            busy per thread:");
        for (int t = 0; t < nthreads; t++)
            printf(" T%d=%.1f", t, best_busy[t]);
        printf("\n");
    }
    return best_wall;
}

int main()
{
    policy_t policies[] = {
        {"static",     omp_sched_static,  0},
        {"static,1",   omp_sched_static,  1},
        {"static,64",  omp_sched_static,  64},
        {"dynamic,1",  omp_sched_dynamic, 1},
        {"dynamic,64", omp_sched_dynamic, 64},
        {"guided,1",   omp_sched_guided,  1},
        {"guided,64",  omp_sched_guided,  64},
    };
    int npolicies = sizeof(policies) / sizeof(policies[0]);

    /* warm-up run (not timed): creates the thread team and lets the CPU clock up,
       otherwise the first policy measured looks slower than it really is */
    int warm_threads, env_chunk;
    omp_sched_t env_kind;
    omp_get_schedule(&env_kind, &env_chunk);    /* keep what OMP_SCHEDULE asked for */
    omp_set_schedule(omp_sched_dynamic, 1);
    run_once(&warm_threads);
    omp_set_schedule(env_kind, env_chunk);

    /* serial reference: correct results and the time to beat (best of SERIAL_RUNS) */
    double serial_ms = 1e30;
    for (int r = 0; r < SERIAL_RUNS; r++) {
        double t0 = omp_get_wtime();
        for (int i = 0; i < N; i++)
            expected[i] = work(i);
        double ms = (omp_get_wtime() - t0) * 1000.0;
        if (ms < serial_ms)
            serial_ms = ms;
    }

#ifdef UNIFORM
    const char *workload = "uniform";
#else
    const char *workload = "uneven";
#endif
    printf("workload=%s  N=%d  threads=%d  serial=%.2f ms  (best of %d runs per policy)\n",
           workload, N, omp_get_max_threads(), serial_ms, RUNS);

    /* optional part: if OMP_SCHEDULE is set, only run that policy from the environment */
    char *env = getenv("OMP_SCHEDULE");
    if (env != NULL) {
        char label[64];
        snprintf(label, sizeof(label), "env:%s", env);
        bench_policy(label, 1);
        return 0;
    }

    int best = 0;
    double best_ms = 1e30;
    for (int p = 0; p < npolicies; p++) {
        omp_set_schedule(policies[p].kind, policies[p].chunk);
        double ms = bench_policy(policies[p].name, 1);
        if (ms < best_ms) {
            best_ms = ms;
            best = p;
        }
    }
    printf("fastest policy: %s (%.2f ms, speedup %.2fx over serial)\n",
           policies[best].name, best_ms, serial_ms / best_ms);
    return 0;
}
