/* reduction.c - Task 1: parallel sum and max, reduction clause vs critical
   build: gcc -O2 -fopenmp -std=gnu11 reduction.c -o reduction
   run:   OMP_NUM_THREADS=4 ./reduction */
#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

#define N       5000000     /* number of array elements */
#define REPS    5           /* timed repetitions per version, average is printed */
#define MAX_VAL 100000      /* values are in 0 .. MAX_VAL-1 */

int *data;
long long ref_sum;          /* filled by the serial version */
int ref_max;

/* fill the array with pseudo random values (same values on every run) */
void init_data()
{
    unsigned int x = 12345;
    for (int i = 0; i < N; i++) {
        x = x * 1103515245u + 12345u;
        data[i] = (x >> 8) % MAX_VAL;
    }
}

void serial_version(long long *sum_out, int *max_out)
{
    long long sum = 0;
    int max_val = 0;
    for (int i = 0; i < N; i++) {
        sum += data[i];
        if (data[i] > max_val)
            max_val = data[i];
    }
    *sum_out = sum;
    *max_out = max_val;
}

/* wrong on purpose: every thread updates the shared sum and max without protection */
void racy_version(long long *sum_out, int *max_out)
{
    long long sum = 0;
    int max_val = 0;
    #pragma omp parallel for
    for (int i = 0; i < N; i++) {
        sum += data[i];
        if (data[i] > max_val)
            max_val = data[i];
    }
    *sum_out = sum;
    *max_out = max_val;
}

/* correct but slow: one critical section for every single element */
void critical_each_version(long long *sum_out, int *max_out)
{
    long long sum = 0;
    int max_val = 0;
    #pragma omp parallel for
    for (int i = 0; i < N; i++) {
        #pragma omp critical
        {
            sum += data[i];
            if (data[i] > max_val)
                max_val = data[i];
        }
    }
    *sum_out = sum;
    *max_out = max_val;
}

/* each thread works on private partial results, one critical section per thread at the end */
void critical_partial_version(long long *sum_out, int *max_out)
{
    long long sum = 0;
    int max_val = 0;
    #pragma omp parallel
    {
        long long my_sum = 0;
        int my_max = 0;
        #pragma omp for
        for (int i = 0; i < N; i++) {
            my_sum += data[i];
            if (data[i] > my_max)
                my_max = data[i];
        }
        #pragma omp critical
        {
            sum += my_sum;
            if (my_max > max_val)
                max_val = my_max;
        }
    }
    *sum_out = sum;
    *max_out = max_val;
}

/* same idea as critical-partial, but OpenMP creates and combines the private copies */
void reduction_version(long long *sum_out, int *max_out)
{
    long long sum = 0;
    int max_val = 0;
    #pragma omp parallel for reduction(+:sum) reduction(max:max_val)
    for (int i = 0; i < N; i++) {
        sum += data[i];
        if (data[i] > max_val)
            max_val = data[i];
    }
    *sum_out = sum;
    *max_out = max_val;
}

typedef void (*version_fn)(long long *, int *);

void run_version(const char *name, version_fn fn)
{
    long long sum = 0;
    int max_val = 0;
    double total = 0.0;
    int sum_ok = 1, max_ok = 1;

    for (int r = 0; r < REPS; r++) {
        double t0 = omp_get_wtime();
        fn(&sum, &max_val);
        total += omp_get_wtime() - t0;
        if (sum != ref_sum) sum_ok = 0;     /* must be right in every repetition */
        if (max_val != ref_max) max_ok = 0;
    }
    printf("%-17s sum=%-12lld max=%-6d sum_ok=%-3s max_ok=%-3s avg=%9.3f ms\n",
           name, sum, max_val, sum_ok ? "YES" : "NO", max_ok ? "YES" : "NO",
           total / REPS * 1000.0);
}

int main()
{
    data = malloc(N * sizeof(int));
    if (data == NULL) {
        printf("malloc failed\n");
        return 1;
    }
    init_data();
    serial_version(&ref_sum, &ref_max);     /* reference values */

    /* warm-up (not timed): creates the thread team and lets the CPU clock up,
       otherwise the first version measured (serial) looks slower than it is */
    long long tmp_sum;
    int tmp_max;
    for (int r = 0; r < 30; r++) {
        serial_version(&tmp_sum, &tmp_max);
        reduction_version(&tmp_sum, &tmp_max);
    }

    printf("threads=%d  N=%d  reps=%d  expected sum=%lld max=%d\n",
           omp_get_max_threads(), N, REPS, ref_sum, ref_max);
    run_version("serial", serial_version);
    run_version("racy", racy_version);
    run_version("critical-each", critical_each_version);
    run_version("critical-partial", critical_partial_version);
    run_version("reduction", reduction_version);

    free(data);
    return 0;
}
