/**
 * @file filemanager.c
 * @brief File Management Module.
 */
#include <stdio.h>
#include <string.h>
#include "filemanager.h"
#include "attendance.h"
#include "auth.h"
#include "department.h"
#include "employee.h"
#include "io.h"
#include "logger.h"
#include "payroll.h"
#include "role.h"

#define DEPT_FILE  "departments.txt"
#define ROLE_FILE  "roles.txt"
#define EMP_FILE   "employees.txt"
#define ATT_FILE   "attendance.txt"
#define PAY_FILE   "payroll.txt"

#define DEFAULT_DATA_FOLDER "."

static char g_dataDir[PATH_SIZE];   /* empty means DEFAULT_DATA_FOLDER */

int setDataDirectory(const char *directory)
{
    int status = EMS_OK;

    if ((directory == NULL) || (directory[0] == '\0') || (strlen(directory) >= ((size_t)PATH_SIZE - 32U))) {
        status = ERR_INVALID_INPUT;
    } else {
        (void)snprintf(g_dataDir, sizeof(g_dataDir), "%s", directory);
    }
    return status;
}

const char *getDataDirectory(void)
{
    return (g_dataDir[0] != '\0') ? g_dataDir : DEFAULT_DATA_FOLDER;
}

int buildDataPath(const char *fileName, char *buffer, size_t size)
{
    int status = EMS_OK;
    int written;

    if ((fileName == NULL) || (buffer == NULL) || (size == 0U)) {
        status = ERR_INVALID_INPUT;
    } else {
        written = snprintf(buffer, size, "%s/%s", getDataDirectory(), fileName);
        if ((written < 0) || ((size_t)written >= size)) {
            status = ERR_INVALID_INPUT;
        }
    }
    return status;
}

FILE *beginSave(const char *fileName, char *tmpPath, size_t size)
{
    FILE *fp = NULL;
    char path[PATH_SIZE];

    if ((tmpPath != NULL) && (buildDataPath(fileName, path, sizeof(path)) == EMS_OK)) {
        if ((size_t)snprintf(tmpPath, size, "%s.tmp", path) < size) {
            fp = fopen(tmpPath, "w");
        }
    }
    return fp;
}

int commitSave(FILE *fp, const char *fileName, const char *tmpPath)
{
    int status = EMS_OK;
    char path[PATH_SIZE];
    char msg[LINE_SIZE];

    if ((fp == NULL) || (tmpPath == NULL)) {
        status = ERR_FILE;
    } else {
        if (ferror(fp) != 0) {
            status = ERR_FILE;
        }
        if (fclose(fp) != 0) {
            status = ERR_FILE;
        }
        if ((status == EMS_OK) && (buildDataPath(fileName, path, sizeof(path)) == EMS_OK)) {
            (void)remove(path);                 /* rename() over an existing file fails on Windows */
            if (rename(tmpPath, path) != 0) {
                status = ERR_FILE;
            }
        } else {
            (void)remove(tmpPath);
            status = ERR_FILE;
        }
    }
    if (status != EMS_OK) {
        (void)snprintf(msg, sizeof(msg), "could not save %s, change kept in memory only",
                       (fileName != NULL) ? fileName : "(null)");
        (void)fprintf(getOutputStream(), "%s: %s (%s)\n", errorCodeText(ERR_FILE), errorMessage(ERR_FILE),
                      (fileName != NULL) ? fileName : "(null)");
        (void)logError("FILE", ERR_FILE, msg);
    }
    return status;
}

int backupFile(const char *fileName)
{
    int status = EMS_OK;
    char path[PATH_SIZE];
    char bak[PATH_SIZE + 8];
    char buf[512];
    size_t n;
    FILE *in = NULL;
    FILE *out = NULL;

    status = buildDataPath(fileName, path, sizeof(path));
    if (status == EMS_OK) {
        (void)snprintf(bak, sizeof(bak), "%s.bak", path);
        in = fopen(path, "rb");
        if (in != NULL) {
            out = fopen(bak, "wb");
        }
        if ((in == NULL) || (out == NULL)) {
            status = ERR_FILE;
        } else {
            n = fread(buf, 1U, sizeof(buf), in);
            while (n > 0U) {
                if (fwrite(buf, 1U, n, out) != n) {
                    status = ERR_FILE;
                }
                n = fread(buf, 1U, sizeof(buf), in);
            }
        }
        if (out != NULL) {
            (void)fclose(out);
        }
        if (in != NULL) {
            (void)fclose(in);
        }
    }
    return status;
}

void reportBadLines(const char *fileName, int bad)
{
    char msg[LINE_SIZE];

    if (bad > 0) {
        (void)backupFile(fileName);
        (void)snprintf(msg, sizeof(msg), "%d damaged line(s) skipped in %s (copy kept as %s.bak)", bad,
                       (fileName != NULL) ? fileName : "(null)", (fileName != NULL) ? fileName : "(null)");
        (void)fprintf(getOutputStream(), "Warning: %s\n", msg);
        (void)logMessage(LOG_WARN, "FILE", msg);
    }
}

/* Opens a data file for reading. A missing file is not an error (start empty). */
FILE *openForLoad(const char *fileName, int *status)
{
    FILE *fp = NULL;
    char path[PATH_SIZE];
    char msg[LINE_SIZE];

    *status = buildDataPath(fileName, path, sizeof(path));
    if (*status == EMS_OK) {
        fp = fopen(path, "r");
        if (fp == NULL) {
            (void)snprintf(msg, sizeof(msg), "%s not found, starting with no records", fileName);
            (void)logMessage(LOG_INFO, "FILE", msg);
        }
    }
    return fp;
}

int loadDepartments(void)
{
    int status = EMS_OK;
    int bad = 0;
    int id = 0;
    char name[TITLE_SIZE];
    char created[TIMESTAMP_SIZE];
    char line[LINE_SIZE];
    FILE *fp = openForLoad(DEPT_FILE, &status);

    if (fp != NULL) {
        while (fgets(line, (int)sizeof(line), fp) != NULL) {
            if ((sscanf(line, "%d|%29[^|]|%19[^|\n]", &id, name, created) != 3) ||
                (departmentAdd(id, name, created) != EMS_OK)) {
                bad++;
            }
        }
        (void)fclose(fp);
        reportBadLines(DEPT_FILE, bad);
    }
    return status;
}

int saveDepartments(void)
{
    char tmp[PATH_SIZE];
    int i;
    const Department *d;
    FILE *fp = beginSave(DEPT_FILE, tmp, sizeof(tmp));

    if (fp != NULL) {
        for (i = 0; i < getDeptCount(); i++) {
            d = getDeptAt(i);
            (void)fprintf(fp, "%d|%s|%s\n", d->id, d->name, d->createdAt);
        }
    }
    return commitSave(fp, DEPT_FILE, tmp);
}

int loadRoles(void)
{
    int status = EMS_OK;
    int bad = 0;
    int id = 0;
    char title[TITLE_SIZE];
    char created[TIMESTAMP_SIZE];
    char line[LINE_SIZE];
    FILE *fp = openForLoad(ROLE_FILE, &status);

    if (fp != NULL) {
        while (fgets(line, (int)sizeof(line), fp) != NULL) {
            if ((sscanf(line, "%d|%29[^|]|%19[^|\n]", &id, title, created) != 3) ||
                (roleAdd(id, title, created) != EMS_OK)) {
                bad++;
            }
        }
        (void)fclose(fp);
        reportBadLines(ROLE_FILE, bad);
    }
    return status;
}

int saveRoles(void)
{
    char tmp[PATH_SIZE];
    int i;
    const Role *r;
    FILE *fp = beginSave(ROLE_FILE, tmp, sizeof(tmp));

    if (fp != NULL) {
        for (i = 0; i < getRoleCount(); i++) {
            r = getRoleAt(i);
            (void)fprintf(fp, "%d|%s|%s\n", r->id, r->title, r->createdAt);
        }
    }
    return commitSave(fp, ROLE_FILE, tmp);
}

int loadEmployees(void)
{
    int status = EMS_OK;
    int bad = 0;
    int result;
    Employee emp;
    char line[LINE_SIZE];
    FILE *fp = openForLoad(EMP_FILE, &status);

    if (fp != NULL) {
        while ((fgets(line, (int)sizeof(line), fp) != NULL) && (status == EMS_OK)) {
            (void)memset(&emp, 0, sizeof(emp));
            if (sscanf(line, "%d|%49[^|]|%10[^|]|%49[^|]|%d|%d|%lf|%19[^|]|%19[^|\n]", &emp.id, emp.name,
                       emp.phone, emp.email, &emp.deptId, &emp.roleId, &emp.basicSalary, emp.createdAt,
                       emp.updatedAt) != 9) {
                bad++;
            } else {
                result = employeeInsert(&emp);
                if (result == ERR_MEMORY) {
                    status = ERR_MEMORY;
                } else if (result != EMS_OK) {
                    bad++;
                } else {
                    /* loaded */
                }
            }
        }
        (void)fclose(fp);
        reportBadLines(EMP_FILE, bad);
    }
    return status;
}

int saveEmployees(void)
{
    char tmp[PATH_SIZE];
    const Employee *e;
    FILE *fp = beginSave(EMP_FILE, tmp, sizeof(tmp));

    if (fp != NULL) {
        for (e = getEmployeeHead(); e != NULL; e = e->next) {
            (void)fprintf(fp, "%d|%s|%s|%s|%d|%d|%.2f|%s|%s\n", e->id, e->name, e->phone, e->email, e->deptId,
                          e->roleId, e->basicSalary, e->createdAt, e->updatedAt);
        }
    }
    return commitSave(fp, EMP_FILE, tmp);
}

int loadAttendance(void)
{
    int status = EMS_OK;
    int bad = 0;
    int result;
    int empId = 0;
    Date date = {0, 0, 0};
    char mark = ' ';
    char created[TIMESTAMP_SIZE];
    char line[LINE_SIZE];
    FILE *fp = openForLoad(ATT_FILE, &status);

    if (fp != NULL) {
        while ((fgets(line, (int)sizeof(line), fp) != NULL) && (status == EMS_OK)) {
            if (sscanf(line, "%d|%d|%d|%d|%c|%19[^|\n]", &empId, &date.day, &date.month, &date.year, &mark,
                       created) != 6) {
                bad++;
            } else {
                result = attendanceAdd(empId, &date, mark, created);
                if (result == ERR_MEMORY) {
                    status = ERR_MEMORY;
                } else if (result != EMS_OK) {
                    bad++;
                } else {
                    /* loaded */
                }
            }
        }
        (void)fclose(fp);
        reportBadLines(ATT_FILE, bad);
    }
    return status;
}

int saveAttendance(void)
{
    char tmp[PATH_SIZE];
    int i;
    const Attendance *a;
    FILE *fp = beginSave(ATT_FILE, tmp, sizeof(tmp));

    if (fp != NULL) {
        for (i = 0; i < getAttendanceCount(); i++) {
            a = getAttendanceAt(i);
            (void)fprintf(fp, "%d|%d|%d|%d|%c|%s\n", a->empId, a->date.day, a->date.month, a->date.year, a->status,
                          a->recordedAt);
        }
    }
    return commitSave(fp, ATT_FILE, tmp);
}

int loadPayroll(void)
{
    int status = EMS_OK;
    int bad = 0;
    int result;
    Payroll p;
    char line[LINE_SIZE];
    FILE *fp = openForLoad(PAY_FILE, &status);

    if (fp != NULL) {
        while ((fgets(line, (int)sizeof(line), fp) != NULL) && (status == EMS_OK)) {
            (void)memset(&p, 0, sizeof(p));
            if (sscanf(line, "%d|%d|%d|%lf|%lf|%lf|%d|%lf|%lf|%lf|%19[^|\n]", &p.empId, &p.month, &p.year,
                       &p.salary.basic, &p.salary.allowance, &p.salary.deduction, &p.absentDays, &p.salary.lop,
                       &p.salary.gross, &p.salary.net, p.processedAt) != 11) {
                bad++;
            } else {
                result = payrollAdd(&p);
                if (result == ERR_MEMORY) {
                    status = ERR_MEMORY;
                } else if (result != EMS_OK) {
                    bad++;
                } else {
                    /* loaded */
                }
            }
        }
        (void)fclose(fp);
        reportBadLines(PAY_FILE, bad);
    }
    return status;
}

int savePayroll(void)
{
    char tmp[PATH_SIZE];
    int i;
    const Payroll *p;
    FILE *fp = beginSave(PAY_FILE, tmp, sizeof(tmp));

    if (fp != NULL) {
        for (i = 0; i < getPayrollCount(); i++) {
            p = getPayrollAt(i);
            (void)fprintf(fp, "%d|%d|%d|%.2f|%.2f|%.2f|%d|%.2f|%.2f|%.2f|%s\n", p->empId, p->month, p->year,
                          p->salary.basic, p->salary.allowance, p->salary.deduction, p->absentDays,
                          p->salary.lop, p->salary.gross, p->salary.net, p->processedAt);
        }
    }
    return commitSave(fp, PAY_FILE, tmp);
}

int loadAllRecords(void)
{
    int status;

    status = loadDepartments();
    if (status == EMS_OK) {
        status = loadRoles();
    }
    if (status == EMS_OK) {
        status = loadEmployees();          /* needs departments and roles */
    }
    if (status == EMS_OK) {
        status = loadAttendance();         /* needs employees */
    }
    if (status == EMS_OK) {
        status = loadPayroll();            /* needs employees */
    }
    if (status != EMS_OK) {
        (void)logError("FILE", (EmsStatus)status, "loading stopped");
    } else {
        (void)logMessage(LOG_INFO, "FILE", "All records loaded");
    }
    return status;
}

int saveAllRecords(void)
{
    int status = EMS_OK;

    if (saveDepartments() != EMS_OK) {
        status = ERR_FILE;
    }
    if (saveRoles() != EMS_OK) {
        status = ERR_FILE;
    }
    if (saveEmployees() != EMS_OK) {
        status = ERR_FILE;
    }
    if (saveAttendance() != EMS_OK) {
        status = ERR_FILE;
    }
    if (savePayroll() != EMS_OK) {
        status = ERR_FILE;
    }
    if (status == EMS_OK) {
        (void)logMessage(LOG_INFO, "FILE", "All records saved");
    }
    return status;
}

void freeAllMemory(void)
{
    employeeReset();
    attendanceReset();
    payrollReset();
    deptReset();
    roleReset();
    authReset();
}
