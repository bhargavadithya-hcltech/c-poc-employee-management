/**
 * @file role.c
 * @brief Role Module.
 */
#include <stdio.h>
#include <string.h>
#include "role.h"
#include "employee.h"
#include "filemanager.h"
#include "io.h"
#include "logger.h"
#include "validation.h"
#include "auth.h"

static Role g_roles[MAX_ROLES];
static int g_roleCount = 0;

void roleReset(void)
{
    (void)memset(g_roles, 0, sizeof(g_roles));
    g_roleCount = 0;
}

int roleAdd(int id, const char *title, const char *createdAt)
{
    int status = EMS_OK;

    if ((validateId(id) == 0) || (validateText(title) == 0) || (strlen(title) >= (size_t)TITLE_SIZE)) {
        status = ERR_INVALID_INPUT;
    } else if ((findRole(id) >= 0) || (findRoleByTitle(title) >= 0)) {
        status = ERR_DUPLICATE_ID;
    } else if (g_roleCount >= MAX_ROLES) {
        status = ERR_LIMIT_REACHED;
    } else if (setTimestamp(g_roles[g_roleCount].createdAt, createdAt) != EMS_OK) {
        status = ERR_INVALID_INPUT;
    } else {
        g_roles[g_roleCount].id = id;
        (void)snprintf(g_roles[g_roleCount].title, sizeof(g_roles[g_roleCount].title), "%s", title);
        g_roleCount++;
    }
    return status;
}

int findRole(int id)
{
    int index = -1;
    int i;

    for (i = 0; i < g_roleCount; i++) {
        if (g_roles[i].id == id) {
            index = i;
            break;
        }
    }
    return index;
}

int findRoleByTitle(const char *title)
{
    int index = -1;
    int i;

    for (i = 0; i < g_roleCount; i++) {
        if (equalsIgnoreCase(g_roles[i].title, title) == 1) {
            index = i;
            break;
        }
    }
    return index;
}

const char *getRoleTitle(int id)
{
    int index = findRole(id);

    return (index >= 0) ? g_roles[index].title : "(none)";
}

int getRoleCount(void)
{
    return g_roleCount;
}

const Role *getRoleAt(int index)
{
    return ((index >= 0) && (index < g_roleCount)) ? &g_roles[index] : NULL;
}

int viewRoles(void)
{
    FILE *out = getOutputStream();
    int i;

    if (g_roleCount == 0) {
        (void)fprintf(out, "No roles yet.\n");
    } else {
        (void)fprintf(out, "\n%-8s %-30s %-10s %s\n", "ID", "Role", "Employees", "Created");
        for (i = 0; i < g_roleCount; i++) {
            (void)fprintf(out, "%-8d %-30s %-10d %s\n", g_roles[i].id, g_roles[i].title,
                          countEmployeesInRole(g_roles[i].id), g_roles[i].createdAt);
        }
    }
    return g_roleCount;
}

int createRole(void)
{
    int status;
    int id = 0;
    char title[TITLE_SIZE];
    char msg[LINE_SIZE];

    status = readInt("Role ID (1-999999): ", 1, MAX_ID, &id);
    if (status == EMS_OK) {
        status = readString("Role title: ", title, sizeof(title), 0);
    }
    if (status == EMS_OK) {
        status = roleAdd(id, title, NULL);
        if (status == EMS_OK) {
            (void)saveRoles();
            (void)snprintf(msg, sizeof(msg), "Role %d (%s) defined by %s", id, title, getCurrentUser());
            (void)logMessage(LOG_INFO, "ROLE", msg);
            (void)fprintf(getOutputStream(), "Role defined.\n");
        } else if (status == ERR_LIMIT_REACHED) {
            (void)fprintf(getOutputStream(), "Role limit of %d reached.\n", MAX_ROLES);
            (void)logError("ROLE", ERR_INVALID_INPUT, "role limit reached");
        } else {
            showError("ROLE", (EmsStatus)status);
        }
    }
    return status;
}

void roleMenu(void)
{
    int choice = -1;
    int status = EMS_OK;

    while ((choice != 0) && (status == EMS_OK)) {
        (void)fprintf(getOutputStream(), "\n--- Role Management ---\n1. Define Role\n"
                      "2. View Roles\n0. Back\n");
        status = readInt("Choose: ", 0, 2, &choice);
        if (status == EMS_OK) {
            if (choice == 1) {
                if (createRole() == ERR_END_OF_INPUT) {
                    status = ERR_END_OF_INPUT;
                }
            } else if (choice == 2) {
                (void)viewRoles();
            } else {
                choice = 0;
            }
        }
    }
}
