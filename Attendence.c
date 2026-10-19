#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
 
typedef struct {
    int empId;
    int day;
    int month;
    int year;
    char status;
} Attendance;
 
Attendance records[1000];
int count = 0;
 
void saveData() {
    int i;
    FILE *fp = fopen("attendance.txt", "w");
    if (fp == NULL) {
        printf("Cannot open file.\n");
        return;
    }
    for (i = 0; i < count; i++) {
        fprintf(fp, "%d %02d %02d %d %c\n", records[i].empId, records[i].day, records[i].month, records[i].year, records[i].status);
    }
    fclose(fp);
}
 
void loadData() {
    FILE *fp = fopen("attendance.txt", "r");
    if (fp == NULL) return;
    while (count < 1000 && fscanf(fp, "%d %d %d %d %c", &records[count].empId, &records[count].day, &records[count].month, &records[count].year, &records[count].status) == 5) count++;
    fclose(fp);
}
 
void recordAttendance() {
    int empId, day, month, year, i;
    char status;
    if (count >= 1000) {
        printf("Attendance storage is full.\n");
        return;
    }
    printf("Enter Employee ID: ");
    scanf("%d", &empId);
    if (empId <= 0) {
        printf("Invalid Employee ID.\n");
        return;
    }
    printf("Enter Day Month Year (example 5 10 2026): ");
    scanf("%d %d %d", &day, &month, &year);
    if (day < 1 || day > 31 || month < 1 || month > 12 || year < 2000 || year > 2100) {
        printf("Invalid date.\n");
        return;
    }
    for (i = 0; i < count; i++) {
        if (records[i].empId == empId && records[i].day == day && records[i].month == month && records[i].year == year) {
            printf("Attendance already recorded for this date.\n");
            return;
        }
    }
    printf("Enter Status (P = Present, A = Absent, L = Leave): ");
    scanf(" %c", &status);
    status = toupper(status);
    if (status != 'P' && status != 'A' && status != 'L') {
        printf("Invalid status.\n");
        return;
    }
    records[count].empId = empId;
    records[count].day = day;
    records[count].month = month;
    records[count].year = year;
    records[count].status = status;
    count++;
    saveData();
    printf("Attendance recorded.\n");
}
 
void viewAttendance() {
    int empId, i, found = 0;
    printf("Enter Employee ID: ");
    scanf("%d", &empId);
    for (i = 0; i < count; i++) {
        if (records[i].empId == empId) {
            printf("%02d-%02d-%d  %c\n", records[i].day, records[i].month, records[i].year, records[i].status);
            found = 1;
        }
    }
    if (found == 0) printf("No attendance records for this employee.\n");
}
 
void attendanceSummary() {
    int empId, i, p = 0, a = 0, l = 0, total;
    printf("Enter Employee ID: ");
    scanf("%d", &empId);
    for (i = 0; i < count; i++) {
        if (records[i].empId == empId) {
            if (records[i].status == 'P') p++;
            else if (records[i].status == 'A') a++;
            else l++;
        }
    }
    total = p + a + l;
    if (total == 0) {
        printf("No attendance records for this employee.\n");
        return;
    }
    printf("Present: %d\nAbsent: %d\nLeave: %d\nTotal: %d\n", p, a, l, total);
    printf("Attendance percentage: %.2f%%\n", (p * 100.0) / total);
}
 
int main() {
    int choice;
    loadData();
    do {
        printf("\n1. Record Attendance\n2. View Attendance\n3. Attendance Summary\n0. Exit\n");
        printf("Enter choice: ");
        scanf("%d", &choice);
        switch (choice) {
            case 1: recordAttendance(); break;
            case 2: viewAttendance(); break;
            case 3: attendanceSummary(); break;
            case 0: break;
            default: printf("Invalid choice.\n");
        }
    } while (choice != 0);
    return 0;
}
