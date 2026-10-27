/**
 * @file common.h
 * @brief Shared constants, error codes and data structures for the
 *        Employee Management System (EMS).
 */
#ifndef EMS_COMMON_H
#define EMS_COMMON_H

#include <stddef.h>

/* ---------------------------------------------------------------- limits */
#define MAX_USERS        20
#define MAX_DEPARTMENTS  100
#define MAX_ROLES        100
#define NAME_SIZE        50
#define TITLE_SIZE       30
#define PHONE_SIZE       11      /* 10 digits + terminator */
#define EMAIL_SIZE       50
#define USERNAME_SIZE    20
#define PASSWORD_SIZE    30
#define LINE_SIZE        256
#define PATH_SIZE        256
#define TIMESTAMP_SIZE   20      /* "YYYY-MM-DD HH:MM:SS" + terminator */
#define MAX_ID           999999
#define MIN_YEAR         2000
#define MAX_YEAR         2100
#define MAX_MONEY        10000000.0
#define WORKING_DAYS     30      /* LOP = basic / WORKING_DAYS * absent days */
#define MAX_LOGIN_TRIES  3

/* ----------------------------------------------------------------- roles */
#define ROLE_NONE        0
#define ROLE_ADMIN       1
#define ROLE_OPERATOR    2

/* -------------------------------------------------- search / report type */
#define SEARCH_BY_ID     1
#define SEARCH_BY_NAME   2
#define SEARCH_BY_DEPT   3
#define SEARCH_BY_ROLE   4

#define REPORT_EMPLOYEE   1
#define REPORT_ATTENDANCE 2
#define REPORT_PAYROLL    3

/* ----------------------------------------------------------- error codes */
/* Plain int constants (not an enum) so that status values and int status
 * variables share one essential type (MISRA C:2012 Rule 10.4). */
typedef int EmsStatus;
#define EMS_OK                  0   /* success                         */
#define ERR_INVALID_INPUT       1   /* E001                            */
#define ERR_DUPLICATE_ID        2   /* E002                            */
#define ERR_NOT_FOUND           3   /* E003                            */
#define ERR_INVALID_ATTENDANCE  4   /* E004                            */
#define ERR_PAYROLL_EXISTS      5   /* E005                            */
#define ERR_DEPT_ROLE_NOT_FOUND 6   /* E006                            */
#define ERR_FILE                7   /* E007                            */
#define ERR_MEMORY              8   /* E008                            */
#define ERR_LOGIN               9   /* E009                            */
#define ERR_ACCESS_DENIED       10  /* E010                            */
#define ERR_END_OF_INPUT        11  /* input stream closed (internal)  */
#define ERR_LIMIT_REACHED       12  /* fixed array is full (internal)  */

/* ------------------------------------------------------- data structures */
typedef struct {
    int day;
    int month;
    int year;
} Date;

typedef struct Employee {
    int id;
    char name[NAME_SIZE];
    char phone[PHONE_SIZE];
    char email[EMAIL_SIZE];
    int deptId;
    int roleId;
    double basicSalary;
    char createdAt[TIMESTAMP_SIZE];   /* when the employee was added   */
    char updatedAt[TIMESTAMP_SIZE];   /* last change to the record     */
    struct Employee *next;       /* singly linked list, kept sorted by id */
} Employee;

typedef struct {
    int id;
    char name[TITLE_SIZE];
    char createdAt[TIMESTAMP_SIZE];
} Department;

typedef struct {
    int id;
    char title[TITLE_SIZE];
    char createdAt[TIMESTAMP_SIZE];
} Role;

/* structure composition: Date inside Attendance */
typedef struct {
    int empId;
    Date date;
    char status;                 /* 'P', 'A' or 'L' */
    char recordedAt[TIMESTAMP_SIZE];
} Attendance;

typedef struct {
    double basic;
    double allowance;
    double deduction;
    double lop;                  /* loss of pay */
    double gross;
    double net;
} SalaryBreakup;

/* structure composition: SalaryBreakup inside Payroll */
typedef struct {
    int empId;
    int month;
    int year;
    int absentDays;
    SalaryBreakup salary;
    char processedAt[TIMESTAMP_SIZE];  /* when the pay was processed */
} Payroll;

typedef struct {
    char username[USERNAME_SIZE];
    unsigned long passwordHash;
    int role;                    /* ROLE_ADMIN or ROLE_OPERATOR */
    char createdAt[TIMESTAMP_SIZE];
} User;

#endif /* EMS_COMMON_H */
