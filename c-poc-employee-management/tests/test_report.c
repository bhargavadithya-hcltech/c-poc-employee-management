/** @file test_report.c  Unit tests for the Report module. */
#include "test_framework.h"

static void reportSetUp(void)
{
    Date d1 = makeDate(1, 3, 2025);
    Date d2 = makeDate(2, 3, 2025);
    Payroll p;

    seed();
    EXPECT_EQ(attendanceAdd(101, &d1, 'P', NULL), EMS_OK);
    EXPECT_EQ(attendanceAdd(101, &d2, 'A', NULL), EMS_OK);
    EXPECT_EQ(payrollCompute(101, 3, 2025, 5000.0, 2000.0, &p), EMS_OK);
    EXPECT_EQ(payrollAdd(&p), EMS_OK);
}


static void printReportTitle_HasTitleTimeAndUser(void)
{
    printReportTitle(getOutputStream(), "TEST REPORT");
    printReportTitle(NULL, "X");
    printReportTitle(getOutputStream(), NULL);
    const char *out = getOutput();
    EXPECT_CONTAINS(out, "TEST REPORT");
    EXPECT_CONTAINS(out, "Generated : ");
    EXPECT_CONTAINS(out, "by admin");
}

static void generateEmployeeReport_AllEmployees(void)
{
    EXPECT_EQ(generateEmployeeReport(getOutputStream()), 2);
    const char *out = getOutput();
    EXPECT_CONTAINS(out, "EMPLOYEE REPORT");
    EXPECT_CONTAINS(out, "Priya Sharma");
    EXPECT_CONTAINS(out, "Total Employees : 2");
}

static void generateEmployeeReport_NullStreamAndEmpty(void)
{
    EXPECT_EQ(generateEmployeeReport(NULL), 0);
    employeeReset();
    EXPECT_EQ(generateEmployeeReport(getOutputStream()), 0);
}

static void generateAttendanceReport_CountsPerEmployee(void)
{
    EXPECT_EQ(generateAttendanceReport(getOutputStream(), 3, 2025), 2);
    const char *out = getOutput();
    EXPECT_CONTAINS(out, "Month / Year : 03 / 2025");
    EXPECT_CONTAINS(out, "|       1 |      1 |     0");
    EXPECT_EQ(generateAttendanceReport(NULL, 3, 2025), 0);
}

static void generatePayrollReport_FiltersByMonthAndTotals(void)
{
    EXPECT_EQ(generatePayrollReport(getOutputStream(), 3, 2025), 1);
    EXPECT_CONTAINS(getOutput(), "Total Net Payout : 32000.00");
    EXPECT_EQ(generatePayrollReport(getOutputStream(), 4, 2025), 0);
    EXPECT_EQ(generatePayrollReport(NULL, 3, 2025), 0);
}

static void generateReport_EmployeeType(void)
{
    EXPECT_EQ(generateReport(REPORT_EMPLOYEE), EMS_OK);
    EXPECT_CONTAINS(logText(), "Report type 1 generated with 2 row(s) by admin");
}

static void generateReport_AttendanceAsksForMonth(void)
{
    feed("3\n2025\n");
    EXPECT_EQ(generateReport(REPORT_ATTENDANCE), EMS_OK);
}

static void generateReport_PayrollAdminOnly(void)
{
    feed("3\n2025\n");
    EXPECT_EQ(generateReport(REPORT_PAYROLL), EMS_OK);
    setSession("hr_user", ROLE_OPERATOR);
    EXPECT_EQ(generateReport(REPORT_PAYROLL), ERR_ACCESS_DENIED);
    EXPECT_CONTAINS(getOutput(), "E010");
}

static void generateReport_NoRowsGivesE003(void)
{
    feed("4\n2025\n");
    EXPECT_EQ(generateReport(REPORT_PAYROLL), ERR_NOT_FOUND);
}

static void generateReport_InvalidTypeAndEof(void)
{
    EXPECT_EQ(generateReport(9), ERR_INVALID_INPUT);
    feed("3\n");
    EXPECT_EQ(generateReport(REPORT_ATTENDANCE), ERR_END_OF_INPUT);
}

static void reportMenu_AllReportsThenBack(void)
{
    feed("1\n2\n3\n2025\n3\n3\n2025\n0\n");
    reportMenu();
    const char *out = getOutput();
    EXPECT_CONTAINS(out, "EMPLOYEE REPORT");
    EXPECT_CONTAINS(out, "ATTENDANCE SUMMARY REPORT");
    EXPECT_CONTAINS(out, "PAYROLL REPORT");
}

static void reportMenu_EndOfInput(void)
{
    feed("2\n3\n");
    reportMenu();
    EXPECT_NOT_CONTAINS(getOutput(), "ATTENDANCE SUMMARY REPORT");
}

static void generatePayrollReport_ShowsProcessedTime(void)
{
    EXPECT_EQ(generatePayrollReport(getOutputStream(), 3, 2025), 1);
    EXPECT_CONTAINS(getOutput(), "Processed On");
    EXPECT_CONTAINS(getOutput(), getPayrollAt(0)->processedAt);
}

static void generateEmployeeReport_ShowsCreatedTime(void)
{
    EXPECT_EQ(generateEmployeeReport(getOutputStream()), 2);
    EXPECT_CONTAINS(getOutput(), findEmployee(101)->createdAt);
}

void run_report_tests(void)
{
    RUN_TEST_WITH(printReportTitle_HasTitleTimeAndUser, reportSetUp);
    RUN_TEST_WITH(generateEmployeeReport_AllEmployees, reportSetUp);
    RUN_TEST_WITH(generateEmployeeReport_NullStreamAndEmpty, reportSetUp);
    RUN_TEST_WITH(generateAttendanceReport_CountsPerEmployee, reportSetUp);
    RUN_TEST_WITH(generatePayrollReport_FiltersByMonthAndTotals, reportSetUp);
    RUN_TEST_WITH(generateReport_EmployeeType, reportSetUp);
    RUN_TEST_WITH(generateReport_AttendanceAsksForMonth, reportSetUp);
    RUN_TEST_WITH(generateReport_PayrollAdminOnly, reportSetUp);
    RUN_TEST_WITH(generateReport_NoRowsGivesE003, reportSetUp);
    RUN_TEST_WITH(generateReport_InvalidTypeAndEof, reportSetUp);
    RUN_TEST_WITH(reportMenu_AllReportsThenBack, reportSetUp);
    RUN_TEST_WITH(reportMenu_EndOfInput, reportSetUp);
    RUN_TEST_WITH(generatePayrollReport_ShowsProcessedTime, reportSetUp);
    RUN_TEST_WITH(generateEmployeeReport_ShowsCreatedTime, reportSetUp);
}
