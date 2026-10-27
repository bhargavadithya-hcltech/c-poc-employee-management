/**
 * @file test_deadlock.c
 * @brief Deadlock demo for Helgrind.
 *
 * Two threads need the employee lock (A) and the payroll lock (B).
 *   Built without -DFIX_DEADLOCK: thread 2 takes B then A, the opposite
 *   order to thread 1. The threads run one after the other so the demo
 *   does not hang, and Helgrind still reports "lock order violated":
 *   if the two threads ever overlapped, each would wait for the other
 *   forever (deadlock).
 *   Built with -DFIX_DEADLOCK: both threads take A then B, 0 errors.
 */
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

static pthread_mutex_t lock_a = PTHREAD_MUTEX_INITIALIZER;   /* employee data */
static pthread_mutex_t lock_b = PTHREAD_MUTEX_INITIALIZER;   /* payroll data  */

static void *worker_1(void *arg)
{
    (void)arg;
    (void)pthread_mutex_lock(&lock_a);
    (void)printf("Thread 1: Acquired Lock A\n");
    (void)sleep(1);
    (void)pthread_mutex_lock(&lock_b);
    (void)printf("Thread 1: Acquired Lock B\n");
    (void)pthread_mutex_unlock(&lock_b);
    (void)pthread_mutex_unlock(&lock_a);
    return NULL;
}

static void *worker_2(void *arg)
{
    (void)arg;
#ifdef FIX_DEADLOCK
    /* FIXED: same global lock order as thread 1 (A then B) */
    (void)pthread_mutex_lock(&lock_a);
    (void)printf("Thread 2: Acquired Lock A\n");
    (void)sleep(1);
    (void)pthread_mutex_lock(&lock_b);
    (void)printf("Thread 2: Acquired Lock B\n");
#else
    /* BUGGY: reversed order (B then A) */
    (void)pthread_mutex_lock(&lock_b);
    (void)printf("Thread 2: Acquired Lock B\n");
    (void)sleep(1);
    (void)pthread_mutex_lock(&lock_a);
    (void)printf("Thread 2: Acquired Lock A\n");
#endif
    (void)pthread_mutex_unlock(&lock_a);
    (void)pthread_mutex_unlock(&lock_b);
    return NULL;
}

int main(void)
{
    pthread_t t1;
    pthread_t t2;

    (void)pthread_create(&t1, NULL, worker_1, NULL);
#ifndef FIX_DEADLOCK
    (void)pthread_join(t1, NULL);      /* buggy demo: run one after the other */
#endif
    (void)pthread_create(&t2, NULL, worker_2, NULL);
#ifdef FIX_DEADLOCK
    (void)pthread_join(t1, NULL);
#endif
    (void)pthread_join(t2, NULL);
    (void)pthread_mutex_destroy(&lock_a);
    (void)pthread_mutex_destroy(&lock_b);
    (void)printf("Both threads executed and finished successfully.\n");
    return 0;
}
