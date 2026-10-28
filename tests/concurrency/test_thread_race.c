/**
 * @file test_thread_race.c
 * @brief Race condition demo for Helgrind.
 *
 * Four threads add to one shared counter, as if four HR operators were
 * recording attendance at the same time.
 *   Built without -DUSE_MUTEX: counter++ is not atomic, so updates are lost
 *   and Helgrind reports "Possible data race".
 *   Built with -DUSE_MUTEX: the mutex makes the update safe, 0 errors.
 */
#include <pthread.h>
#include <stdio.h>

#define N_THREADS  4
#define ITERATIONS 100000

static long counter = 0;

#ifdef USE_MUTEX
static pthread_mutex_t counter_lock = PTHREAD_MUTEX_INITIALIZER;
#endif

static void *increment_worker(void *arg)
{
    long i;

    (void)arg;
    for (i = 0; i < ITERATIONS; i++) {
#ifdef USE_MUTEX
        (void)pthread_mutex_lock(&counter_lock);
        counter++;                          /* protected critical section */
        (void)pthread_mutex_unlock(&counter_lock);
#else
        counter++;                          /* BUG: unsynchronised write */
#endif
    }
    return NULL;
}

int main(void)
{
    pthread_t threads[N_THREADS];
    int i;

    for (i = 0; i < N_THREADS; i++) {
        (void)pthread_create(&threads[i], NULL, increment_worker, NULL);
    }
    for (i = 0; i < N_THREADS; i++) {
        (void)pthread_join(threads[i], NULL);
    }
#ifdef USE_MUTEX
    (void)printf("[FIXED] Final Counter = %ld (Expected: %ld)\n", counter, (long)N_THREADS * ITERATIONS);
    (void)pthread_mutex_destroy(&counter_lock);
#else
    (void)printf("[BUGGY] Final Counter = %ld (Expected: %ld)\n", counter, (long)N_THREADS * ITERATIONS);
#endif
    return 0;
}
