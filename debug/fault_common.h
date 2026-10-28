/**
 * @file fault_common.h
 * @brief Shared setup for the fault-injection programs: one department,
 *        one role and two employees, all in memory.
 */
#ifndef FAULT_COMMON_H
#define FAULT_COMMON_H

#include <stdio.h>
#include <string.h>
#include "../include/common.h"
#include "../include/department.h"
#include "../include/employee.h"
#include "../include/filemanager.h"
#include "../include/payroll.h"
#include "../include/role.h"
#include "../include/validation.h"

/** Unbuffered output so every line is visible even if the program crashes. */
static inline void faultInit(void)
{
    (void)setvbuf(stdout, NULL, _IONBF, 0);
}

static inline void faultSetup(void)
{
    Employee e;

    faultInit();
    (void)memset(&e, 0, sizeof(e));
    (void)departmentAdd(10, "Finance", NULL);
    (void)roleAdd(20, "Accountant", NULL);
    e.id = 101;
    (void)snprintf(e.name, sizeof(e.name), "%s", "Ravi Kumar");
    (void)snprintf(e.phone, sizeof(e.phone), "%s", "9876543210");
    (void)snprintf(e.email, sizeof(e.email), "%s", "ravi@corp.com");
    e.deptId = 10;
    e.roleId = 20;
    e.basicSalary = 30000.0;
    (void)employeeInsert(&e);
    e.id = 102;
    (void)snprintf(e.name, sizeof(e.name), "%s", "Priya Sharma");
    (void)employeeInsert(&e);
}

#endif /* FAULT_COMMON_H */
