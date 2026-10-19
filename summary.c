#include <stdio.h>
#include <stdlib.h>
#include <string.h>
 
typedef struct {
    int id;
    char name[50];
    char phone[15];
    char department[30];
    char role[30];
    float salary;
} Employee;
 
Employee emps[100];
int empCount = 0;
 
void loadEmployees() {
    FILE *fp = fopen("employees.txt", "r");
    if (fp == NULL) {
        printf("employees.txt not found.\n");
        return;
    }
    while (empCount < 100 && fscanf(fp, "%d|%49[^|]|%14[^|]|%29[^|]|%29[^|]|%f\n", &emps[empCount].id, emps[empCount].name, emps[empCount].phone, emps[empCount].department, emps[empCount].role, &emps[empCount].salary) == 6) empCount++;
    fclose(fp);
}
 
void printEmployee(Employee *e) {
    printf("%-6d %-20s %-12s %-12s %-12s %10.2f\n", e->id, e->name, e->phone, e->department, e->role, e->salary);
}
 
void searchEmployees() {
    int choice, id = 0, i, match, found = 0;
    char key[50];
    printf("\nSearch by: 1. ID  2. Name  3. Department  4. Role\n");
    printf("Enter choice: ");
    scanf("%d", &choice);
    if (choice < 1 || choice > 4) {
        printf("Invalid choice.\n");
        return;
    }
    if (choice == 1) {
        printf("Enter Employee ID: ");
        scanf("%d", &id);
    } else {
        printf("Enter search text: ");
        scanf(" %49[^\n]", key);
    }
    for (i = 0; i < empCount; i++) {
        match = 0;
        if (choice == 1 && emps[i].id == id) match = 1;
        if (choice == 2 && strstr(emps[i].name, key) != NULL) match = 1;
        if (choice == 3 && strstr(emps[i].department, key) != NULL) match = 1;
        if (choice == 4 && strstr(emps[i].role, key) != NULL) match = 1;
        if (match == 1) {
            printEmployee(&emps[i]);
            found = 1;
        }
    }
    if (found == 0) printf("No matching employees found.\n");
}
 
void employeeSummary() {
    int i;
    float total = 0;
    if (empCount == 0) {
        printf("No employee records.\n");
        return;
    }
    for (i = 0; i < empCount; i++) total += emps[i].salary;
    printf("Total employees: %d\n", empCount);
    printf("Total salary: %.2f\n", total);
    printf("Average salary: %.2f\n", total / empCount);
}
 
void attendanceSummary() {
    FILE *fp = fopen("attendance.txt", "r");
    int id, d, m, y, p = 0, a = 0, l = 0;
    char status;
    if (fp == NULL) {
        printf("attendance.txt not found.\n");
        return;
    }
    while (fscanf(fp, "%d %d %d %d %c", &id, &d, &m, &y, &status) == 5) {
        if (status == 'P') p++;
        else if (status == 'A') a++;
        else if (status == 'L') l++;
    }
    fclose(fp);
    printf("Total records: %d\nPresent: %d\nAbsent: %d\nLeave: %d\n", p + a + l, p, a, l);
}
 
void payrollSummary() {
    FILE *fp = fopen("payroll.txt", "r");
    int id, month, year, n = 0;
    float basic, allowance, deduction, gross, net, totalGross = 0, totalNet = 0;
    if (fp == NULL) {
        printf("payroll.txt not found.\n");
        return;
    }
    while (fscanf(fp, "%d %d %d %f %f %f %f %f", &id, &month, &year, &basic, &allowance, &deduction, &gross, &net) == 8) {
        totalGross += gross;
        totalNet += net;
        n++;
    }
    fclose(fp);
    printf("Payroll records: %d\nTotal gross: %.2f\nTotal net: %.2f\n", n, totalGross, totalNet);
}
 
int main() {
    int choice;
    loadEmployees();
    do {
        printf("\n1. Search Employees\n2. Employee Summary\n3. Attendance Summary\n4. Payroll Summary\n0. Exit\n");
        printf("Enter choice: ");
        scanf("%d", &choice);
        switch (choice) {
            case 1: searchEmployees(); break;
            case 2: employeeSummary(); break;
            case 3: attendanceSummary(); break;
            case 4: payrollSummary(); break;
            case 0: break;
            default: printf("Invalid choice.\n");
        }
    } while (choice != 0);
    return 0;
}
 
