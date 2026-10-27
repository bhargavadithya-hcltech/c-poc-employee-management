/**
 * @file app.c
 * @brief Application layer: initialization, main menu and cleanup.
 */
#include <stdio.h>
#include <stdlib.h>
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

int systemInit(const char *dataDir, const char *logPath)
{
    int status;

    freeAllMemory();
    if (logInit((logPath != NULL) ? logPath : DEFAULT_LOG_FILE) != EMS_OK) {
        (void)fprintf(getOutputStream(), "Warning: log file could not be opened. Continuing without a log.\n");
    }
    status = setDataDirectory((dataDir != NULL) ? dataDir : DEFAULT_DATA_DIR);
    if (status == EMS_OK) {
        (void)logMessage(LOG_INFO, "APP", "System started");
        status = loadUsers();
    }
    return status;
}

void systemCleanup(void)
{
    freeAllMemory();
    (void)logMessage(LOG_INFO, "APP", "System stopped, memory released");
    logClose();
}

void displayMainMenu(void)
{
    (void)fprintf(getOutputStream(),
                  "\n==========================================\n"
                  "EMPLOYEE MANAGEMENT SYSTEM\n"
                  "==========================================\n"
                  "Logged in as : %s (%s)\n"
                  "Employees: %d | Departments: %d | Roles: %d\n"
                  "1. Employee Management\n"
                  "2. Department Management (Admin)\n"
                  "3. Role Management (Admin)\n"
                  "4. Attendance Management\n"
                  "5. Payroll Management (Admin)\n"
                  "6. Reports\n"
                  "7. Save and Exit\n"
                  "8. Add User (Admin)\n",
                  getCurrentUser(), roleName(getCurrentRole()), getEmployeeCount(), getDeptCount(),
                  getRoleCount());
}

int mainMenu(void)
{
    int status = EMS_OK;
    int choice = 0;

    while ((status == EMS_OK) && (choice != 7)) {
        displayMainMenu();
        status = readInt("Enter Choice : ", 1, 8, &choice);
        if (status == EMS_OK) {
            switch (choice) {
            case 1: employeeMenu(); break;
            case 2: if (checkAccess(ROLE_ADMIN) == 1) { departmentMenu(); } break;
            case 3: if (checkAccess(ROLE_ADMIN) == 1) { roleMenu(); } break;
            case 4: attendanceMenu(); break;
            case 5: if (checkAccess(ROLE_ADMIN) == 1) { payrollMenu(); } break;
            case 6: reportMenu(); break;
            case 8: if (checkAccess(ROLE_ADMIN) == 1) { (void)addUser(); } break;
            default: break;   /* 7 = Save and Exit */
            }
        }
    }
    return status;
}

int runApplication(const char *dataDir, const char *logPath)
{
    int status;
    int exitCode = EXIT_SUCCESS;

    status = systemInit(dataDir, logPath);
    if (status == EMS_OK) {
        status = login();
    }
    if (status == EMS_OK) {
        status = loadAllRecords();
    }
    if (status == EMS_OK) {
        status = mainMenu();
        if (status == ERR_END_OF_INPUT) {
            (void)fprintf(getOutputStream(), "\nInput closed. Saving and exiting safely.\n");
        }
        if (saveAllRecords() != EMS_OK) {
            exitCode = EXIT_FAILURE;
        }
        (void)fprintf(getOutputStream(), "Data saved. Goodbye.\n");
    } else {
        if (status == ERR_LOGIN) {
            (void)logError("APP", ERR_LOGIN, "too many failed attempts");
        }
        exitCode = EXIT_FAILURE;
    }
    logout();
    systemCleanup();
    return exitCode;
}
