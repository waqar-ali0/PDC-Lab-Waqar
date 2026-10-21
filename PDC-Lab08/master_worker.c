#include <stdio.h>
#include <mpi.h>

#define TASKS 40
#define TAG_WORK 1
#define TAG_STOP 2

// the "work": square of the task number
int do_task(int t)
{
    return t * t;
}

int main(int argc, char **argv)
{
    int rank, size;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (rank == 0) {
        // master = server, hands out tasks one by one
        int served[64] = {0};
        long total = 0;
        int next = 0, active = 0, result;
        MPI_Status st;

        // give every worker its first task
        for (int w = 1; w < size; w++) {
            if (next < TASKS) {
                MPI_Send(&next, 1, MPI_INT, w, TAG_WORK, MPI_COMM_WORLD);
                next++;
                active++;
            } else {
                MPI_Send(&next, 1, MPI_INT, w, TAG_STOP, MPI_COMM_WORLD);
            }
        }

        // whoever finishes gets the next task
        while (active > 0) {
            MPI_Recv(&result, 1, MPI_INT, MPI_ANY_SOURCE, TAG_WORK, MPI_COMM_WORLD, &st);
            int w = st.MPI_SOURCE;
            total += result;
            served[w]++;
            active--;

            if (next < TASKS) {
                MPI_Send(&next, 1, MPI_INT, w, TAG_WORK, MPI_COMM_WORLD);
                next++;
                active++;
            } else {
                MPI_Send(&next, 1, MPI_INT, w, TAG_STOP, MPI_COMM_WORLD);
            }
        }

        long expected = 0;
        for (int t = 0; t < TASKS; t++) expected += (long)t * t;

        printf("ranks=%d workers=%d tasks=%d\n", size, size - 1, TASKS);
        for (int w = 1; w < size; w++)
            printf("worker %d served %d tasks\n", w, served[w]);
        printf("total=%ld expected=%ld correct=%s\n", total, expected,
               total == expected ? "YES" : "NO");
    } else {
        // worker = client, asks for work until told to stop
        int task;
        MPI_Status st;
        while (1) {
            MPI_Recv(&task, 1, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &st);
            if (st.MPI_TAG == TAG_STOP) break;
            int result = do_task(task);
            MPI_Send(&result, 1, MPI_INT, 0, TAG_WORK, MPI_COMM_WORLD);
        }
    }

    MPI_Finalize();
    return 0;
}
