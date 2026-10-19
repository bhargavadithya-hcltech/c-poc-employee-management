#include <stdio.h>
#include <stdlib.h>
 
typedef struct {
    int empId;
    int month;
    int year;
    float basic;
    float allowance;
    float deduction;
    float gross;
    float net;
} Payroll;
 
Payroll records[1000];
int count = 0;
 
void saveData() {
    int i;
    FILE *fp = fopen("payroll.txt", "w");
    if (fp == NULL) {
        printf("Cannot open file.\n");
        return;
    }
    for (i = 0; i < count; i++) {
        fprintf(fp, "%d %d %d %.2f %.2f %.2f %.2f %.2f\n", records[i].empId, records[i].month, records[i].year, records[i].basic, records[i].allowance, records[i].deduction, records[i].gross, records[i].net);
    }
    fclose(fp);
}
 
void loadData() {
    FILE *fp = fopen("payroll.txt", "r");
    if (fp == NULL) return;
    while (count < 1000 && fscanf(fp, "%d %d %d %f %f %f %f %f", &records[count].empId, &records[count].month, &records[count].year, &records[count].basic, &records[count].allowance, &records[count].deduction, &records[count].gross, &records[count].net) == 8) count++;
    fclose(fp);
}
 
void printRecord(Payroll *p) {
    printf("%-6d %-6d %-6d %10.2f %10.2f %10.2f %10.2f %10.2f\n", p->empId, p->month, p->year, p->basic, p->allowance, p->deduction, p->gross, p->net);
}
 
void printHeader() {
    printf("%-6s %-6s %-6s %10s %10s %10s %10s %10s\n", "EmpID", "Month", "Year", "Basic", "Allowance", "Deduction", "Gross", "Net");
}
 
void generatePayroll() {
    Payroll p;
    int i;
    if (count >= 1000) {
        printf("Payroll storage is full.\n");
        return;
    }
    printf("Enter Employee ID: ");
    scanf("%d", &p.empId);
    if (p.empId <= 0) {
        printf("Invalid Employee ID.\n");
        return;
    }
    printf("Enter Month (1-12): ");
    scanf("%d", &p.month);
    printf("Enter Year (2000-2100): ");
    scanf("%d", &p.year);
    if (p.month < 1 || p.month > 12 || p.year < 2000 || p.year > 2100) {
        printf("Invalid month or year.\n");
        return;
    }
    for (i = 0; i < count; i++) {
        if (records[i].empId == p.empId && records[i].month == p.month && records[i].year == p.year) {
            printf("Payroll already generated for this month.\n");
            return;
        }
    }
    printf("Enter Basic Salary: ");
    scanf("%f", &p.basic);
    printf("Enter Allowance: ");
    scanf("%f", &p.allowance);
    printf("Enter Deduction: ");
    scanf("%f", &p.deduction);
    if (p.basic <= 0 || p.allowance < 0 || p.deduction < 0) {
        printf("Invalid amounts.\n");
        return;
    }
    p.gross = p.basic + p.allowance;
    if (p.deduction > p.gross) {
        printf("Deduction cannot be more than gross salary.\n");
        return;
    }
    p.net = p.gross - p.deduction;
    records[count] = p;
    count++;
    saveData();
    printf("Payroll generated. Gross: %.2f  Net: %.2f\n", p.gross, p.net);
}
 
void viewPayrollByEmployee() {
    int empId, i, found = 0;
    printf("Enter Employee ID: ");
    scanf("%d", &empId);
    for (i = 0; i < count; i++) {
        if (records[i].empId == empId) {
            if (found == 0) printHeader();
            printRecord(&records[i]);
            found = 1;
        }
    }
    if (found == 0) printf("No payroll records for this employee.\n");
}
 
void viewMonthlyPayroll() {
    int month, year, i, found = 0;
    printf("Enter Month (1-12): ");
    scanf("%d", &month);
    printf("Enter Year: ");
    scanf("%d", &year);
    for (i = 0; i < count; i++) {
        if (records[i].month == month && records[i].year == year) {
            if (found == 0) printHeader();
            printRecord(&records[i]);
            found = 1;
        }
    }
    if (found == 0) printf("No payroll records for this month.\n");
}
 
int main() {
    int choice;
    loadData();
    do {
        printf("\n1. Generate Payroll\n2. View Payroll by Employee\n3. View Monthly Payroll\n0. Exit\n");
        printf("Enter choice: ");
        scanf("%d", &choice);
        switch (choice) {
            case 1: generatePayroll(); break;
            case 2: viewPayrollByEmployee(); break;
            case 3: viewMonthlyPayroll(); break;
            case 0: break;
            default: printf("Invalid choice.\n");
        }
    } while (choice != 0);
    return 0;
}
 
