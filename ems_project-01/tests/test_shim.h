/**
 * @file test_shim.h
 * @brief C/C++ bridge between the GoogleTest suite (test_ems.cpp) and the
 *        C modules. extern "C" stops C++ name mangling so GTest can link
 *        to the C functions.
 */
#ifndef TEST_SHIM_H
#define TEST_SHIM_H

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/* All module headers */
#include "../include/common.h"
#include "../include/logger.h"
#include "../include/io.h"
#include "../include/validation.h"
#include "../include/auth.h"
#include "../include/department.h"
#include "../include/role.h"
#include "../include/employee.h"
#include "../include/searching.h"
#include "../include/attendance.h"
#include "../include/payroll.h"
#include "../include/report.h"
#include "../include/filemanager.h"
#include "../include/app.h"

/* ---- lifecycle: called from the fixture's SetUp() / TearDown() ---- */
/** Clean state for every test: empty tables, fresh temporary data folder,
 *  fresh log file, captured output, Administrator session. */
void test_reset_system(void);
/** Frees all memory, closes streams and the log, deletes the temp folder. */
void test_cleanup_system(void);
/** Departments 10 Finance, 11 HR; roles 20 Accountant, 21 Executive;
 *  employees 101 Ravi Kumar (10/20, 30000) and 102 Priya Sharma (11/21, 24000).
 *  Returns EMS_OK when everything was added. */
int test_seed_data(void);

/* ---- helpers used by the tests ---- */
const char *testDir(void);
const char *testPath(const char *name);
FILE *testInput(void);
FILE *testOutput(void);
/** Scripted keyboard input; every line ends with '\n'. */
void feed(const char *text);
/** Everything the program printed since the test started. */
const char *getOutput(void);
const char *readFile(const char *name);
const char *logText(void);
void writeFile(const char *name, const char *text);
int fileExists(const char *name);
/** A string of n copies of c (n < 300). */
const char *repeatChar(int n, char c);
/** How many times part appears in text. */
int countOccurrences(const char *text, const char *part);
Employee makeEmployee(int id);
Employee makeEmployeeFull(int id, const char *name, int dept, int role, double salary);
Date makeDate(int day, int month, int year);

#ifdef __cplusplus
}
#endif

#endif /* TEST_SHIM_H */
