/**
 * @file department.h
 * @brief Department Module: fixed array of departments.
 */
#ifndef EMS_DEPARTMENT_H
#define EMS_DEPARTMENT_H

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Clears all departments (used at start-up, cleanup and in tests). */
void deptReset(void);
/** createdAt = NULL stamps the current time (files pass the saved time).
 *  Adds a department. Returns EMS_OK, ERR_INVALID_INPUT, ERR_DUPLICATE_ID or ERR_LIMIT_REACHED. */
int departmentAdd(int id, const char *name, const char *createdAt);
/** Returns the array index of the department, or -1. */
int findDept(int id);
/** Returns the array index of the department with this name (any case), or -1. */
int findDeptByName(const char *name);
/** Returns the department name, or "(none)". */
const char *getDeptName(int id);
int getDeptCount(void);
/** Returns the department at index, or NULL. */
const Department *getDeptAt(int index);
/** Prints all departments with their employee counts. Returns the number printed. */
int viewDepartments(void);
/** Interactive: asks for ID and name, adds and saves. Returns a status code. */
int createDepartment(void);
/** Interactive department sub-menu (Admin only, checked by caller). */
void departmentMenu(void);

#ifdef __cplusplus
}
#endif

#endif /* EMS_DEPARTMENT_H */
