/**
 * @file searching.h
 * @brief Searching Module: find employees by ID, name, department or role.
 */
#ifndef EMS_SEARCHING_H
#define EMS_SEARCHING_H

#include <stdio.h>
#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

/** 1 when the employee matches the search (type = SEARCH_BY_*). */
int employeeMatches(const Employee *emp, int type, int id, const char *key);
/** Prints all matches to out and returns how many matched.
 *  An ID search uses the sorted list and stops early. */
int employeeSearch(int type, int id, const char *key, FILE *out);
/** Interactive: asks for the search type and value, prints the matches. */
int searchEmployee(void);

#ifdef __cplusplus
}
#endif

#endif /* EMS_SEARCHING_H */
