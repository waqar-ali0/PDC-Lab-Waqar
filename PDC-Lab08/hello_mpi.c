#include <stdio.h>
#include <string.h>
#include <mpi.h>

int main(int argc, char **argv)
{
    int rank, size, len;
    char name[MPI_MAX_PROCESSOR_NAME];
    char line[MPI_MAX_PROCESSOR_NAME + 64];

    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Get_processor_name(name, &len);

    // unordered: every rank prints whenever it gets here
    printf("Hello from rank %d of %d on %s\n", rank, size, name);
    fflush(stdout);

    MPI_Barrier(MPI_COMM_WORLD);

    // ordered: every rank sends its line to rank 0, rank 0 prints in order
    snprintf(line, sizeof(line), "rank %d of %d on %s", rank, size, name);
    if (rank == 0) {
        printf("--- ordered report from rank 0 ---\n");
        printf("%s\n", line);
        for (int r = 1; r < size; r++) {
            MPI_Recv(line, sizeof(line), MPI_CHAR, r, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            printf("%s\n", line);
        }
    } else {
        MPI_Send(line, strlen(line) + 1, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
    }

    MPI_Finalize();
    return 0;
}
