#include <stdio.h>
#include <mpi.h>

#define LAPS 1000

int main(int argc, char **argv)
{
    int rank, size, token;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // every rank only talks to its two neighbours
    int next = (rank + 1) % size;
    int prev = (rank - 1 + size) % size;

    MPI_Barrier(MPI_COMM_WORLD);
    double t0 = MPI_Wtime();

    for (int lap = 0; lap < LAPS; lap++) {
        if (rank == 0) {
            token = 0;
            MPI_Send(&token, 1, MPI_INT, next, 0, MPI_COMM_WORLD);
            MPI_Recv(&token, 1, MPI_INT, prev, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        } else {
            MPI_Recv(&token, 1, MPI_INT, prev, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            token++;   // each rank adds 1 before passing it on
            MPI_Send(&token, 1, MPI_INT, next, 0, MPI_COMM_WORLD);
        }
    }

    double t = (MPI_Wtime() - t0) * 1000;

    if (rank == 0) {
        long messages = (long)LAPS * size;
        printf("ranks=%d laps=%d messages=%ld\n", size, LAPS, messages);
        printf("token after last lap=%d expected=%d\n", token, size - 1);
        printf("time=%.3f ms  per message=%.5f ms  correct=%s\n",
               t, t / messages, token == size - 1 ? "YES" : "NO");
    }

    MPI_Finalize();
    return 0;
}
