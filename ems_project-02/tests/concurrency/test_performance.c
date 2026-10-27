/**
 * @file test_performance.c
 * @brief Multithreading performance: the cost of synchronisation.
 *
 * Adds up 40,000,000 salary units (e.g. a payroll total) four ways and
 * prints the time of each:
 *   1. single thread                    - baseline
 *   2. 4 threads, mutex on every add    - correct but slow (lock contention)
 *   3. 4 threads, local sum + one lock  - correct and fast (lock once per thread)
 *   4. 4 threads, no lock               - fast but WRONG (race condition)
 */
#include <pthread.h>
#include <stdio.h>
#include <time.h>

#define N_THREADS 4
#define TOTAL     40000000L

static volatile long g_sum = 0;
static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;

static double nowMs(void)
{
    struct timespec t;

    (void)clock_gettime(CLOCK_MONOTONIC, &t);
    return ((double)t.tv_sec * 1000.0) + ((double)t.tv_nsec / 1000000.0);
}

static void *lockEveryAdd(void *arg)
{
    long i;

    (void)arg;
    for (i = 0; i < (TOTAL / N_THREADS); i++) {
        (void)pthread_mutex_lock(&g_lock);
        g_sum = g_sum + 1;
        (void)pthread_mutex_unlock(&g_lock);
    }
    return NULL;
}

static void *localThenLockOnce(void *arg)
{
    long i;
    volatile long local = 0;     /* volatile: stops the compiler deleting the loop */

    (void)arg;
    for (i = 0; i < (TOTAL / N_THREADS); i++) {
        local++;
    }
    (void)pthread_mutex_lock(&g_lock);
    g_sum = g_sum + local;
    (void)pthread_mutex_unlock(&g_lock);
    return NULL;
}

static void *noLock(void *arg)
{
    long i;

    (void)arg;
    for (i = 0; i < (TOTAL / N_THREADS); i++) {
        g_sum = g_sum + 1;              /* race: read-modify-write is not atomic */
    }
    return NULL;
}

static void runThreads(const char *label, void *(*worker)(void *))
{
    pthread_t t[N_THREADS];
    double start;
    double ms;
    int i;

    g_sum = 0;
    start = nowMs();
    for (i = 0; i < N_THREADS; i++) {
        (void)pthread_create(&t[i], NULL, worker, NULL);
    }
    for (i = 0; i < N_THREADS; i++) {
        (void)pthread_join(t[i], NULL);
    }
    ms = nowMs() - start;
    (void)printf("%-38s %10.1f ms   result %9ld  %s\n", label, ms, (long)g_sum,
                 (g_sum == TOTAL) ? "correct" : "WRONG (lost updates)");
}

int main(void)
{
    long i;
    double start;
    double ms;

    (void)printf("Adding %ld units with %d threads\n\n", TOTAL, N_THREADS);
    g_sum = 0;
    start = nowMs();
    for (i = 0; i < TOTAL; i++) {
        g_sum = g_sum + 1;
    }
    ms = nowMs() - start;
    (void)printf("%-38s %10.1f ms   result %9ld  correct\n", "1. single thread", ms, (long)g_sum);
    runThreads("2. 4 threads, mutex on every add", lockEveryAdd);
    runThreads("3. 4 threads, local sum + one lock", localThenLockOnce);
    runThreads("4. 4 threads, no lock", noLock);
    (void)pthread_mutex_destroy(&g_lock);
    return 0;
}
