/* buffer_sem.c - Task 2: bounded buffer, many producers and consumers, semaphores
   usage: ./buffer_sem [producers] [consumers] [buffer_size] [items_per_producer]
   default: 4 4 8 50000 */
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>

#define MAX_BUFFER 64
#define MAX_THREADS 8

int buffer[MAX_BUFFER];
int num_producers = 4, num_consumers = 4;
int buffer_size = 8;
int items_per_producer = 50000;
int total_items;

int in_pos = 0, out_pos = 0;                /* shared state */
sem_t empty_slots;                          /* number of free slots   */
sem_t full_slots;                           /* number of filled slots */
pthread_mutex_t buf_lock = PTHREAD_MUTEX_INITIALIZER;
int *seen;                                  /* how many times each item was consumed */

double now_ms()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
}

void *producer(void *arg)
{
    long id = (long)arg;
    for (int k = 0; k < items_per_producer; k++) {
        int item = id * items_per_producer + k;     /* unique item number */

        sem_wait(&empty_slots);
        pthread_mutex_lock(&buf_lock);
        buffer[in_pos] = item;
        in_pos = (in_pos + 1) % buffer_size;
        pthread_mutex_unlock(&buf_lock);
        sem_post(&full_slots);
    }
    return NULL;
}

void *consumer(void *arg)
{
    for (int k = 0; k < total_items / num_consumers; k++) {
        sem_wait(&full_slots);
        pthread_mutex_lock(&buf_lock);
        int item = buffer[out_pos];
        out_pos = (out_pos + 1) % buffer_size;
        pthread_mutex_unlock(&buf_lock);
        sem_post(&empty_slots);

        __atomic_fetch_add(&seen[item], 1, __ATOMIC_RELAXED);
    }
    return NULL;
}

int main(int argc, char *argv[])
{
    if (argc > 1) num_producers = atoi(argv[1]);
    if (argc > 2) num_consumers = atoi(argv[2]);
    if (argc > 3) buffer_size = atoi(argv[3]);
    if (argc > 4) items_per_producer = atoi(argv[4]);
    total_items = num_producers * items_per_producer;
    seen = calloc(total_items, sizeof(int));

    pthread_t prod[MAX_THREADS], cons[MAX_THREADS];
    sem_init(&empty_slots, 0, buffer_size);
    sem_init(&full_slots, 0, 0);

    double start = now_ms();
    for (long i = 0; i < num_producers; i++)
        pthread_create(&prod[i], NULL, producer, (void *)i);
    for (long i = 0; i < num_consumers; i++)
        pthread_create(&cons[i], NULL, consumer, NULL);
    for (int i = 0; i < num_producers; i++) pthread_join(prod[i], NULL);
    for (int i = 0; i < num_consumers; i++) pthread_join(cons[i], NULL);
    double elapsed = now_ms() - start;

    long lost = 0, duplicated = 0;
    for (int i = 0; i < total_items; i++) {
        if (seen[i] == 0) lost++;
        else if (seen[i] > 1) duplicated += seen[i] - 1;
    }
    printf("[semaphores] producers=%d consumers=%d buffer=%d items=%d\n",
           num_producers, num_consumers, buffer_size, total_items);
    printf("lost=%ld duplicated=%ld correct=%s time=%.2f ms\n",
           lost, duplicated, (lost == 0 && duplicated == 0) ? "YES" : "NO", elapsed);

    sem_destroy(&empty_slots);
    sem_destroy(&full_slots);
    free(seen);
    return 0;
}
