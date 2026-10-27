/**
 * @file payroll.h
 * @brief Payroll Module: monthly payroll with attendance-based loss of pay.
 *
 *   gross = basic + allowance
 *   lop   = basic / WORKING_DAYS * absent days   (leave days are paid)
 *   net   = gross - deduction - lop
 */
#ifndef EMS_PAYROLL_H
#define EMS_PAYROLL_H

#include <stdio.h>
#include "common.h"

void payrollReset(void);
int payrollGrow(void);
/** Rounds to 2 decimal places (half away from zero). */
double roundMoney(double amount);
double calculateGross(double basic, double allowance);
/** absentDays is capped at WORKING_DAYS. */
double calculateLop(double basic, int absentDays);
/** Never returns less than 0. */
double calculateNet(double gross, double deduction, double lop);
/** Returns the index of the payroll record, or -1. */
int findPayroll(int empId, int month, int year);
/** Builds a payroll record without storing it.
 *  Returns EMS_OK, ERR_NOT_FOUND, ERR_INVALID_INPUT or ERR_PAYROLL_EXISTS. */
int payrollCompute(int empId, int month, int year, double allowance, double deduction, Payroll *out);
/** Stores a computed record. An empty processedAt is stamped with the current time. Returns EMS_OK, ERR_INVALID_INPUT, ERR_PAYROLL_EXISTS or ERR_MEMORY. */
int payrollAdd(const Payroll *record);
/** Removes every payroll record of an employee. Returns how many were removed. */
int payrollRemoveEmployee(int empId);
int getPayrollCount(void);
const Payroll *getPayrollAt(int index);
void printPayslip(FILE *out, const Payroll *record);

/* ---- interactive functions (Admin only, checked by caller) ---- */
int processPayroll(void);
int viewPayslip(void);
void payrollMenu(void);

#endif /* EMS_PAYROLL_H */
