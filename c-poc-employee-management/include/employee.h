/**
 * @file employee.h
 * @brief Employee Module: singly linked list of employees sorted by ID.
 */
#ifndef EMS_EMPLOYEE_H
#define EMS_EMPLOYEE_H

#include <stdio.h>
#include "common.h"

/** Frees every node and empties the list. */
void employeeReset(void);
/** Checks every field of an employee and that its department/role exist.
 *  Returns EMS_OK, ERR_INVALID_INPUT or ERR_DEPT_ROLE_NOT_FOUND. */
int validateEmployeeData(const Employee *data);
/** Copies data into a new node and inserts it in ID order.
 *  An empty createdAt is stamped with the current time; updatedAt defaults to createdAt.
 *  Returns EMS_OK, ERR_INVALID_INPUT, ERR_DUPLICATE_ID, ERR_DEPT_ROLE_NOT_FOUND or ERR_MEMORY. */
int employeeInsert(const Employee *data);
/** Finds an employee. Stops early because the list is sorted. NULL if missing. */
Employee *findEmployee(int id);
/** Replaces the editable fields (name, phone, email, salary, department, role). */
int employeeUpdate(int id, const Employee *data);
/** Unlinks and frees an employee and removes their attendance and payroll. */
int employeeRemove(int id);
int employeeSetDepartment(int id, int deptId);
int employeeSetRole(int id, int roleId);
/** 1 when the employee matches the search (type = SEARCH_BY_*). */
int employeeMatches(const Employee *emp, int type, int id, const char *key);
/** Prints all matches to out and returns how many matched. */
int employeeSearch(int type, int id, const char *key, FILE *out);
int countEmployeesInDept(int deptId);
int countEmployeesInRole(int roleId);
const Employee *getEmployeeHead(void);
int getEmployeeCount(void);
void printEmployeeHeader(FILE *out);
void printEmployee(FILE *out, const Employee *emp);
/** Interactive pickers: list the choices and loop until an existing ID is chosen (E006 otherwise). */
int selectDepartment(int *deptId);
int selectRole(int *roleId);

/** Writes "<action> employee <id> by <user>" to the log. */
void logEmployeeEvent(const char *action, int id);

/* ---- interactive functions (menu actions) ---- */
int addEmployee(void);
int viewEmployee(void);
int searchEmployee(void);
int updateEmployee(void);
int deleteEmployee(void);
int assignDepartment(void);
int assignRole(void);
void employeeMenu(void);

#endif /* EMS_EMPLOYEE_H */
