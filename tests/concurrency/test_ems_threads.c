/**
 * @file test_ems_threads.c
 * @brief Multithreading on the real EMS attendance module.
 *
 * Four threads act as four HR operators recording February 2025 attendance
 * at the same time, each for a different employee, into the ONE shared
 * attendance array (attendanceAdd() grows it with realloc()).
 *
 *   Without -DUSE_MUTEX: no synchronisation. Helgrind reports data races on
 *   the array pointer, the count and realloc(); records can be lost or the
 *   program can crash. Run it only under Helgrind (make helgrind-ems-buggy).
 *   With -DUSE_MUTEX: one mutex guards attendanceAdd(), every record is
 *   stored exactly once and Helgrind reports 0 errors.
 */
#include <pthread.h>
#include <stdio.h>
#include <time.h>
#include "../../include/attendance.h"
#include "../../include/department.h"
#include "../../include/employee.h"
#include "../../include/filemanager.h"
#include "../../include/role.h"

#define N_OPERATORS 4
#define DAYS        28           /* February 2025 */

#ifdef USE_MUTEX
static pthread_mutex_t attendance_lock = PTHREAD_MUTEX_INITIALIZER;
#endif

typedef struct {
    int empId;
    int stored;
} OperatorJob;

static void *operator_worker(void *arg)
{
    OperatorJob *job = (OperatorJob *)arg;
    int day;
    int result;
    Date date;

    for (day = 1; day <= DAYS; day++) {
        date.day = day;
        date.month = 2;
        date.year = 2025;
#ifdef USE_MUTEX
        (void)pthread_mutex_lock(&attendance_lock);
        result = attendanceAdd(job->empId, &date, ((day % 7) == 0) ? 'A' : 'P', NULL);
        (void)pthread_mutex_unlock(&attendance_lock);
#else
        result = attendanceAdd(job->empId, &date, ((day % 7) == 0) ? 'A' : 'P', NULL);   /* BUG: shared array */
#endif
        if (result == EMS_OK) {
            job->stored++;
        }
    }
    return NULL;
}

int main(void)
{
    pthread_t threads[N_OPERATORS];
    OperatorJob jobs[N_OPERATORS];
    struct timespec t0;
    struct timespec t1;
    int i;
    int stored = 0;
    double ms;
    Employee e = {0};

    /* in-memory setup: one department, one role, four employees */
    (void)departmentAdd(10, "Finance", NULL);
    (void)roleAdd(20, "Accountant", NULL);
    for (i = 0; i < N_OPERATORS; i++) {
        e.id = 101 + i;
        (void)snprintf(e.name, sizeof(e.name), "Employee %c", (char)('A' + i));
        (void)snprintf(e.phone, sizeof(e.phone), "%s", "9876543210");
        (void)snprintf(e.email, sizeof(e.email), "%s", "emp@corp.com");
        e.deptId = 10;
        e.roleId = 20;
        e.basicSalary = 30000.0;
        e.createdAt[0] = '\0';
        e.updatedAt[0] = '\0';
        (void)employeeInsert(&e);
        jobs[i].empId = 101 + i;
        jobs[i].stored = 0;
    }

    (void)clock_gettime(CLOCK_MONOTONIC, &t0);
    for (i = 0; i < N_OPERATORS; i++) {
        (void)pthread_create(&threads[i], NULL, operator_worker, &jobs[i]);   /* thread creation */
    }
    for (i = 0; i < N_OPERATORS; i++) {
        (void)pthread_join(threads[i], NULL);
        stored += jobs[i].stored;
    }
    (void)clock_gettime(CLOCK_MONOTONIC, &t1);
    ms = ((double)(t1.tv_sec - t0.tv_sec) * 1000.0) + ((double)(t1.tv_nsec - t0.tv_nsec) / 1000000.0);

#ifdef USE_MUTEX
    (void)printf("[FIXED] ");
#else
    (void)printf("[BUGGY] ");
#endif
    (void)printf("%d operators stored %d records, array holds %d (expected %d) in %.3f ms\n", N_OPERATORS,
                 stored, getAttendanceCount(), N_OPERATORS * DAYS, ms);
    freeAllMemory();
#ifdef USE_MUTEX
    (void)pthread_mutex_destroy(&attendance_lock);
#endif
    return 0;
}
