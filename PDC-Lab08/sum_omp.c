#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

#define N 10000000
#define REPS 3

int main(void)
{
    double *data = malloc(N * sizeof(double));
    for (int i = 0; i < N; i++) data[i] = 1.0;

    double sum = 0, t = 0;

    for (int rep = 0; rep < REPS; rep++) {
        double t0 = omp_get_wtime();
        sum = 0;

        // all threads read the same array, no copying needed
        #pragma omp parallel for reduction(+:sum)
        for (int i = 0; i < N; i++) sum += data[i];

        t += omp_get_wtime() - t0;
    }

    printf("threads=%d\n", omp_get_max_threads());
    printf("reduction sum=%.0f  avg=%.2f ms  correct=%s\n",
           sum, t / REPS * 1000, sum == N ? "YES" : "NO");

    free(data);
    return 0;
}
