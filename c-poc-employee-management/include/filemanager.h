/**
 * @file filemanager.h
 * @brief File Management Module: text file storage in the data folder.
 *
 * File formats (one record per line):
 *   departments.txt  id|name|createdAt
 *   roles.txt        id|title|createdAt
 *   employees.txt    id|name|phone|email|deptId|roleId|basicSalary|createdAt|updatedAt
 *   attendance.txt   empId|day|month|year|status|recordedAt
 *   payroll.txt      empId|month|year|basic|allowance|deduction|absentDays|lop|gross|net|processedAt
 *   users.txt        username|hash|role|createdAt          (handled by auth.c)
 *
 * Every timestamp has the form "YYYY-MM-DD HH:MM:SS".
 *
 * Every save writes "<file>.tmp" first and renames it over the real file,
 * so a crash never leaves a half-written data file. Damaged lines are
 * skipped on load and the original file is copied to "<file>.bak".
 */
#ifndef EMS_FILEMANAGER_H
#define EMS_FILEMANAGER_H

#include <stdio.h>
#include "common.h"

/** Sets the folder for data files (default: the current folder). */
int setDataDirectory(const char *directory);
const char *getDataDirectory(void);
/** Joins the data folder and fileName into buffer. */
int buildDataPath(const char *fileName, char *buffer, size_t size);
/** Opens "<data>/<fileName>.tmp" for writing. tmpPath receives its path. */
FILE *beginSave(const char *fileName, char *tmpPath, size_t size);
/** Closes the temp file and renames it over the real file. Returns EMS_OK or ERR_FILE. */
int commitSave(FILE *fp, const char *fileName, const char *tmpPath);
/** Copies "<data>/<fileName>" to "<data>/<fileName>.bak". */
int backupFile(const char *fileName);
/** Warns, logs and backs up the file when bad > 0. */
void reportBadLines(const char *fileName, int bad);

/** Opens a data file for reading; NULL (status EMS_OK) when it does not exist yet. */
FILE *openForLoad(const char *fileName, int *status);

int loadDepartments(void);
int saveDepartments(void);
int loadRoles(void);
int saveRoles(void);
int loadEmployees(void);
int saveEmployees(void);
int loadAttendance(void);
int saveAttendance(void);
int loadPayroll(void);
int savePayroll(void);
/** Loads departments, roles, employees, attendance and payroll in that order. */
int loadAllRecords(void);
int saveAllRecords(void);
/** Frees the employee list and the attendance and payroll arrays and clears every table. */
void freeAllMemory(void);

#endif /* EMS_FILEMANAGER_H */
