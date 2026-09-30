#include <stdio.h>
#include <pthread.h>

#define NUM_THREADS 4
#define INCREMENTS_PER_THREAD 1000000

long counter = 0;

void *worker(void *arg)
{
    int i;
    for (i = 0; i < INCREMENTS_PER_THREAD; i++)
        counter++;
    return NULL;
}

int main()
{
    pthread_t threads[NUM_THREADS];
    int i;

    for (i = 0; i < NUM_THREADS; i++)
        pthread_create(&threads[i], NULL, worker, NULL);

    for (i = 0; i < NUM_THREADS; i++)
        pthread_join(threads[i], NULL);

    long expected = (long)NUM_THREADS * INCREMENTS_PER_THREAD;
    printf("Expected: %ld\n", expected);
    printf("Actual:   %ld\n", counter);
    printf("Lost:     %ld\n", expected - counter);

    return 0;
}
