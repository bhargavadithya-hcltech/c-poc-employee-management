/**
 * @file test_framework.h
 * @brief Small unit test framework in plain C (gtest-style output).
 *
 * Every test is a "static void name(void)" function named
 * functionUnderTest_Scenario. RUN_TEST() gives each test a fresh
 * temporary data folder, an empty log, scripted keyboard input (feed())
 * and captured output (getOutput()).
 */
#ifndef EMS_TEST_FRAMEWORK_H
#define EMS_TEST_FRAMEWORK_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "app.h"
#include "attendance.h"
#include "auth.h"
#include "department.h"
#include "employee.h"
#include "filemanager.h"
#include "io.h"
#include "logger.h"
#include "payroll.h"
#include "report.h"
#include "role.h"
#include "validation.h"

/* ------------------------------------------------------------ runner */
typedef void (*TestFn)(void);
void runTest(const char *name, TestFn test, TestFn extraSetUp);
#define RUN_TEST(fn)            runTest(#fn, fn, NULL)
#define RUN_TEST_WITH(fn, prep) runTest(#fn, fn, prep)

void testFail(const char *file, int line, const char *message);
void testFailInt(const char *file, int line, const char *expr, long long expected, long long actual);
void testFailStr(const char *file, int line, const char *expr, const char *expected, const char *actual);
int testCurrentFailed(void);
/** Implemented in test_main.c: counts results per function under test. */
void recordTestResult(const char *name, int failed);

/* ------------------------------------------------------------ checks */
#define EXPECT_TRUE(c) \
    do { if (!(c)) { testFail(__FILE__, __LINE__, "expected true: " #c); } } while (0)
#define EXPECT_FALSE(c) \
    do { if (c) { testFail(__FILE__, __LINE__, "expected false: " #c); } } while (0)
#define EXPECT_EQ(a, b) \
    do { long long ea_ = (long long)(a); long long eb_ = (long long)(b); \
         if (ea_ != eb_) { testFailInt(__FILE__, __LINE__, #a " == " #b, eb_, ea_); } } while (0)
#define EXPECT_NE(a, b) \
    do { if ((long long)(a) == (long long)(b)) { testFail(__FILE__, __LINE__, "expected " #a " != " #b); } } while (0)
#define EXPECT_GT(a, b) \
    do { if (!((long long)(a) > (long long)(b))) { testFail(__FILE__, __LINE__, "expected " #a " > " #b); } } while (0)
#define EXPECT_GE(a, b) \
    do { if (!((long long)(a) >= (long long)(b))) { testFail(__FILE__, __LINE__, "expected " #a " >= " #b); } } while (0)
#define EXPECT_LE(a, b) \
    do { if (!((long long)(a) <= (long long)(b))) { testFail(__FILE__, __LINE__, "expected " #a " <= " #b); } } while (0)
#define EXPECT_PTR_EQ(a, b) \
    do { if ((const void *)(a) != (const void *)(b)) { testFail(__FILE__, __LINE__, "expected " #a " == " #b); } } while (0)
#define EXPECT_NULL(p) \
    do { if ((p) != NULL) { testFail(__FILE__, __LINE__, "expected NULL: " #p); } } while (0)
#define EXPECT_NOT_NULL(p) \
    do { if ((p) == NULL) { testFail(__FILE__, __LINE__, "unexpected NULL: " #p); } } while (0)
#define EXPECT_STREQ(a, b) \
    do { const char *sa_ = (a); const char *sb_ = (b); \
         if ((sa_ == NULL) || (sb_ == NULL) || (strcmp(sa_, sb_) != 0)) { \
             testFailStr(__FILE__, __LINE__, #a, sb_, sa_); } } while (0)
#define EXPECT_DBL_EQ(a, b) \
    do { double da_ = (double)(a); double db_ = (double)(b); double dd_ = da_ - db_; \
         if ((dd_ > 0.000001) || (dd_ < -0.000001)) { \
             char m_[128]; (void)snprintf(m_, sizeof(m_), "%s: expected %.6f, got %.6f", #a, db_, da_); \
             testFail(__FILE__, __LINE__, m_); } } while (0)
#define EXPECT_CONTAINS(text, part) \
    do { const char *ct_ = (text); if ((ct_ == NULL) || (strstr(ct_, (part)) == NULL)) { \
             testFailStr(__FILE__, __LINE__, "text does not contain", (part), "(see output)"); } } while (0)
#define EXPECT_NOT_CONTAINS(text, part) \
    do { const char *ct_ = (text); if ((ct_ != NULL) && (strstr(ct_, (part)) != NULL)) { \
             testFailStr(__FILE__, __LINE__, "text should not contain", (part), "(found)"); } } while (0)

/* ASSERT_* stop the current test on failure */
#define ASSERT_TRUE(c)      do { EXPECT_TRUE(c); if (testCurrentFailed() != 0) { return; } } while (0)
#define ASSERT_EQ(a, b)     do { EXPECT_EQ(a, b); if (testCurrentFailed() != 0) { return; } } while (0)
#define ASSERT_NOT_NULL(p)  do { EXPECT_NOT_NULL(p); if (testCurrentFailed() != 0) { return; } } while (0)

/* ------------------------------------------------------------ fixture */
const char *testDir(void);
const char *testPath(const char *name);
FILE *testInput(void);
FILE *testOutput(void);
/** Scripted keyboard input; every line ends with '\n'. */
void feed(const char *text);
/** Everything printed since the test started. */
const char *getOutput(void);
const char *readFile(const char *name);
const char *logText(void);
void writeFile(const char *name, const char *text);
int fileExists(const char *name);
/** Returns a string of n copies of c (n < 300). */
const char *repeatChar(int n, char c);
/** Number of times part appears in text. */
int countOccurrences(const char *text, const char *part);
Employee makeEmployee(int id);
Employee makeEmployeeFull(int id, const char *name, int dept, int role, double salary);
Date makeDate(int day, int month, int year);
/** Departments 10 Finance, 11 HR; roles 20 Accountant, 21 Executive;
 *  employees 101 Ravi Kumar (10/20, 30000) and 102 Priya Sharma (11/21, 24000). */
void seed(void);

/* ------------------------------------------------------------ suites */
void run_logger_tests(void);
void run_io_tests(void);
void run_validation_tests(void);
void run_auth_tests(void);
void run_department_tests(void);
void run_role_tests(void);
void run_employee_tests(void);
void run_attendance_tests(void);
void run_payroll_tests(void);
void run_report_tests(void);
void run_filemanager_tests(void);
void run_app_tests(void);

#endif /* EMS_TEST_FRAMEWORK_H */
