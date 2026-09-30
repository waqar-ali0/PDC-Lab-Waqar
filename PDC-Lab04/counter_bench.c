#include <stdio.h>
#include <pthread.h>
#include <time.h>

#define NUM_THREADS 4
#define INCREMENTS_PER_THREAD 1000000

long counter = 0;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

void *worker_mutex(void *arg)
{
    int i;
    for (i = 0; i < INCREMENTS_PER_THREAD; i++) {
        pthread_mutex_lock(&lock);
        counter++;
        pthread_mutex_unlock(&lock);
    }
    return NULL;
}

void *worker_sync(void *arg)
{
    int i;
    for (i = 0; i < INCREMENTS_PER_THREAD; i++) {
        __sync_fetch_and_add(&counter, 1);
    }
    return NULL;
}

double get_time_ms()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1000000.0;
}

void run_test(const char *name, void *(*func)(void *))
{
    pthread_t threads[NUM_THREADS];
    int i;

    counter = 0;
    double start = get_time_ms();

    for (i = 0; i < NUM_THREADS; i++)
        pthread_create(&threads[i], NULL, func, NULL);

    for (i = 0; i < NUM_THREADS; i++)
        pthread_join(threads[i], NULL);

    double elapsed = get_time_ms() - start;
    long expected = (long)NUM_THREADS * INCREMENTS_PER_THREAD;

    printf("[%s] expected=%ld actual=%ld correct=%s time=%.2f ms\n",
           name, expected, counter,
           (expected == counter) ? "YES" : "NO", elapsed);
}

int main()
{
    run_test("mutex", worker_mutex);
    run_test("__sync_fetch_and_add", worker_sync);
    return 0;
}
