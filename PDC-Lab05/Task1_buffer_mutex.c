/* buffer_mutex.c - Task 1: bounded buffer, one producer, one consumer, mutex only
   usage: ./buffer_mutex [buffer_size] [items]      default: 8 200000 */
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sched.h>
#include <time.h>

#define MAX_BUFFER 64

int buffer[MAX_BUFFER];
int buffer_size = 8;
int items = 200000;

int in_pos = 0, out_pos = 0, count = 0;     /* shared state */
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

long producer_retries = 0;
long consumer_retries = 0;
long consumed_sum = 0;
long order_errors = 0;

double now_ms()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
}

void *producer(void *arg)
{
    for (int i = 1; i <= items; i++) {
        while (1) {
            pthread_mutex_lock(&lock);
            if (count < buffer_size) {
                buffer[in_pos] = i;
                in_pos = (in_pos + 1) % buffer_size;
                count++;
                pthread_mutex_unlock(&lock);
                break;
            }
            pthread_mutex_unlock(&lock);    /* buffer full, try again */
            producer_retries++;
            sched_yield();
        }
    }
    return NULL;
}

void *consumer(void *arg)
{
    for (int i = 1; i <= items; i++) {
        int item = 0;
        while (1) {
            pthread_mutex_lock(&lock);
            if (count > 0) {
                item = buffer[out_pos];
                out_pos = (out_pos + 1) % buffer_size;
                count--;
                pthread_mutex_unlock(&lock);
                break;
            }
            pthread_mutex_unlock(&lock);    /* buffer empty, try again */
            consumer_retries++;
            sched_yield();
        }
        if (item != i) order_errors++;      /* items must come as 1, 2, 3, ... */
        consumed_sum += item;
    }
    return NULL;
}

int main(int argc, char *argv[])
{
    if (argc > 1) buffer_size = atoi(argv[1]);
    if (argc > 2) items = atoi(argv[2]);

    pthread_t prod, cons;
    double start = now_ms();
    pthread_create(&prod, NULL, producer, NULL);
    pthread_create(&cons, NULL, consumer, NULL);
    pthread_join(prod, NULL);
    pthread_join(cons, NULL);
    double elapsed = now_ms() - start;

    long expected_sum = (long)items * (items + 1) / 2;
    printf("[mutex + polling] items=%d buffer=%d\n", items, buffer_size);
    printf("expected_sum=%ld actual_sum=%ld checksum_ok=%s order_errors=%ld\n",
           expected_sum, consumed_sum,
           expected_sum == consumed_sum ? "YES" : "NO", order_errors);
    printf("producer_retries=%ld consumer_retries=%ld time=%.2f ms\n",
           producer_retries, consumer_retries, elapsed);
    return 0;
}
