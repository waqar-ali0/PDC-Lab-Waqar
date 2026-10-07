/* cond_signal.c - Task 3: condition-variable signaling of a "data ready" flag
   usage: ./cond_signal [scenario] [delay_ms]       default: A 1000
   A = 1 worker waits, B = signal first, C = 3 workers, D = spin (polling) */
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

#define MAX_WORKERS 3

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t data_cv = PTHREAD_COND_INITIALIZER;
int data_ready = 0;         /* the flag, protected by lock */
int shared_data = 0;
int wait_calls[MAX_WORKERS];
int received[MAX_WORKERS];

int num_workers = 1;
int use_spin = 0;

double now_ms()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
}

double cpu_ms()
{
    struct timespec ts;
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
}

void sleep_ms(int ms)
{
    struct timespec ts = { ms / 1000, (ms % 1000) * 1000000L };
    nanosleep(&ts, NULL);
}

void *worker(void *arg)
{
    long id = (long)arg;

    if (use_spin) {
        /* polling: keep checking the flag, burns CPU */
        while (1) {
            pthread_mutex_lock(&lock);
            int ready = data_ready;
            pthread_mutex_unlock(&lock);
            if (ready) break;
        }
        pthread_mutex_lock(&lock);
    } else {
        pthread_mutex_lock(&lock);
        while (!data_ready) {           /* check again after every wakeup */
            wait_calls[id]++;
            pthread_cond_wait(&data_cv, &lock);
        }
    }
    received[id] = shared_data;
    pthread_mutex_unlock(&lock);
    return NULL;
}

void publish()
{
    pthread_mutex_lock(&lock);
    shared_data = 42;
    data_ready = 1;
    if (num_workers > 1)
        pthread_cond_broadcast(&data_cv);   /* wake all workers */
    else
        pthread_cond_signal(&data_cv);      /* wake one worker */
    pthread_mutex_unlock(&lock);
}

int main(int argc, char *argv[])
{
    char scenario = 'A';
    int delay = 1000;
    if (argc > 1) scenario = argv[1][0];
    if (argc > 2) delay = atoi(argv[2]);
    if (scenario == 'C') num_workers = 3;
    if (scenario == 'D') use_spin = 1;

    pthread_t threads[MAX_WORKERS];
    double wall0 = now_ms(), cpu0 = cpu_ms();

    if (scenario == 'B')
        publish();                          /* publish before any worker waits */

    for (long i = 0; i < num_workers; i++)
        pthread_create(&threads[i], NULL, worker, (void *)i);

    if (scenario != 'B') {
        sleep_ms(delay);                    /* workers are waiting now */
        publish();
    }

    for (int i = 0; i < num_workers; i++)
        pthread_join(threads[i], NULL);

    double wall = now_ms() - wall0, cpu = cpu_ms() - cpu0;
    for (int i = 0; i < num_workers; i++)
        printf("[worker %d] blocked in wait %d time(s), received %d\n",
               i, wait_calls[i], received[i]);
    printf("scenario=%c workers=%d delay=%d ms wall=%.2f ms cpu=%.2f ms\n",
           scenario, num_workers, delay, wall, cpu);
    return 0;
}
