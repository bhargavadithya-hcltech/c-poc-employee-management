#include <stdio.h>
#include <stdlib.h>
#include <string.h>
 
typedef struct Employee {
    int id;
    char name[50];
    char phone[15];
    char department[30];
    char role[30];
    float salary;
    struct Employee *next;
} Employee;
 
Employee *head = NULL;
 
Employee *findEmployee(int id) {
    Employee *t = head;
    while (t != NULL) {
        if (t->id == id) return t;
        t = t->next;
    }
    return NULL;
}
 
void saveToFile() {
    FILE *fp = fopen("employees.txt", "w");
    Employee *t = head;
    if (fp == NULL) {
        printf("Cannot open file.\n");
        return;
    }
    while (t != NULL) {
        fprintf(fp, "%d|%s|%s|%s|%s|%.2f\n", t->id, t->name, t->phone, t->department, t->role, t->salary);
        t = t->next;
    }
    fclose(fp);
}
 
void loadFromFile() {
    FILE *fp = fopen("employees.txt", "r");
    Employee *e, *tail = NULL;
    int id;
    char name[50], phone[15], dept[30], role[30];
    float salary;
    if (fp == NULL) return;
    while (fscanf(fp, "%d|%49[^|]|%14[^|]|%29[^|]|%29[^|]|%f\n", &id, name, phone, dept, role, &salary) == 6) {
        e = (Employee *)malloc(sizeof(Employee));
        e->id = id;
        strcpy(e->name, name);
        strcpy(e->phone, phone);
        strcpy(e->department, dept);
        strcpy(e->role, role);
        e->salary = salary;
        e->next = NULL;
        if (head == NULL) head = e;
        else tail->next = e;
        tail = e;
    }
    fclose(fp);
}
 
void addEmployee() {
    Employee *e, *t;
    int id;
    printf("Enter Employee ID: ");
    scanf("%d", &id);
    if (id <= 0) {
        printf("ID must be positive.\n");
        return;
    }
    if (findEmployee(id) != NULL) {
        printf("Employee ID already exists.\n");
        return;
    }
    e = (Employee *)malloc(sizeof(Employee));
    e->id = id;
    printf("Enter Name: ");
    scanf(" %[^\n]", e->name);
    printf("Enter Phone: ");
    scanf("%14s", e->phone);
    printf("Enter Department: ");
    scanf(" %[^\n]", e->department);
    printf("Enter Role: ");
    scanf(" %[^\n]", e->role);
    printf("Enter Salary: ");
    scanf("%f", &e->salary);
    if (e->salary <= 0) {
        printf("Salary must be greater than zero.\n");
        free(e);
        return;
    }
    e->next = NULL;
    if (head == NULL) {
        head = e;
    } else {
        t = head;
        while (t->next != NULL) t = t->next;
        t->next = e;
    }
    saveToFile();
    printf("Employee added.\n");
}
 
void viewEmployees() {
    Employee *t = head;
    if (t == NULL) {
        printf("No records.\n");
        return;
    }
    while (t != NULL) {
        printf("%d | %s | %s | %s | %s | %.2f\n", t->id, t->name, t->phone, t->department, t->role, t->salary);
        t = t->next;
    }
}
 
void searchEmployee() {
    int id;
    Employee *e;
    printf("Enter Employee ID to search: ");
    scanf("%d", &id);
    e = findEmployee(id);
    if (e == NULL) {
        printf("Employee not found.\n");
        return;
    }
    printf("%d | %s | %s | %s | %s | %.2f\n", e->id, e->name, e->phone, e->department, e->role, e->salary);
}
 
void updateEmployee() {
    int id;
    Employee *e;
    printf("Enter Employee ID to update: ");
    scanf("%d", &id);
    e = findEmployee(id);
    if (e == NULL) {
        printf("Employee not found.\n");
        return;
    }
    printf("Enter new Name: ");
    scanf(" %[^\n]", e->name);
    printf("Enter new Phone: ");
    scanf("%14s", e->phone);
    printf("Enter new Department: ");
    scanf(" %[^\n]", e->department);
    printf("Enter new Role: ");
    scanf(" %[^\n]", e->role);
    printf("Enter new Salary: ");
    scanf("%f", &e->salary);
    saveToFile();
    printf("Employee updated.\n");
}
 
void deleteEmployee() {
    int id;
    Employee *cur = head, *prev = NULL;
    printf("Enter Employee ID to delete: ");
    scanf("%d", &id);
    while (cur != NULL && cur->id != id) {
        prev = cur;
        cur = cur->next;
    }
    if (cur == NULL) {
        printf("Employee not found.\n");
        return;
    }
    if (prev == NULL) head = cur->next;
    else prev->next = cur->next;
    free(cur);
    saveToFile();
    printf("Employee deleted.\n");
}
 
void freeList() {
    Employee *t;
    while (head != NULL) {
        t = head;
        head = head->next;
        free(t);
    }
}
 
int main() {
    int choice;
    loadFromFile();
    do {
        printf("\n1. Add\n2. View\n3. Search\n4. Update\n5. Delete\n0. Exit\n");
        printf("Enter choice: ");
        scanf("%d", &choice);
        switch (choice) {
            case 1: addEmployee(); break;
            case 2: viewEmployees(); break;
            case 3: searchEmployee(); break;
            case 4: updateEmployee(); break;
            case 5: deleteEmployee(); break;
            case 0: break;
            default: printf("Invalid choice.\n");
        }
    } while (choice != 0);
    freeList();
    return 0;
}
