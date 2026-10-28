/**
 * @file department.c
 * @brief Department Module.
 */
#include <stdio.h>
#include <string.h>
#include "department.h"
#include "employee.h"
#include "filemanager.h"
#include "io.h"
#include "logger.h"
#include "validation.h"
#include "auth.h"

static Department g_depts[MAX_DEPARTMENTS];
static int g_deptCount = 0;

void deptReset(void)
{
    (void)memset(g_depts, 0, sizeof(g_depts));
    g_deptCount = 0;
}

int departmentAdd(int id, const char *name, const char *createdAt)
{
    int status = EMS_OK;

    if ((validateId(id) == 0) || (validateText(name) == 0) || (strlen(name) >= (size_t)TITLE_SIZE)) {
        status = ERR_INVALID_INPUT;
    } else if ((findDept(id) >= 0) || (findDeptByName(name) >= 0)) {
        status = ERR_DUPLICATE_ID;
    } else if (g_deptCount >= MAX_DEPARTMENTS) {
        status = ERR_LIMIT_REACHED;
    } else if (setTimestamp(g_depts[g_deptCount].createdAt, createdAt) != EMS_OK) {
        status = ERR_INVALID_INPUT;
    } else {
        g_depts[g_deptCount].id = id;
        (void)snprintf(g_depts[g_deptCount].name, sizeof(g_depts[g_deptCount].name), "%s", name);
        g_deptCount++;
    }
    return status;
}

int findDept(int id)
{
    int index = -1;
    int i;

    for (i = 0; i < g_deptCount; i++) {
        if (g_depts[i].id == id) {
            index = i;
            break;
        }
    }
    return index;
}

int findDeptByName(const char *name)
{
    int index = -1;
    int i;

    for (i = 0; i < g_deptCount; i++) {
        if (equalsIgnoreCase(g_depts[i].name, name) == 1) {
            index = i;
            break;
        }
    }
    return index;
}

const char *getDeptName(int id)
{
    int index = findDept(id);

    return (index >= 0) ? g_depts[index].name : "(none)";
}

int getDeptCount(void)
{
    return g_deptCount;
}

const Department *getDeptAt(int index)
{
    return ((index >= 0) && (index < g_deptCount)) ? &g_depts[index] : NULL;
}

int viewDepartments(void)
{
    FILE *out = getOutputStream();

    if (g_deptCount == 0) {
        (void)fprintf(out, "No departments yet.\n");
    } else {
        int i;
        (void)fprintf(out, "\n%-8s %-30s %-10s %s\n", "ID", "Department", "Employees", "Created");
        for (i = 0; i < g_deptCount; i++) {
            (void)fprintf(out, "%-8d %-30s %-10d %s\n", g_depts[i].id, g_depts[i].name,
                          countEmployeesInDept(g_depts[i].id), g_depts[i].createdAt);
        }
    }
    return g_deptCount;
}

int createDepartment(void)
{
    int status;
    int id = 0;
    char name[TITLE_SIZE];

    status = readInt("Department ID (1-999999): ", 1, MAX_ID, &id);
    if (status == EMS_OK) {
        status = readString("Department name: ", name, sizeof(name), 0);
    }
    if (status == EMS_OK) {
        status = departmentAdd(id, name, NULL);
        if (status == EMS_OK) {
            char msg[LINE_SIZE];
            (void)saveDepartments();
            (void)snprintf(msg, sizeof(msg), "Department %d (%s) created by %s", id, name, getCurrentUser());
            (void)logMessage(LOG_INFO, "DEPARTMENT", msg);
            (void)fprintf(getOutputStream(), "Department created.\n");
        } else if (status == ERR_LIMIT_REACHED) {
            (void)fprintf(getOutputStream(), "Department limit of %d reached.\n", MAX_DEPARTMENTS);
            (void)logError("DEPARTMENT", ERR_INVALID_INPUT, "department limit reached");
        } else {
            showError("DEPARTMENT", (EmsStatus)status);
        }
    }
    return status;
}

void departmentMenu(void)
{
    int choice = -1;
    int status = EMS_OK;

    while ((choice != 0) && (status == EMS_OK)) {
        (void)fprintf(getOutputStream(), "\n--- Department Management ---\n1. Create Department\n"
                      "2. View Departments\n0. Back\n");
        status = readInt("Choose: ", 0, 2, &choice);
        if (status == EMS_OK) {
            if (choice == 1) {
                if (createDepartment() == ERR_END_OF_INPUT) {
                    status = ERR_END_OF_INPUT;
                }
            } else if (choice == 2) {
                (void)viewDepartments();
            } else {
                choice = 0;
            }
        }
    }
}
