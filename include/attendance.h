/**
 * @file attendance.h
 * @brief Attendance Module: dynamic array of daily attendance records.
 */
#ifndef EMS_ATTENDANCE_H
#define EMS_ATTENDANCE_H

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Frees the array and resets the counters. */
void attendanceReset(void);
/** Makes room for one more record (doubles the array when full). Returns EMS_OK or ERR_MEMORY. */
int attendanceGrow(void);
/** Adds one record after all checks. recordedAt = NULL stamps the current time.
 *  Returns EMS_OK, ERR_NOT_FOUND (no employee), ERR_INVALID_ATTENDANCE
 *  (bad/future date, bad status, duplicate, month locked by payroll) or ERR_MEMORY. */
int attendanceAdd(int empId, const Date *date, char status, const char *recordedAt);
/** Returns the index of the record, or -1. */
int findAttendance(int empId, int day, int month, int year);
/** 1 when payroll already exists for this employee and month (attendance is frozen). */
int isMonthLocked(int empId, int month, int year);
/** Counts P/A/L days. month == 0 counts every month. */
void getMonthlySummary(int empId, int month, int year, int *present, int *absent, int *leave);
/** Removes every record of an employee. Returns how many were removed. */
int attendanceRemoveEmployee(int empId);
int getAttendanceCount(void);
const Attendance *getAttendanceAt(int index);
/** "Present", "Absent", "Leave" or "Unknown". */
const char *statusText(char status);

/* ---- interactive functions ---- */
int recordAttendance(void);
int attendanceSummary(void);
void attendanceMenu(void);

#ifdef __cplusplus
}
#endif

#endif /* EMS_ATTENDANCE_H */
