#include <stdio.h>
#include <stdlib.h>
#include <string.h>
 
typedef struct {
    int id;
    char title[30];
} Role;
 
typedef struct {
    int empId;
    int roleId;
} Assignment;
 
Role roles[50];
Assignment assigns[200];
int roleCount = 0;
int assignCount = 0;
 
void saveData() {
    int i;
    FILE *fp = fopen("roles.txt", "w");
    if (fp != NULL) {
        for (i = 0; i < roleCount; i++) fprintf(fp, "%d|%s\n", roles[i].id, roles[i].title);
        fclose(fp);
    }
    fp = fopen("role_assign.txt", "w");
    if (fp != NULL) {
        for (i = 0; i < assignCount; i++) fprintf(fp, "%d %d\n", assigns[i].empId, assigns[i].roleId);
        fclose(fp);
    }
}
 
void loadData() {
    FILE *fp = fopen("roles.txt", "r");
    if (fp != NULL) {
        while (roleCount < 50 && fscanf(fp, "%d|%29[^\n]\n", &roles[roleCount].id, roles[roleCount].title) == 2) roleCount++;
        fclose(fp);
    }
    fp = fopen("role_assign.txt", "r");
    if (fp != NULL) {
        while (assignCount < 200 && fscanf(fp, "%d %d", &assigns[assignCount].empId, &assigns[assignCount].roleId) == 2) assignCount++;
        fclose(fp);
    }
}
 
int findRole(int id) {
    int i;
    for (i = 0; i < roleCount; i++) {
        if (roles[i].id == id) return i;
    }
    return -1;
}
 
void defineRole() {
    int id;
    if (roleCount >= 50) {
        printf("Role limit reached.\n");
        return;
    }
    printf("Enter Role ID: ");
    scanf("%d", &id);
    if (id <= 0) {
        printf("ID must be positive.\n");
        return;
    }
    if (findRole(id) != -1) {
        printf("Role ID already exists.\n");
        return;
    }
    roles[roleCount].id = id;
    printf("Enter Role Title: ");
    scanf(" %29[^\n]", roles[roleCount].title);
    roleCount++;
    saveData();
    printf("Role defined.\n");
}
 
void viewRoles() {
    int i;
    if (roleCount == 0) {
        printf("No roles.\n");
        return;
    }
    for (i = 0; i < roleCount; i++) printf("%d | %s\n", roles[i].id, roles[i].title);
}
 
void assignRole() {
    int empId, roleId, i;
    printf("Enter Employee ID: ");
    scanf("%d", &empId);
    if (empId <= 0) {
        printf("Invalid Employee ID.\n");
        return;
    }
    printf("Enter Role ID: ");
    scanf("%d", &roleId);
    if (findRole(roleId) == -1) {
        printf("Role does not exist.\n");
        return;
    }
    for (i = 0; i < assignCount; i++) {
        if (assigns[i].empId == empId) {
            assigns[i].roleId = roleId;
            saveData();
            printf("Employee role modified.\n");
            return;
        }
    }
    if (assignCount >= 200) {
        printf("Assignment limit reached.\n");
        return;
    }
    assigns[assignCount].empId = empId;
    assigns[assignCount].roleId = roleId;
    assignCount++;
    saveData();
    printf("Role assigned.\n");
}
 
void viewEmployeeRole() {
    int empId, i, r;
    printf("Enter Employee ID: ");
    scanf("%d", &empId);
    for (i = 0; i < assignCount; i++) {
        if (assigns[i].empId == empId) {
            r = findRole(assigns[i].roleId);
            if (r != -1) printf("Employee %d has role: %s\n", empId, roles[r].title);
            return;
        }
    }
    printf("No role assigned to this employee.\n");
}
 
int main() {
    int choice;
    loadData();
    do {
        printf("\n1. Define Role\n2. View Roles\n3. Assign or Modify Role\n4. View Employee Role\n0. Exit\n");
        printf("Enter choice: ");
        scanf("%d", &choice);
        switch (choice) {
            case 1: defineRole(); break;
            case 2: viewRoles(); break;
            case 3: assignRole(); break;
            case 4: viewEmployeeRole(); break;
            case 0: break;
            default: printf("Invalid choice.\n");
        }
    } while (choice != 0);
    return 0;
}
