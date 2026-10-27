/**
 * @file searching.c
 * @brief Searching Module.
 */
#include <string.h>
#include "searching.h"
#include "employee.h"
#include "io.h"
#include "logger.h"
#include "validation.h"

int employeeMatches(const Employee *emp, int type, int id, const char *key)
{
    int match = 0;

    if (emp != NULL) {
        if (type == SEARCH_BY_ID) {
            if (emp->id == id) {
                match = 1;
            } else {
                match = 0;
            }
        } else if (type == SEARCH_BY_NAME) {
            match = containsIgnoreCase(emp->name, key);
        } else if (type == SEARCH_BY_DEPT) {
            if (emp->deptId == id) {
                match = 1;
            } else {
                match = 0;
            }
        } else if (type == SEARCH_BY_ROLE) {
            if (emp->roleId == id) {
                match = 1;
            } else {
                match = 0;
            }
        } else {
            match = 0;
        }
    }
    return match;
}

int employeeSearch(int type, int id, const char *key, FILE *out)
{
    int found = 0;
    const Employee *node;

    if (type == SEARCH_BY_ID) {
        node = findEmployee(id);        /* sorted list: no full traversal */
        if (node != NULL) {
            if (out != NULL) {
                printEmployeeHeader(out);
                printEmployee(out, node);
            }
            found = 1;
        }
    } else {
        for (node = getEmployeeHead(); node != NULL; node = node->next) {
            if (employeeMatches(node, type, id, key) == 1) {
                if (out != NULL) {
                    if (found == 0) {
                        printEmployeeHeader(out);
                    }
                    printEmployee(out, node);
                }
                found++;
            }
        }
    }
    return found;
}

int searchEmployee(void)
{
    int status;
    int type = 0;
    int id = 0;
    char key[NAME_SIZE];

    key[0] = '\0';
    (void)fprintf(getOutputStream(), "\nSearch by: 1. ID  2. Name  3. Department  4. Role\n");
    status = readInt("Choose: ", SEARCH_BY_ID, SEARCH_BY_ROLE, &type);
    if (status == EMS_OK) {
        if (type == SEARCH_BY_ID) {
            status = readInt("Employee ID: ", 1, MAX_ID, &id);
        } else if (type == SEARCH_BY_NAME) {
            status = readString("Name (or part of it): ", key, sizeof(key), 0);
        } else if (type == SEARCH_BY_DEPT) {
            status = selectDepartment(&id);
        } else {
            status = selectRole(&id);
        }
    }
    if (status == EMS_OK) {
        int found;
        found = employeeSearch(type, id, key, getOutputStream());
        if (found == 0) {
            status = ERR_NOT_FOUND;
            showError("EMPLOYEE", ERR_NOT_FOUND);
        } else {
            (void)fprintf(getOutputStream(), "%d match(es).\n", found);
        }
    }
    return status;
}
