/**
 * @file payroll.c
 * @brief Payroll Module.
 */
#include <stdlib.h>
#include <string.h>
#include "payroll.h"
#include "attendance.h"
#include "auth.h"
#include "employee.h"
#include "filemanager.h"
#include "io.h"
#include "logger.h"
#include "validation.h"

static Payroll *g_pay = NULL;
static int g_payCount = 0;
static int g_payCapacity = 0;

void payrollReset(void)
{
    free(g_pay);
    g_pay = NULL;
    g_payCount = 0;
    g_payCapacity = 0;
}

int payrollGrow(void)
{
    int status = EMS_OK;
    Payroll *grown;

    if (g_payCount >= g_payCapacity) {
        int newCapacity;
        newCapacity = (g_payCapacity == 0) ? 16 : (g_payCapacity * 2);
        grown = (Payroll *)realloc(g_pay, (size_t)newCapacity * sizeof(Payroll));
        if (grown == NULL) {
            status = ERR_MEMORY;
        } else {
            g_pay = grown;
            g_payCapacity = newCapacity;
        }
    }
    return status;
}

double roundMoney(double amount)
{
    double scaled = amount * 100.0;
    long whole;

    if (scaled >= 0.0) {
        whole = (long)(scaled + 0.5);
    } else {
        whole = (long)(scaled - 0.5);
    }
    return (double)whole / 100.0;
}

double calculateGross(double basic, double allowance)
{
    return roundMoney(basic + allowance);
}

double calculateLop(double basic, int absentDays)
{
    int days = absentDays;

    if (days < 0) {
        days = 0;
    }
    if (days > WORKING_DAYS) {
        days = WORKING_DAYS;
    }
    return roundMoney((basic / (double)WORKING_DAYS) * (double)days);
}

double calculateNet(double gross, double deduction, double lop)
{
    double net = roundMoney(gross - deduction - lop);

    return (net < 0.0) ? 0.0 : net;
}

int findPayroll(int empId, int month, int year)
{
    int index = -1;
    int i;

    for (i = 0; i < g_payCount; i++) {
        if ((g_pay[i].empId == empId) && (g_pay[i].month == month) && (g_pay[i].year == year)) {
            index = i;
            break;
        }
    }
    return index;
}

int payrollCompute(int empId, int month, int year, double allowance, double deduction, Payroll *out)
{
    int status = EMS_OK;
    const Employee *emp = findEmployee(empId);
    int present = 0;
    int absent = 0;
    int leave = 0;

    if (out == NULL) {
        status = ERR_INVALID_INPUT;
    } else if (emp == NULL) {
        status = ERR_NOT_FOUND;
    } else if ((month < 1) || (month > 12) || (year < MIN_YEAR) || (year > MAX_YEAR) ||
               (isFutureMonth(month, year) == 1) || (validateAmount(allowance) == 0) ||
               (validateAmount(deduction) == 0)) {
        status = ERR_INVALID_INPUT;
    } else if (findPayroll(empId, month, year) >= 0) {
        status = ERR_PAYROLL_EXISTS;
    } else {
        getMonthlySummary(empId, month, year, &present, &absent, &leave);
        (void)memset(out, 0, sizeof(Payroll));
        out->empId = empId;
        out->month = month;
        out->year = year;
        out->absentDays = absent;
        out->salary.basic = emp->basicSalary;
        out->salary.allowance = roundMoney(allowance);
        out->salary.deduction = roundMoney(deduction);
        out->salary.lop = calculateLop(emp->basicSalary, absent);
        out->salary.gross = calculateGross(emp->basicSalary, allowance);
        if ((out->salary.deduction + out->salary.lop) > (out->salary.gross + 0.005)) {
            status = ERR_INVALID_INPUT;
        } else {
            out->salary.net = calculateNet(out->salary.gross, out->salary.deduction, out->salary.lop);
        }
    }
    return status;
}

int payrollAdd(const Payroll *record)
{
    int status = EMS_OK;

    if ((record == NULL) || (findEmployee(record->empId) == NULL) || (record->month < 1) ||
        (record->month > 12) || (record->year < MIN_YEAR) || (record->year > MAX_YEAR) ||
        (record->absentDays < 0) || (record->salary.net < 0.0) ||
        (validateSalary(record->salary.basic) == 0)) {
        status = ERR_INVALID_INPUT;
    } else if (findPayroll(record->empId, record->month, record->year) >= 0) {
        status = ERR_PAYROLL_EXISTS;
    } else if ((record->processedAt[0] != '\0') && (validateTimestamp(record->processedAt) == 0)) {
        status = ERR_INVALID_INPUT;
    } else {
        status = payrollGrow();
        if (status == EMS_OK) {
            g_pay[g_payCount] = *record;
            (void)setTimestamp(g_pay[g_payCount].processedAt, record->processedAt);
            g_payCount++;
        }
    }
    return status;
}

int payrollRemoveEmployee(int empId)
{
    int i;
    int keep = 0;
    int removed;

    for (i = 0; i < g_payCount; i++) {
        if (g_pay[i].empId != empId) {
            g_pay[keep] = g_pay[i];
            keep++;
        }
    }
    removed = g_payCount - keep;
    g_payCount = keep;
    return removed;
}

int getPayrollCount(void)
{
    return g_payCount;
}

const Payroll *getPayrollAt(int index)
{
    return ((index >= 0) && (index < g_payCount)) ? &g_pay[index] : NULL;
}

void printPayslip(FILE *out, const Payroll *record)
{
    if ((out != NULL) && (record != NULL)) {
        const Employee *emp;
        emp = findEmployee(record->empId);
        (void)fprintf(out, "\n---------------------------------------\n");
        (void)fprintf(out, "PAYSLIP - %02d / %04d\n", record->month, record->year);
        (void)fprintf(out, "---------------------------------------\n");
        (void)fprintf(out, "Employee          : %d %s\n", record->empId, (emp != NULL) ? emp->name : "");
        (void)fprintf(out, "Basic Salary      : %.2f\n", record->salary.basic);
        (void)fprintf(out, "Allowance         : %.2f\n", record->salary.allowance);
        (void)fprintf(out, "Gross Salary      : %.2f\n", record->salary.gross);
        (void)fprintf(out, "Deduction         : %.2f\n", record->salary.deduction);
        (void)fprintf(out, "Absent Days (LOP) : %d (%.2f)\n", record->absentDays, record->salary.lop);
        (void)fprintf(out, "Net Salary        : %.2f\n", record->salary.net);
        (void)fprintf(out, "Processed On      : %s\n",
                      (record->processedAt[0] != '\0') ? record->processedAt : "(not saved yet)");
        (void)fprintf(out, "---------------------------------------\n");
    }
}

/* ------------------------------------------------- interactive functions */

int processPayroll(void)
{
    int status;
    int empId = 0;
    int month = 0;
    int year = 0;
    int yes = 0;
    double allowance = 0.0;
    double deduction = 0.0;
    Payroll record;

    status = readInt("Employee ID: ", 1, MAX_ID, &empId);
    if ((status == EMS_OK) && (findEmployee(empId) == NULL)) {
        status = ERR_NOT_FOUND;
        showError("PAYROLL", ERR_NOT_FOUND);
    }
    if (status == EMS_OK) {
        status = readInt("Month (1-12): ", 1, 12, &month);
    }
    if (status == EMS_OK) {
        status = readInt("Year (2000-2100): ", MIN_YEAR, MAX_YEAR, &year);
    }
    if ((status == EMS_OK) && (isFutureMonth(month, year) == 1)) {
        (void)fprintf(getOutputStream(), "Payroll cannot be processed for a future month.\n");
        status = ERR_INVALID_INPUT;
        showError("PAYROLL", ERR_INVALID_INPUT);
    }
    if ((status == EMS_OK) && (findPayroll(empId, month, year) >= 0)) {
        status = ERR_PAYROLL_EXISTS;
        showError("PAYROLL", ERR_PAYROLL_EXISTS);
    }
    if (status == EMS_OK) {
        status = readFloat("Allowance: ", 0.0, MAX_MONEY, &allowance);
    }
    if (status == EMS_OK) {
        status = readFloat("Deduction: ", 0.0, MAX_MONEY, &deduction);
    }
    if (status == EMS_OK) {
        status = payrollCompute(empId, month, year, allowance, deduction, &record);
        if (status != EMS_OK) {
            (void)fprintf(getOutputStream(), "Deductions and loss of pay are more than gross salary.\n");
            showError("PAYROLL", (EmsStatus)status);
        }
    }
    if (status == EMS_OK) {
        printPayslip(getOutputStream(), &record);
        status = readYesNo("Save this payroll? (y/n): ", &yes);
    }
    if ((status == EMS_OK) && (yes == 1)) {
        status = payrollAdd(&record);
        if (status == EMS_OK) {
            char msg[LINE_SIZE];
            (void)savePayroll();
            (void)snprintf(msg, sizeof(msg), "Payroll %02d/%04d processed for employee %d by %s, net %.2f", month,
                           year, empId, getCurrentUser(), record.salary.net);
            (void)logMessage(LOG_INFO, "PAYROLL", msg);
            (void)fprintf(getOutputStream(), "Payroll saved. Processed on %s.\n", g_pay[g_payCount - 1].processedAt);
        } else {
            showError("PAYROLL", (EmsStatus)status);
        }
    } else if (status == EMS_OK) {
        (void)fprintf(getOutputStream(), "Payroll not saved.\n");
    } else {
        /* error already reported */
    }
    return status;
}

int viewPayslip(void)
{
    int status;
    int empId = 0;
    int month = 0;
    int year = 0;

    status = readInt("Employee ID: ", 1, MAX_ID, &empId);
    if (status == EMS_OK) {
        status = readInt("Month (1-12): ", 1, 12, &month);
    }
    if (status == EMS_OK) {
        status = readInt("Year (2000-2100): ", MIN_YEAR, MAX_YEAR, &year);
    }
    if (status == EMS_OK) {
        int index;
        index = findPayroll(empId, month, year);
        if (index < 0) {
            status = ERR_NOT_FOUND;
            showError("PAYROLL", ERR_NOT_FOUND);
        } else {
            printPayslip(getOutputStream(), &g_pay[index]);
        }
    }
    return status;
}

void payrollMenu(void)
{
    int choice = -1;
    int status = EMS_OK;
    int result;

    while ((choice != 0) && (status == EMS_OK)) {
        (void)fprintf(getOutputStream(), "\n--- Payroll Management ---\n1. Process Monthly Payroll\n"
                      "2. View Payslip\n0. Back\n");
        status = readInt("Choose: ", 0, 2, &choice);
        if (status == EMS_OK) {
            result = EMS_OK;
            if (choice == 1) {
                result = processPayroll();
            } else if (choice == 2) {
                result = viewPayslip();
            } else {
                choice = 0;
            }
            if (result == ERR_END_OF_INPUT) {
                status = ERR_END_OF_INPUT;
            }
        }
    }
}
