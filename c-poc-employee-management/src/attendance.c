/**
 * @file attendance.c
 * @brief Attendance Module.
 */
#include <stdlib.h>
#include <string.h>
#include "attendance.h"
#include "auth.h"
#include "employee.h"
#include "filemanager.h"
#include "io.h"
#include "logger.h"
#include "payroll.h"
#include "validation.h"

static Attendance *g_att = NULL;
static int g_attCount = 0;
static int g_attCapacity = 0;

void attendanceReset(void)
{
    free(g_att);
    g_att = NULL;
    g_attCount = 0;
    g_attCapacity = 0;
}

int attendanceGrow(void)
{
    int status = EMS_OK;
    int newCapacity;
    Attendance *grown;

    if (g_attCount >= g_attCapacity) {
        newCapacity = (g_attCapacity == 0) ? 16 : (g_attCapacity * 2);
        grown = (Attendance *)realloc(g_att, (size_t)newCapacity * sizeof(Attendance));
        if (grown == NULL) {
            status = ERR_MEMORY;
        } else {
            g_att = grown;
            g_attCapacity = newCapacity;
        }
    }
    return status;
}

int attendanceAdd(int empId, const Date *date, char status, const char *recordedAt)
{
    int result = EMS_OK;

    if (findEmployee(empId) == NULL) {
        result = ERR_NOT_FOUND;
    } else if ((date == NULL) || (validateDate(date->day, date->month, date->year) == 0) ||
               (isFutureDate(date) == 1) || (validateStatus(status) == 0)) {
        result = ERR_INVALID_ATTENDANCE;
    } else if ((findAttendance(empId, date->day, date->month, date->year) >= 0) ||
               (isMonthLocked(empId, date->month, date->year) == 1)) {
        result = ERR_INVALID_ATTENDANCE;
    } else if ((recordedAt != NULL) && (recordedAt[0] != '\0') && (validateTimestamp(recordedAt) == 0)) {
        result = ERR_INVALID_ATTENDANCE;
    } else {
        result = attendanceGrow();
        if (result == EMS_OK) {
            (void)setTimestamp(g_att[g_attCount].recordedAt, recordedAt);
            g_att[g_attCount].empId = empId;
            g_att[g_attCount].date = *date;
            g_att[g_attCount].status = status;
            g_attCount++;
        }
    }
    return result;
}

int findAttendance(int empId, int day, int month, int year)
{
    int index = -1;
    int i;

    for (i = 0; i < g_attCount; i++) {
        if ((g_att[i].empId == empId) && (g_att[i].date.day == day) &&
            (g_att[i].date.month == month) && (g_att[i].date.year == year)) {
            index = i;
            break;
        }
    }
    return index;
}

int isMonthLocked(int empId, int month, int year)
{
    return (findPayroll(empId, month, year) >= 0) ? 1 : 0;
}

void getMonthlySummary(int empId, int month, int year, int *present, int *absent, int *leave)
{
    int i;
    int p = 0;
    int a = 0;
    int l = 0;

    for (i = 0; i < g_attCount; i++) {
        if ((g_att[i].empId == empId) &&
            ((month == 0) || ((g_att[i].date.month == month) && (g_att[i].date.year == year)))) {
            if (g_att[i].status == 'P') {
                p++;
            } else if (g_att[i].status == 'A') {
                a++;
            } else {
                l++;
            }
        }
    }
    if (present != NULL) {
        *present = p;
    }
    if (absent != NULL) {
        *absent = a;
    }
    if (leave != NULL) {
        *leave = l;
    }
}

int attendanceRemoveEmployee(int empId)
{
    int i;
    int keep = 0;
    int removed;

    for (i = 0; i < g_attCount; i++) {
        if (g_att[i].empId != empId) {
            g_att[keep] = g_att[i];
            keep++;
        }
    }
    removed = g_attCount - keep;
    g_attCount = keep;
    return removed;
}

int getAttendanceCount(void)
{
    return g_attCount;
}

const Attendance *getAttendanceAt(int index)
{
    return ((index >= 0) && (index < g_attCount)) ? &g_att[index] : NULL;
}

const char *statusText(char status)
{
    const char *text = "Unknown";

    if (status == 'P') {
        text = "Present";
    } else if (status == 'A') {
        text = "Absent";
    } else if (status == 'L') {
        text = "Leave";
    } else {
        text = "Unknown";
    }
    return text;
}

/* ------------------------------------------------- interactive functions */

int recordAttendance(void)
{
    int status;
    int empId = 0;
    Date date = {0, 0, 0};
    char mark = 'P';
    char msg[LINE_SIZE];

    status = readInt("Employee ID: ", 1, MAX_ID, &empId);
    if ((status == EMS_OK) && (findEmployee(empId) == NULL)) {
        status = ERR_NOT_FOUND;
        showError("ATTENDANCE", ERR_NOT_FOUND);
    }
    if (status == EMS_OK) {
        status = readDate(&date);
    }
    if (status == EMS_OK) {
        status = readChar("Status (P = Present, A = Absent, L = Leave): ", "PAL", &mark);
    }
    if (status == EMS_OK) {
        status = attendanceAdd(empId, &date, mark, NULL);
        if (status == EMS_OK) {
            (void)saveAttendance();
            (void)snprintf(msg, sizeof(msg), "Attendance %c for employee %d on %02d-%02d-%04d by %s", mark,
                           empId, date.day, date.month, date.year, getCurrentUser());
            (void)logMessage(LOG_INFO, "ATTENDANCE", msg);
            (void)fprintf(getOutputStream(), "Recorded %s on %02d-%02d-%04d (entered at %s).\n", statusText(mark),
                          date.day, date.month, date.year, g_att[g_attCount - 1].recordedAt);
        } else {
            if (isFutureDate(&date) == 1) {
                (void)fprintf(getOutputStream(), "Attendance cannot be recorded for a future date.\n");
            } else if (isMonthLocked(empId, date.month, date.year) == 1) {
                (void)fprintf(getOutputStream(), "Payroll for this month already exists. Attendance is locked.\n");
            } else {
                /* duplicate date: message below is enough */
            }
            showError("ATTENDANCE", (EmsStatus)status);
        }
    }
    return status;
}

int attendanceSummary(void)
{
    int status;
    int empId = 0;
    int month = 0;
    int year = 0;
    int p = 0;
    int a = 0;
    int l = 0;
    const Employee *emp = NULL;
    FILE *out = getOutputStream();

    status = readInt("Employee ID: ", 1, MAX_ID, &empId);
    if (status == EMS_OK) {
        emp = findEmployee(empId);
        if (emp == NULL) {
            status = ERR_NOT_FOUND;
            showError("ATTENDANCE", ERR_NOT_FOUND);
        }
    }
    if (status == EMS_OK) {
        status = readInt("Month (1-12): ", 1, 12, &month);
    }
    if (status == EMS_OK) {
        status = readInt("Year (2000-2100): ", MIN_YEAR, MAX_YEAR, &year);
    }
    if (status == EMS_OK) {
        getMonthlySummary(empId, month, year, &p, &a, &l);
        (void)fprintf(out, "\n==========================================\n");
        (void)fprintf(out, "ATTENDANCE SUMMARY\n");
        (void)fprintf(out, "==========================================\n");
        (void)fprintf(out, "Employee          : %d %s\n", emp->id, emp->name);
        (void)fprintf(out, "Month / Year      : %02d / %04d\n", month, year);
        (void)fprintf(out, "Present Days      : %d\n", p);
        (void)fprintf(out, "Absent Days       : %d\n", a);
        (void)fprintf(out, "Leave Days        : %d\n", l);
        (void)fprintf(out, "Total Days Marked : %d\n", p + a + l);
        (void)fprintf(out, "==========================================\n");
    }
    return status;
}

void attendanceMenu(void)
{
    int choice = -1;
    int status = EMS_OK;
    int result;

    while ((choice != 0) && (status == EMS_OK)) {
        (void)fprintf(getOutputStream(), "\n--- Attendance Management ---\n1. Record Attendance\n"
                      "2. Attendance Summary (one month)\n0. Back\n");
        status = readInt("Choose: ", 0, 2, &choice);
        if (status == EMS_OK) {
            result = EMS_OK;
            if (choice == 1) {
                result = recordAttendance();
            } else if (choice == 2) {
                result = attendanceSummary();
            } else {
                choice = 0;
            }
            if (result == ERR_END_OF_INPUT) {
                status = ERR_END_OF_INPUT;
            }
        }
    }
}
