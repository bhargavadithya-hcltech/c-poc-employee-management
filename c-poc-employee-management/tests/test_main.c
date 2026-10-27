/**
 * @file test_main.c
 * @brief Runs every unit test suite and prints, for each of the functions
 *        in src/, how many test cases exercised it.
 *
 * Test functions are named functionUnderTest_Scenario, so the part before
 * the first '_' tells which function a test belongs to.
 */
#include <stdio.h>
#include <string.h>
#include "test_framework.h"

typedef struct {
    const char *file;
    const char *name;
} FunctionInfo;

/* Every function defined in src/ (main.c excluded: it only calls runApplication()). */
static const FunctionInfo g_functions[] = {
    /* app.c */
    {"app.c", "systemInit"},
    {"app.c", "systemCleanup"},
    {"app.c", "displayMainMenu"},
    {"app.c", "mainMenu"},
    {"app.c", "runApplication"},
    /* attendance.c */
    {"attendance.c", "attendanceReset"},
    {"attendance.c", "attendanceGrow"},
    {"attendance.c", "attendanceAdd"},
    {"attendance.c", "findAttendance"},
    {"attendance.c", "isMonthLocked"},
    {"attendance.c", "getMonthlySummary"},
    {"attendance.c", "attendanceRemoveEmployee"},
    {"attendance.c", "getAttendanceCount"},
    {"attendance.c", "getAttendanceAt"},
    {"attendance.c", "statusText"},
    {"attendance.c", "recordAttendance"},
    {"attendance.c", "attendanceSummary"},
    {"attendance.c", "attendanceMenu"},
    /* auth.c */
    {"auth.c", "authReset"},
    {"auth.c", "hashPassword"},
    {"auth.c", "findUser"},
    {"auth.c", "addUserHashed"},
    {"auth.c", "addUserRecord"},
    {"auth.c", "verifyCredentials"},
    {"auth.c", "setSession"},
    {"auth.c", "logout"},
    {"auth.c", "getCurrentRole"},
    {"auth.c", "getCurrentUser"},
    {"auth.c", "roleName"},
    {"auth.c", "getUserCount"},
    {"auth.c", "getUserAt"},
    {"auth.c", "checkAccess"},
    {"auth.c", "loadUsers"},
    {"auth.c", "saveUsers"},
    {"auth.c", "readNewPassword"},
    {"auth.c", "createFirstAdmin"},
    {"auth.c", "login"},
    {"auth.c", "addUser"},
    /* department.c */
    {"department.c", "deptReset"},
    {"department.c", "departmentAdd"},
    {"department.c", "findDept"},
    {"department.c", "findDeptByName"},
    {"department.c", "getDeptName"},
    {"department.c", "getDeptCount"},
    {"department.c", "getDeptAt"},
    {"department.c", "viewDepartments"},
    {"department.c", "createDepartment"},
    {"department.c", "departmentMenu"},
    /* employee.c */
    {"employee.c", "employeeReset"},
    {"employee.c", "validateEmployeeData"},
    {"employee.c", "employeeInsert"},
    {"employee.c", "findEmployee"},
    {"employee.c", "employeeUpdate"},
    {"employee.c", "employeeRemove"},
    {"employee.c", "employeeSetDepartment"},
    {"employee.c", "employeeSetRole"},
    {"employee.c", "employeeMatches"},
    {"employee.c", "employeeSearch"},
    {"employee.c", "countEmployeesInDept"},
    {"employee.c", "countEmployeesInRole"},
    {"employee.c", "getEmployeeHead"},
    {"employee.c", "getEmployeeCount"},
    {"employee.c", "printEmployeeHeader"},
    {"employee.c", "printEmployee"},
    {"employee.c", "selectDepartment"},
    {"employee.c", "selectRole"},
    {"employee.c", "logEmployeeEvent"},
    {"employee.c", "addEmployee"},
    {"employee.c", "viewEmployee"},
    {"employee.c", "searchEmployee"},
    {"employee.c", "updateEmployee"},
    {"employee.c", "deleteEmployee"},
    {"employee.c", "assignDepartment"},
    {"employee.c", "assignRole"},
    {"employee.c", "employeeMenu"},
    /* filemanager.c */
    {"filemanager.c", "setDataDirectory"},
    {"filemanager.c", "getDataDirectory"},
    {"filemanager.c", "buildDataPath"},
    {"filemanager.c", "beginSave"},
    {"filemanager.c", "commitSave"},
    {"filemanager.c", "backupFile"},
    {"filemanager.c", "reportBadLines"},
    {"filemanager.c", "openForLoad"},
    {"filemanager.c", "loadDepartments"},
    {"filemanager.c", "saveDepartments"},
    {"filemanager.c", "loadRoles"},
    {"filemanager.c", "saveRoles"},
    {"filemanager.c", "loadEmployees"},
    {"filemanager.c", "saveEmployees"},
    {"filemanager.c", "loadAttendance"},
    {"filemanager.c", "saveAttendance"},
    {"filemanager.c", "loadPayroll"},
    {"filemanager.c", "savePayroll"},
    {"filemanager.c", "loadAllRecords"},
    {"filemanager.c", "saveAllRecords"},
    {"filemanager.c", "freeAllMemory"},
    /* io.c */
    {"io.c", "setInputStream"},
    {"io.c", "setOutputStream"},
    {"io.c", "getInputStream"},
    {"io.c", "getOutputStream"},
    {"io.c", "trimString"},
    {"io.c", "readLine"},
    {"io.c", "readInt"},
    {"io.c", "readFloat"},
    {"io.c", "readString"},
    {"io.c", "readChar"},
    {"io.c", "readYesNo"},
    {"io.c", "readDate"},
    {"io.c", "readValidated"},
    /* logger.c */
    {"logger.c", "logInit"},
    {"logger.c", "logClose"},
    {"logger.c", "logIsOpen"},
    {"logger.c", "getTimestamp"},
    {"logger.c", "setTimestamp"},
    {"logger.c", "logLevelName"},
    {"logger.c", "logMessage"},
    {"logger.c", "logError"},
    {"logger.c", "errorCodeText"},
    {"logger.c", "errorMessage"},
    {"logger.c", "showError"},
    /* payroll.c */
    {"payroll.c", "payrollReset"},
    {"payroll.c", "payrollGrow"},
    {"payroll.c", "roundMoney"},
    {"payroll.c", "calculateGross"},
    {"payroll.c", "calculateLop"},
    {"payroll.c", "calculateNet"},
    {"payroll.c", "findPayroll"},
    {"payroll.c", "payrollCompute"},
    {"payroll.c", "payrollAdd"},
    {"payroll.c", "payrollRemoveEmployee"},
    {"payroll.c", "getPayrollCount"},
    {"payroll.c", "getPayrollAt"},
    {"payroll.c", "printPayslip"},
    {"payroll.c", "processPayroll"},
    {"payroll.c", "viewPayslip"},
    {"payroll.c", "payrollMenu"},
    /* report.c */
    {"report.c", "printReportTitle"},
    {"report.c", "generateEmployeeReport"},
    {"report.c", "generateAttendanceReport"},
    {"report.c", "generatePayrollReport"},
    {"report.c", "generateReport"},
    {"report.c", "reportMenu"},
    /* role.c */
    {"role.c", "roleReset"},
    {"role.c", "roleAdd"},
    {"role.c", "findRole"},
    {"role.c", "findRoleByTitle"},
    {"role.c", "getRoleTitle"},
    {"role.c", "getRoleCount"},
    {"role.c", "getRoleAt"},
    {"role.c", "viewRoles"},
    {"role.c", "createRole"},
    {"role.c", "roleMenu"},
    /* validation.c */
    {"validation.c", "validateText"},
    {"validation.c", "validateName"},
    {"validation.c", "validatePhone"},
    {"validation.c", "validateEmail"},
    {"validation.c", "validateUsername"},
    {"validation.c", "validatePassword"},
    {"validation.c", "validateId"},
    {"validation.c", "validateSalary"},
    {"validation.c", "validateAmount"},
    {"validation.c", "isLeapYear"},
    {"validation.c", "daysInMonth"},
    {"validation.c", "validateDate"},
    {"validation.c", "validateStatus"},
    {"validation.c", "digitsValue"},
    {"validation.c", "validateTimestamp"},
    {"validation.c", "containsIgnoreCase"},
    {"validation.c", "equalsIgnoreCase"},
    {"validation.c", "getToday"},
    {"validation.c", "isFutureDate"},
    {"validation.c", "isFutureMonth"},
};

#define FUNCTION_COUNT ((int)(sizeof(g_functions) / sizeof(g_functions[0])))

static int g_testsRun = 0;
static int g_testsFailed = 0;
static int g_perFunction[FUNCTION_COUNT];

void recordTestResult(const char *name, int failed)
{
    int i;
    size_t len = strcspn(name, "_");

    g_testsRun++;
    if (failed != 0) {
        g_testsFailed++;
    }
    for (i = 0; i < FUNCTION_COUNT; i++) {
        if ((strlen(g_functions[i].name) == len) && (strncmp(g_functions[i].name, name, len) == 0)) {
            g_perFunction[i]++;
            break;
        }
    }
}

static void printFunctionTable(void)
{
    int i;
    int untested = 0;
    const char *file = "";

    (void)printf("\n================ FUNCTIONS AND THEIR TEST CASES ================\n");
    for (i = 0; i < FUNCTION_COUNT; i++) {
        if (strcmp(file, g_functions[i].file) != 0) {
            file = g_functions[i].file;
            (void)printf("\n%s\n", file);
        }
        (void)printf("  %-28s %3d test(s)%s\n", g_functions[i].name, g_perFunction[i],
                     (g_perFunction[i] == 0) ? "   <-- NOT TESTED" : "");
        if (g_perFunction[i] == 0) {
            untested++;
        }
    }
    (void)printf("\nFunctions in modules      : %d\n", FUNCTION_COUNT);
    (void)printf("Functions incl. main()    : %d\n", FUNCTION_COUNT + 1);
    (void)printf("Functions without a test  : %d\n", untested);
    (void)printf("Test cases run            : %d\n", g_testsRun);
}

int main(void)
{
    run_logger_tests();
    run_io_tests();
    run_validation_tests();
    run_auth_tests();
    run_department_tests();
    run_role_tests();
    run_employee_tests();
    run_attendance_tests();
    run_payroll_tests();
    run_report_tests();
    run_filemanager_tests();
    run_app_tests();

    printFunctionTable();
    (void)printf("\n[==========] %d tests ran.\n", g_testsRun);
    if (g_testsFailed == 0) {
        (void)printf("[  PASSED  ] %d tests.\n", g_testsRun);
    } else {
        (void)printf("[  FAILED  ] %d of %d tests.\n", g_testsFailed, g_testsRun);
    }
    return (g_testsFailed == 0) ? 0 : 1;
}
