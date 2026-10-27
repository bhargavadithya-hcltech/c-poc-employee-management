/**
 * @file employee.c
 * @brief Employee Module: singly linked list kept sorted by employee ID.
 */
#include <stdlib.h>
#include <string.h>
#include "employee.h"
#include "attendance.h"
#include "auth.h"
#include "department.h"
#include "filemanager.h"
#include "io.h"
#include "logger.h"
#include "payroll.h"
#include "role.h"
#include "validation.h"

static Employee *g_head = NULL;
static int g_empCount = 0;

void employeeReset(void)
{
    Employee *node = g_head;
    Employee *next;

    while (node != NULL) {
        next = node->next;
        free(node);
        node = next;
    }
    g_head = NULL;
    g_empCount = 0;
}

int validateEmployeeData(const Employee *data)
{
    int status = EMS_OK;

    if (data == NULL) {
        status = ERR_INVALID_INPUT;
    } else if ((validateId(data->id) == 0) || (validateName(data->name) == 0) ||
               (validatePhone(data->phone) == 0) || (validateEmail(data->email) == 0) ||
               (validateSalary(data->basicSalary) == 0)) {
        status = ERR_INVALID_INPUT;
    } else if ((findDept(data->deptId) < 0) || (findRole(data->roleId) < 0)) {
        status = ERR_DEPT_ROLE_NOT_FOUND;
    } else {
        status = EMS_OK;
    }
    return status;
}

int employeeInsert(const Employee *data)
{
    int status = validateEmployeeData(data);
    Employee *node = NULL;
    Employee *cur;
    Employee *prev = NULL;

    if ((status == EMS_OK) && (findEmployee(data->id) != NULL)) {
        status = ERR_DUPLICATE_ID;
    }
    if (status == EMS_OK) {
        node = (Employee *)malloc(sizeof(Employee));
        if (node == NULL) {
            status = ERR_MEMORY;
        } else {
            *node = *data;
            if (setTimestamp(node->createdAt, data->createdAt) != EMS_OK) {
                status = ERR_INVALID_INPUT;
            } else if (setTimestamp(node->updatedAt,
                                    (data->updatedAt[0] != '\0') ? data->updatedAt : node->createdAt) != EMS_OK) {
                status = ERR_INVALID_INPUT;
            } else {
                /* timestamps set */
            }
        }
        if ((status == EMS_OK) && (node != NULL)) {
            cur = g_head;
            while ((cur != NULL) && (cur->id < node->id)) {
                prev = cur;
                cur = cur->next;
            }
            node->next = cur;
            if (prev == NULL) {
                g_head = node;
            } else {
                prev->next = node;
            }
            g_empCount++;
        } else {
            free(node);
        }
    }
    return status;
}

Employee *findEmployee(int id)
{
    Employee *node = g_head;

    while ((node != NULL) && (node->id < id)) {
        node = node->next;
    }
    return ((node != NULL) && (node->id == id)) ? node : NULL;
}

int employeeUpdate(int id, const Employee *data)
{
    int status = EMS_OK;
    Employee *emp = findEmployee(id);
    Employee check;

    if (emp == NULL) {
        status = ERR_NOT_FOUND;
    } else if (data == NULL) {
        status = ERR_INVALID_INPUT;
    } else {
        check = *data;
        check.id = id;
        status = validateEmployeeData(&check);
        if (status == EMS_OK) {
            (void)snprintf(emp->name, sizeof(emp->name), "%s", data->name);
            (void)snprintf(emp->phone, sizeof(emp->phone), "%s", data->phone);
            (void)snprintf(emp->email, sizeof(emp->email), "%s", data->email);
            emp->basicSalary = data->basicSalary;
            emp->deptId = data->deptId;
            emp->roleId = data->roleId;
            (void)setTimestamp(emp->updatedAt, NULL);
        }
    }
    return status;
}

int employeeRemove(int id)
{
    int status = ERR_NOT_FOUND;
    Employee *cur = g_head;
    Employee *prev = NULL;

    while ((cur != NULL) && (cur->id < id)) {
        prev = cur;
        cur = cur->next;
    }
    if ((cur != NULL) && (cur->id == id)) {
        if (prev == NULL) {
            g_head = cur->next;
        } else {
            prev->next = cur->next;
        }
        free(cur);
        g_empCount--;
        (void)attendanceRemoveEmployee(id);
        (void)payrollRemoveEmployee(id);
        status = EMS_OK;
    }
    return status;
}

int employeeSetDepartment(int id, int deptId)
{
    int status = EMS_OK;
    Employee *emp = findEmployee(id);

    if (emp == NULL) {
        status = ERR_NOT_FOUND;
    } else if (findDept(deptId) < 0) {
        status = ERR_DEPT_ROLE_NOT_FOUND;
    } else {
        emp->deptId = deptId;
        (void)setTimestamp(emp->updatedAt, NULL);
    }
    return status;
}

int employeeSetRole(int id, int roleId)
{
    int status = EMS_OK;
    Employee *emp = findEmployee(id);

    if (emp == NULL) {
        status = ERR_NOT_FOUND;
    } else if (findRole(roleId) < 0) {
        status = ERR_DEPT_ROLE_NOT_FOUND;
    } else {
        emp->roleId = roleId;
        (void)setTimestamp(emp->updatedAt, NULL);
    }
    return status;
}

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
        for (node = g_head; node != NULL; node = node->next) {
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

int countEmployeesInDept(int deptId)
{
    int count = 0;
    const Employee *node;

    for (node = g_head; node != NULL; node = node->next) {
        if (node->deptId == deptId) {
            count++;
        }
    }
    return count;
}

int countEmployeesInRole(int roleId)
{
    int count = 0;
    const Employee *node;

    for (node = g_head; node != NULL; node = node->next) {
        if (node->roleId == roleId) {
            count++;
        }
    }
    return count;
}

const Employee *getEmployeeHead(void)
{
    return g_head;
}

int getEmployeeCount(void)
{
    return g_empCount;
}

void printEmployeeHeader(FILE *out)
{
    if (out != NULL) {
        (void)fprintf(out, "\n%-7s %-20s %-11s %-24s %-14s %-14s %12s  %-19s\n", "ID", "Name", "Phone",
                      "Email", "Department", "Role", "Basic Salary", "Created");
    }
}

void printEmployee(FILE *out, const Employee *emp)
{
    if ((out != NULL) && (emp != NULL)) {
        (void)fprintf(out, "%-7d %-20.20s %-11s %-24.24s %-14.14s %-14.14s %12.2f  %-19s\n", emp->id,
                      emp->name, emp->phone, emp->email, getDeptName(emp->deptId),
                      getRoleTitle(emp->roleId), emp->basicSalary, emp->createdAt);
    }
}

int selectDepartment(int *deptId)
{
    int status = ERR_INVALID_INPUT;

    if (deptId == NULL) {
        status = ERR_INVALID_INPUT;
    } else if (getDeptCount() == 0) {
        (void)fprintf(getOutputStream(), "No departments exist. The Administrator must create one first.\n");
        status = ERR_DEPT_ROLE_NOT_FOUND;
    } else {
        (void)viewDepartments();
        while (status == ERR_INVALID_INPUT) {
            status = readInt("Department ID: ", 1, MAX_ID, deptId);
            if ((status == EMS_OK) && (findDept(*deptId) < 0)) {
                showError("EMPLOYEE", ERR_DEPT_ROLE_NOT_FOUND);
                status = ERR_INVALID_INPUT;
            }
        }
    }
    return status;
}

int selectRole(int *roleId)
{
    int status = ERR_INVALID_INPUT;

    if (roleId == NULL) {
        status = ERR_INVALID_INPUT;
    } else if (getRoleCount() == 0) {
        (void)fprintf(getOutputStream(), "No roles exist. The Administrator must define one first.\n");
        status = ERR_DEPT_ROLE_NOT_FOUND;
    } else {
        (void)viewRoles();
        while (status == ERR_INVALID_INPUT) {
            status = readInt("Role ID: ", 1, MAX_ID, roleId);
            if ((status == EMS_OK) && (findRole(*roleId) < 0)) {
                showError("EMPLOYEE", ERR_DEPT_ROLE_NOT_FOUND);
                status = ERR_INVALID_INPUT;
            }
        }
    }
    return status;
}

/* ------------------------------------------------- interactive functions */

void logEmployeeEvent(const char *action, int id)
{
    char msg[LINE_SIZE];

    (void)snprintf(msg, sizeof(msg), "%s employee %d by %s", action, id, getCurrentUser());
    (void)logMessage(LOG_INFO, "EMPLOYEE", msg);
}

int addEmployee(void)
{
    int status;
    Employee emp;

    (void)memset(&emp, 0, sizeof(emp));
    status = readInt("Employee ID (1-999999): ", 1, MAX_ID, &emp.id);
    if ((status == EMS_OK) && (findEmployee(emp.id) != NULL)) {
        status = ERR_DUPLICATE_ID;
        showError("EMPLOYEE", ERR_DUPLICATE_ID);
    }
    if (status == EMS_OK) {
        status = readValidated("Name: ", emp.name, sizeof(emp.name), validateName,
                               "Use letters, spaces, '.' or ' only.");
    }
    if (status == EMS_OK) {
        status = readValidated("Phone (10 digits): ", emp.phone, sizeof(emp.phone), validatePhone,
                               "Phone must be exactly 10 digits.");
    }
    if (status == EMS_OK) {
        status = readValidated("Email: ", emp.email, sizeof(emp.email), validateEmail,
                               "Enter a valid email such as name@company.com.");
    }
    if (status == EMS_OK) {
        status = selectDepartment(&emp.deptId);
    }
    if (status == EMS_OK) {
        status = selectRole(&emp.roleId);
    }
    if (status == EMS_OK) {
        status = readFloat("Basic monthly salary: ", 0.01, MAX_MONEY, &emp.basicSalary);
    }
    if (status == EMS_OK) {
        status = employeeInsert(&emp);
        if (status == EMS_OK) {
            (void)saveEmployees();
            logEmployeeEvent("Added", emp.id);
            (void)fprintf(getOutputStream(), "Employee %d added at %s.\n", emp.id, findEmployee(emp.id)->createdAt);
        } else {
            showError("EMPLOYEE", (EmsStatus)status);
        }
    }
    return status;
}

int viewEmployee(void)
{
    const Employee *node;
    FILE *out = getOutputStream();

    if (g_head == NULL) {
        (void)fprintf(out, "No employees yet.\n");
    } else {
        printEmployeeHeader(out);
        for (node = g_head; node != NULL; node = node->next) {
            printEmployee(out, node);
        }
        (void)fprintf(out, "Total employees: %d\n", g_empCount);
    }
    return g_empCount;
}

int searchEmployee(void)
{
    int status;
    int type = 0;
    int id = 0;
    char key[NAME_SIZE];
    int found;

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

int updateEmployee(void)
{
    int status;
    int id = 0;
    int choice = 0;
    const Employee *emp = NULL;
    Employee copy;

    status = readInt("Employee ID to update: ", 1, MAX_ID, &id);
    if (status == EMS_OK) {
        emp = findEmployee(id);
        if (emp == NULL) {
            status = ERR_NOT_FOUND;
            showError("EMPLOYEE", ERR_NOT_FOUND);
        }
    }
    if (status == EMS_OK) {
        copy = *emp;
        printEmployeeHeader(getOutputStream());
        printEmployee(getOutputStream(), emp);
        (void)fprintf(getOutputStream(), "1. Name\n2. Phone\n3. Email\n4. Basic Salary\n0. Cancel\n");
        status = readInt("What to change: ", 0, 4, &choice);
    }
    if ((status == EMS_OK) && (choice != 0)) {
        if (choice == 1) {
            status = readValidated("New name: ", copy.name, sizeof(copy.name), validateName,
                                   "Use letters, spaces, '.' or ' only.");
        } else if (choice == 2) {
            status = readValidated("New phone: ", copy.phone, sizeof(copy.phone), validatePhone,
                                   "Phone must be exactly 10 digits.");
        } else if (choice == 3) {
            status = readValidated("New email: ", copy.email, sizeof(copy.email), validateEmail,
                                   "Enter a valid email such as name@company.com.");
        } else {
            status = readFloat("New basic salary: ", 0.01, MAX_MONEY, &copy.basicSalary);
        }
        if (status == EMS_OK) {
            status = employeeUpdate(id, &copy);
        }
        if (status == EMS_OK) {
            (void)saveEmployees();
            logEmployeeEvent("Updated", id);
            (void)fprintf(getOutputStream(), "Employee %d updated.\n", id);
        } else if (status != ERR_END_OF_INPUT) {
            showError("EMPLOYEE", (EmsStatus)status);
        } else {
            /* input closed: nothing to report */
        }
    }
    return status;
}

int deleteEmployee(void)
{
    int status;
    int id = 0;
    int yes = 0;
    const Employee *emp = NULL;

    status = readInt("Employee ID to delete: ", 1, MAX_ID, &id);
    if (status == EMS_OK) {
        emp = findEmployee(id);
        if (emp == NULL) {
            status = ERR_NOT_FOUND;
            showError("EMPLOYEE", ERR_NOT_FOUND);
        }
    }
    if (status == EMS_OK) {
        printEmployeeHeader(getOutputStream());
        printEmployee(getOutputStream(), emp);
        status = readYesNo("Delete this employee with all attendance and payroll? (y/n): ", &yes);
    }
    if ((status == EMS_OK) && (yes == 1)) {
        status = employeeRemove(id);
        if (status == EMS_OK) {
            (void)saveEmployees();
            (void)saveAttendance();
            (void)savePayroll();
            logEmployeeEvent("Deleted", id);
            (void)fprintf(getOutputStream(), "Employee %d deleted.\n", id);
        }
    } else if (status == EMS_OK) {
        (void)fprintf(getOutputStream(), "Delete cancelled.\n");
    } else {
        /* error already reported */
    }
    return status;
}

int assignDepartment(void)
{
    int status;
    int id = 0;
    int deptId = 0;

    status = readInt("Employee ID: ", 1, MAX_ID, &id);
    if ((status == EMS_OK) && (findEmployee(id) == NULL)) {
        status = ERR_NOT_FOUND;
        showError("EMPLOYEE", ERR_NOT_FOUND);
    }
    if (status == EMS_OK) {
        status = selectDepartment(&deptId);
    }
    if (status == EMS_OK) {
        status = employeeSetDepartment(id, deptId);
        if (status == EMS_OK) {
            (void)saveEmployees();
            logEmployeeEvent("Department changed for", id);
            (void)fprintf(getOutputStream(), "Department of employee %d set to %s.\n", id, getDeptName(deptId));
        }
    }
    return status;
}

int assignRole(void)
{
    int status;
    int id = 0;
    int roleId = 0;

    status = readInt("Employee ID: ", 1, MAX_ID, &id);
    if ((status == EMS_OK) && (findEmployee(id) == NULL)) {
        status = ERR_NOT_FOUND;
        showError("EMPLOYEE", ERR_NOT_FOUND);
    }
    if (status == EMS_OK) {
        status = selectRole(&roleId);
    }
    if (status == EMS_OK) {
        status = employeeSetRole(id, roleId);
        if (status == EMS_OK) {
            (void)saveEmployees();
            logEmployeeEvent("Role changed for", id);
            (void)fprintf(getOutputStream(), "Role of employee %d set to %s.\n", id, getRoleTitle(roleId));
        }
    }
    return status;
}

void employeeMenu(void)
{
    int choice = -1;
    int status = EMS_OK;
    int result;

    while ((choice != 0) && (status == EMS_OK)) {
        (void)fprintf(getOutputStream(), "\n--- Employee Management ---\n1. Add Employee\n2. View Employees\n"
                      "3. Search Employee\n4. Update Employee\n5. Delete Employee (Admin)\n"
                      "6. Assign Department (Admin)\n7. Assign Role (Admin)\n0. Back\n");
        status = readInt("Choose: ", 0, 7, &choice);
        if (status == EMS_OK) {
            result = EMS_OK;
            switch (choice) {
            case 1: result = addEmployee(); break;
            case 2: (void)viewEmployee(); break;
            case 3: result = searchEmployee(); break;
            case 4: result = updateEmployee(); break;
            case 5: if (checkAccess(ROLE_ADMIN) == 1) { result = deleteEmployee(); } break;
            case 6: if (checkAccess(ROLE_ADMIN) == 1) { result = assignDepartment(); } break;
            case 7: if (checkAccess(ROLE_ADMIN) == 1) { result = assignRole(); } break;
            default: choice = 0; break;
            }
            if (result == ERR_END_OF_INPUT) {
                status = ERR_END_OF_INPUT;
            }
        }
    }
}
