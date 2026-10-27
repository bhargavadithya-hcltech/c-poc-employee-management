/**
 * @file report.h
 * @brief Report Module: employee, attendance and payroll reports built from
 *        in-memory data. Each generator writes to the given stream and
 *        returns the number of rows printed.
 */
#ifndef EMS_REPORT_H
#define EMS_REPORT_H

#include <stdio.h>
#include "common.h"

/** Prints the report banner with timestamp and user. */
void printReportTitle(FILE *out, const char *title);
int generateEmployeeReport(FILE *out);
int generateAttendanceReport(FILE *out, int month, int year);
int generatePayrollReport(FILE *out, int month, int year);

/* ---- interactive functions ---- */
/** Asks for month/year where needed and prints one report (payroll = Admin only). */
int generateReport(int type);
void reportMenu(void);

#endif /* EMS_REPORT_H */
