#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

#define N 10000000
#define REPS 3

int main(int argc, char **argv)
{
    int rank, size;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (N % size != 0) {
        if (rank == 0) printf("number of ranks must divide %d\n", N);
        MPI_Finalize();
        return 1;
    }

    int chunk = N / size;
    double *data = NULL;
    double *part = malloc(chunk * sizeof(double));

    // only rank 0 has the full array
    if (rank == 0) {
        data = malloc(N * sizeof(double));
        for (int i = 0; i < N; i++) data[i] = 1.0;
    }

    double expected = N;
    double t_coll = 0, t_p2p = 0;
    double total_coll = 0, total_p2p = 0;

    for (int rep = 0; rep < REPS; rep++) {
        // collective variant: Scatter + Reduce
        MPI_Barrier(MPI_COMM_WORLD);
        double t0 = MPI_Wtime();

        MPI_Scatter(data, chunk, MPI_DOUBLE, part, chunk, MPI_DOUBLE, 0, MPI_COMM_WORLD);
        double local = 0;
        for (int i = 0; i < chunk; i++) local += part[i];
        MPI_Reduce(&local, &total_coll, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

        t_coll += MPI_Wtime() - t0;

        // point-to-point variant: Send / Recv by hand
        MPI_Barrier(MPI_COMM_WORLD);
        t0 = MPI_Wtime();

        if (rank == 0) {
            for (int r = 1; r < size; r++)
                MPI_Send(data + r * chunk, chunk, MPI_DOUBLE, r, 0, MPI_COMM_WORLD);
            local = 0;
            for (int i = 0; i < chunk; i++) local += data[i];
            total_p2p = local;
            for (int r = 1; r < size; r++) {
                double other;
                MPI_Recv(&other, 1, MPI_DOUBLE, r, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                total_p2p += other;
            }
        } else {
            MPI_Recv(part, chunk, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            local = 0;
            for (int i = 0; i < chunk; i++) local += part[i];
            MPI_Send(&local, 1, MPI_DOUBLE, 0, 1, MPI_COMM_WORLD);
        }

        t_p2p += MPI_Wtime() - t0;
    }

    if (rank == 0) {
        printf("ranks=%d\n", size);
        printf("collective     sum=%.0f  avg=%.2f ms  correct=%s\n",
               total_coll, t_coll / REPS * 1000, total_coll == expected ? "YES" : "NO");
        printf("point-to-point sum=%.0f  avg=%.2f ms  correct=%s\n",
               total_p2p, t_p2p / REPS * 1000, total_p2p == expected ? "YES" : "NO");
        free(data);
    }

    free(part);
    MPI_Finalize();
    return 0;
}
