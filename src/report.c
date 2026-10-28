/**
 * @file report.c
 * @brief Report Module.
 */
#include "report.h"
#include "attendance.h"
#include "auth.h"
#include "department.h"
#include "employee.h"
#include "io.h"
#include "logger.h"
#include "payroll.h"
#include "role.h"

#define REPORT_SEP  "===================================================="
#define REPORT_DASH "----------------------------------------------------"

void printReportTitle(FILE *out, const char *title)
{
    if ((out != NULL) && (title != NULL)) {
        char stamp[32];
        (void)getTimestamp(stamp, sizeof(stamp));
        (void)fprintf(out, "\n%s\n%s\n%s\nGenerated : %s by %s\n", REPORT_SEP, title, REPORT_SEP, stamp,
                      getCurrentUser());
    }
}

int generateEmployeeReport(FILE *out)
{
    int rows = 0;
    const Employee *e;

    if (out != NULL) {
        printReportTitle(out, "EMPLOYEE REPORT");
        (void)fprintf(out, "%-7s | %-20s | %-14s | %-14s | %12s | %-19s\n", "ID", "Name", "Department", "Role",
                      "Basic Salary", "Created");
        for (e = getEmployeeHead(); e != NULL; e = e->next) {
            (void)fprintf(out, "%-7d | %-20.20s | %-14.14s | %-14.14s | %12.2f | %-19s\n", e->id, e->name,
                          getDeptName(e->deptId), getRoleTitle(e->roleId), e->basicSalary, e->createdAt);
            rows++;
        }
        (void)fprintf(out, "%s\nTotal Employees : %d\n%s\n", REPORT_DASH, rows, REPORT_SEP);
    }
    return rows;
}

int generateAttendanceReport(FILE *out, int month, int year)
{
    int rows = 0;
    int p = 0;
    int a = 0;
    int l = 0;
    const Employee *e;

    if (out != NULL) {
        printReportTitle(out, "ATTENDANCE SUMMARY REPORT");
        (void)fprintf(out, "Month / Year : %02d / %04d\n", month, year);
        (void)fprintf(out, "%-7s | %-20s | %7s | %6s | %5s\n", "ID", "Name", "Present", "Absent", "Leave");
        for (e = getEmployeeHead(); e != NULL; e = e->next) {
            getMonthlySummary(e->id, month, year, &p, &a, &l);
            (void)fprintf(out, "%-7d | %-20.20s | %7d | %6d | %5d\n", e->id, e->name, p, a, l);
            rows++;
        }
        (void)fprintf(out, "%s\nTotal Employees : %d\n%s\n", REPORT_DASH, rows, REPORT_SEP);
    }
    return rows;
}

int generatePayrollReport(FILE *out, int month, int year)
{
    int rows = 0;

    if (out != NULL) {
        int i;
        double totalNet = 0.0;
        printReportTitle(out, "PAYROLL REPORT");
        (void)fprintf(out, "Month / Year : %02d / %04d\n", month, year);
        (void)fprintf(out, "%-7s | %10s | %9s | %9s | %9s | %10s | %10s | %-19s\n", "ID", "Basic", "Allow.",
                      "Deduct.", "LOP", "Gross", "Net", "Processed On");
        for (i = 0; i < getPayrollCount(); i++) {
            const Payroll *p;
            p = getPayrollAt(i);
            if ((p->month == month) && (p->year == year)) {
                (void)fprintf(out, "%-7d | %10.2f | %9.2f | %9.2f | %9.2f | %10.2f | %10.2f | %-19s\n", p->empId,
                              p->salary.basic, p->salary.allowance, p->salary.deduction, p->salary.lop,
                              p->salary.gross, p->salary.net, p->processedAt);
                totalNet += p->salary.net;
                rows++;
            }
        }
        (void)fprintf(out, "%s\nTotal Net Payout : %.2f\n%s\n", REPORT_DASH, totalNet, REPORT_SEP);
    }
    return rows;
}

int generateReport(int type)
{
    int status = EMS_OK;
    int month = 0;
    int year = 0;
    int rows;
    char msg[LINE_SIZE];

    if ((type == REPORT_PAYROLL) && (checkAccess(ROLE_ADMIN) == 0)) {
        status = ERR_ACCESS_DENIED;
    } else if ((type != REPORT_EMPLOYEE) && (type != REPORT_ATTENDANCE) && (type != REPORT_PAYROLL)) {
        status = ERR_INVALID_INPUT;
    } else {
        if (type != REPORT_EMPLOYEE) {
            status = readInt("Month (1-12): ", 1, 12, &month);
            if (status == EMS_OK) {
                status = readInt("Year (2000-2100): ", MIN_YEAR, MAX_YEAR, &year);
            }
        }
        if (status == EMS_OK) {
            if (type == REPORT_EMPLOYEE) {
                rows = generateEmployeeReport(getOutputStream());
            } else if (type == REPORT_ATTENDANCE) {
                rows = generateAttendanceReport(getOutputStream(), month, year);
            } else {
                rows = generatePayrollReport(getOutputStream(), month, year);
            }
            if (rows == 0) {
                status = ERR_NOT_FOUND;
                showError("REPORT", ERR_NOT_FOUND);
            }
            (void)snprintf(msg, sizeof(msg), "Report type %d generated with %d row(s) by %s", type, rows,
                           getCurrentUser());
            (void)logMessage(LOG_INFO, "REPORT", msg);
        }
    }
    return status;
}

void reportMenu(void)
{
    int choice = -1;
    int status = EMS_OK;

    while ((choice != 0) && (status == EMS_OK)) {
        (void)fprintf(getOutputStream(), "\n--- Reports ---\n1. Employee Report\n2. Attendance Report\n"
                      "3. Payroll Report (Admin)\n0. Back\n");
        status = readInt("Choose: ", 0, 3, &choice);
        if ((status == EMS_OK) && (choice != 0)) {
            if (generateReport(choice) == ERR_END_OF_INPUT) {
                status = ERR_END_OF_INPUT;
            }
        }
    }
}
