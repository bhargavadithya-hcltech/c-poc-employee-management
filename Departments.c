#include <stdio.h>
#include <stdlib.h>
#include <string.h>
 
typedef struct {
    int id;
    char name[30];
} Department;
 
typedef struct {
    int empId;
    int deptId;
} Assignment;
 
Department depts[50];
Assignment assigns[200];
int deptCount = 0;
int assignCount = 0;
 
void saveData() {
    int i;
    FILE *fp = fopen("departments.txt", "w");
    if (fp != NULL) {
        for (i = 0; i < deptCount; i++) fprintf(fp, "%d|%s\n", depts[i].id, depts[i].name);
        fclose(fp);
    }
    fp = fopen("dept_assign.txt", "w");
    if (fp != NULL) {
        for (i = 0; i < assignCount; i++) fprintf(fp, "%d %d\n", assigns[i].empId, assigns[i].deptId);
        fclose(fp);
    }
}
 
void loadData() {
    FILE *fp = fopen("departments.txt", "r");
    if (fp != NULL) {
        while (deptCount < 50 && fscanf(fp, "%d|%29[^\n]\n", &depts[deptCount].id, depts[deptCount].name) == 2) deptCount++;
        fclose(fp);
    }
    fp = fopen("dept_assign.txt", "r");
    if (fp != NULL) {
        while (assignCount < 200 && fscanf(fp, "%d %d", &assigns[assignCount].empId, &assigns[assignCount].deptId) == 2) assignCount++;
        fclose(fp);
    }
}
 
int findDept(int id) {
    int i;
    for (i = 0; i < deptCount; i++) {
        if (depts[i].id == id) return i;
    }
    return -1;
}
 
void createDepartment() {
    int id;
    if (deptCount >= 50) {
        printf("Department limit reached.\n");
        return;
    }
    printf("Enter Department ID: ");
    scanf("%d", &id);
    if (id <= 0) {
        printf("ID must be positive.\n");
        return;
    }
    if (findDept(id) != -1) {
        printf("Department ID already exists.\n");
        return;
    }
    depts[deptCount].id = id;
    printf("Enter Department Name: ");
    scanf(" %29[^\n]", depts[deptCount].name);
    deptCount++;
    saveData();
    printf("Department created.\n");
}
 
void viewDepartments() {
    int i;
    if (deptCount == 0) {
        printf("No departments.\n");
        return;
    }
    for (i = 0; i < deptCount; i++) printf("%d | %s\n", depts[i].id, depts[i].name);
}
 
void assignEmployee() {
    int empId, deptId, i;
    printf("Enter Employee ID: ");
    scanf("%d", &empId);
    if (empId <= 0) {
        printf("Invalid Employee ID.\n");
        return;
    }
    printf("Enter Department ID: ");
    scanf("%d", &deptId);
    if (findDept(deptId) == -1) {
        printf("Department does not exist.\n");
        return;
    }
    for (i = 0; i < assignCount; i++) {
        if (assigns[i].empId == empId) {
            assigns[i].deptId = deptId;
            saveData();
            printf("Employee department updated.\n");
            return;
        }
    }
    if (assignCount >= 200) {
        printf("Assignment limit reached.\n");
        return;
    }
    assigns[assignCount].empId = empId;
    assigns[assignCount].deptId = deptId;
    assignCount++;
    saveData();
    printf("Employee assigned.\n");
}
 
void searchByDepartment() {
    int deptId, i, d, found = 0;
    printf("Enter Department ID: ");
    scanf("%d", &deptId);
    d = findDept(deptId);
    if (d == -1) {
        printf("Department does not exist.\n");
        return;
    }
    printf("Employees in %s:\n", depts[d].name);
    for (i = 0; i < assignCount; i++) {
        if (assigns[i].deptId == deptId) {
            printf("Employee ID: %d\n", assigns[i].empId);
            found = 1;
        }
    }
    if (found == 0) printf("No employees in this department.\n");
}
 
int main() {
    int choice;
    loadData();
    do {
        printf("\n1. Create Department\n2. View Departments\n3. Assign Employee\n4. Search by Department\n0. Exit\n");
        printf("Enter choice: ");
        scanf("%d", &choice);
        switch (choice) {
            case 1: createDepartment(); break;
            case 2: viewDepartments(); break;
            case 3: assignEmployee(); break;
            case 4: searchByDepartment(); break;
            case 0: break;
            default: printf("Invalid choice.\n");
        }
    } while (choice != 0);
    return 0;
}
