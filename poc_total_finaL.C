/*
 * Employee Management System - final single-file version (C99)
 *
 * Modules : Employee, Department, Role, Attendance, Payroll, Reports, Users
 * Storage : plain text files in the program's folder, saved after every change
 *   employees.txt    id|name|phone|deptId|roleId|salary
 *   departments.txt  id|name
 *   roles.txt        id|name
 *   attendance.txt   empId day month year status        (P, A or L)
 *   payroll.txt      empId month year basic allowance deduction absentDays absentCut gross net
 *   users.txt        username|passwordHash|level        (1 Admin, 2 HR, 3 Viewer)
 *   log.txt          one line per action (no passwords, no salaries)
 *
 * Build: gcc -std=c99 -Wall -Wextra -pedantic -g -o ems ems.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <stdint.h>

#define EMP_FILE   "employees.txt"
#define DEPT_FILE  "departments.txt"
#define ROLE_FILE  "roles.txt"
#define ATT_FILE   "attendance.txt"
#define PAY_FILE   "payroll.txt"
#define USER_FILE  "users.txt"
#define LOG_FILE   "log.txt"

#define MAX_ITEMS   100          /* departments / roles each        */
#define MAX_USERS   20
#define NAME_SIZE   50
#define ITEM_SIZE   30
#define PHONE_SIZE  11           /* 10 digits + '\0'                */
#define USER_SIZE   20
#define LINE_SIZE   256
#define MAX_ID      999999
#define MAX_MONEY   10000000.0

#define LEVEL_ADMIN  1
#define LEVEL_HR     2
#define LEVEL_VIEWER 3

/* ---------------------------- data structures ---------------------------- */

typedef struct Employee {
    int id;
    char name[NAME_SIZE];
    char phone[PHONE_SIZE];
    int deptId;                  /* only the ID is stored; the name is looked up */
    int roleId;
    double salary;               /* basic monthly salary */
    struct Employee *next;       /* linked list, kept sorted by id */
} Employee;

typedef struct { int id; char name[ITEM_SIZE]; } Item;

/* A department list and a role list work the same way, so they share one type */
typedef struct {
    const char *label;           /* "Department" or "Role" */
    const char *file;
    Item items[MAX_ITEMS];
    int count;
} Table;

typedef struct { int day, month, year; } Date;

typedef struct {                 /* structure composition: Date inside Attendance */
    int empId;
    Date date;
    char status;                 /* P, A or L */
} Attendance;

typedef struct {
    double basic, allowance, deduction, absentCut, gross, net;
} Salary;

typedef struct {                 /* structure composition: Salary inside Payroll */
    int empId, month, year;
    int absentDays;
    Salary sal;
} Payroll;

typedef struct {
    char name[USER_SIZE];
    uint32_t hash;
    int level;
} User;

/* -------------------------------- globals -------------------------------- */

static Employee *head;
static int empCount;
static Table depts, roles;
static Attendance *att;
static int attCount, attCap;
static Payroll *pay;
static int payCount, payCap;
static User users[MAX_USERS];
static int userCount;
static User *current;            /* who is logged in */

static void quitSafely(void);

/* ------------------------------- logging --------------------------------- */

static void logEvent(const char *action, int id)
{
    FILE *f = fopen(LOG_FILE, "a");
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char stamp[32] = "unknown-time";

    if (f == NULL) return;
    if (t != NULL) strftime(stamp, sizeof stamp, "%Y-%m-%d %H:%M:%S", t);
    fprintf(f, "%s | %s | %s | id=%d\n", stamp, current ? current->name : "-", action, id);
    fclose(f);
}

static void logTable(const Table *t, const char *op, int id)
{
    char action[48];
    snprintf(action, sizeof action, "%s_%s", t->label, op);
    logEvent(action, id);
}

/* ------------------------------ input helpers ----------------------------- */

static void trim(char *s)
{
    size_t start = 0, len;
    while (isspace((unsigned char)s[start])) start++;
    if (start > 0) memmove(s, s + start, strlen(s + start) + 1);
    len = strlen(s);
    while (len > 0 && isspace((unsigned char)s[len - 1])) { s[len - 1] = '\0'; len--; }
}

/* Reads one line. Returns 0 if the line was too long (and throws the rest away). */
static int readLine(const char *prompt, char *buf, int size)
{
    int ch;
    printf("%s", prompt);
    fflush(stdout);
    if (fgets(buf, size, stdin) == NULL) {
        printf("\nInput closed. Exiting safely.\n");
        quitSafely();
    }
    if (strchr(buf, '\n') != NULL || feof(stdin)) return 1;
    while ((ch = getchar()) != '\n' && ch != EOF) { }
    printf("That input is too long.\n");
    return 0;
}

static int validText(const char *s)       /* not blank, no '|', no control chars */
{
    int hasChar = 0;
    for (; *s != '\0'; s++) {
        if (*s == '|' || iscntrl((unsigned char)*s)) return 0;
        if (!isspace((unsigned char)*s)) hasChar = 1;
    }
    return hasChar;
}

static int validPhone(const char *s)
{
    size_t i;
    if (strlen(s) != 10) return 0;
    for (i = 0; i < 10; i++) if (!isdigit((unsigned char)s[i])) return 0;
    return 1;
}

static int validUserName(const char *s)
{
    size_t i, n = strlen(s);
    if (n < 3 || n >= USER_SIZE) return 0;
    for (i = 0; i < n; i++) if (!isalnum((unsigned char)s[i]) && s[i] != '_') return 0;
    return 1;
}

static int validPassword(const char *s)
{
    size_t i, n = strlen(s);
    if (n < 6 || n > 29) return 0;
    for (i = 0; i < n; i++) if (isspace((unsigned char)s[i]) || s[i] == '|') return 0;
    return 1;
}

static int readInt(const char *prompt, int lo, int hi)
{
    char buf[LINE_SIZE], *end;
    long v;
    while (1) {
        if (!readLine(prompt, buf, sizeof buf)) continue;
        trim(buf);
        if (buf[0] == '\0') { printf("Please enter a number.\n"); continue; }
        v = strtol(buf, &end, 10);
        if (*end != '\0') { printf("That is not a whole number.\n"); continue; }
        if (v < lo || v > hi) { printf("Enter a number from %d to %d.\n", lo, hi); continue; }
        return (int)v;
    }
}

static double readMoney(const char *prompt, double lo, double hi)
{
    char buf[LINE_SIZE], *end;
    double v;
    while (1) {
        if (!readLine(prompt, buf, sizeof buf)) continue;
        trim(buf);
        if (buf[0] == '\0') { printf("Please enter an amount.\n"); continue; }
        v = strtod(buf, &end);
        if (*end != '\0') { printf("That is not a valid amount.\n"); continue; }
        if (!(v >= lo && v <= hi)) { printf("Amount must be from %.2f to %.2f.\n", lo, hi); continue; }
        return v;
    }
}

/* Returns 1 when text was read, 0 when the user left it blank (only if allowBlank). */
static int readText(const char *prompt, char *dest, int size, int allowBlank)
{
    char buf[LINE_SIZE];
    while (1) {
        if (!readLine(prompt, buf, sizeof buf)) continue;
        trim(buf);
        if (buf[0] == '\0' && allowBlank) return 0;
        if (!validText(buf)) { printf("Cannot be blank and cannot contain the | character.\n"); continue; }
        if ((int)strlen(buf) >= size) { printf("Too long (maximum %d characters).\n", size - 1); continue; }
        snprintf(dest, (size_t)size, "%s", buf);
        return 1;
    }
}

static void readPhone(const char *prompt, char *dest)
{
    char buf[LINE_SIZE];
    while (1) {
        if (!readLine(prompt, buf, sizeof buf)) continue;
        trim(buf);
        if (!validPhone(buf)) { printf("Phone must be exactly 10 digits.\n"); continue; }
        snprintf(dest, PHONE_SIZE, "%.10s", buf);
        return;
    }
}

static void readPassword(const char *prompt, char *dest)
{
    char buf[LINE_SIZE];
    while (1) {
        if (!readLine(prompt, buf, sizeof buf)) continue;
        trim(buf);
        if (!validPassword(buf)) { printf("Password: 6 to 29 characters, no spaces, no |.\n"); continue; }
        snprintf(dest, 30, "%.29s", buf);
        return;
    }
}

static int confirm(const char *prompt)
{
    char buf[LINE_SIZE];
    while (1) {
        if (!readLine(prompt, buf, sizeof buf)) continue;
        trim(buf);
        if (strlen(buf) == 1 && tolower((unsigned char)buf[0]) == 'y') return 1;
        if (strlen(buf) == 1 && tolower((unsigned char)buf[0]) == 'n') return 0;
        printf("Please type y or n.\n");
    }
}

static char readStatus(void)
{
    char buf[LINE_SIZE];
    while (1) {
        if (!readLine("Status (P = Present, A = Absent, L = Leave): ", buf, sizeof buf)) continue;
        trim(buf);
        if (strlen(buf) == 1 && strchr("PAL", toupper((unsigned char)buf[0])) != NULL)
            return (char)toupper((unsigned char)buf[0]);
        printf("Type P, A or L.\n");
    }
}

static int containsIgnoreCase(const char *text, const char *pat)
{
    size_t i, j, n = strlen(text), m = strlen(pat);
    for (i = 0; i + m <= n; i++) {
        for (j = 0; j < m; j++)
            if (tolower((unsigned char)text[i + j]) != tolower((unsigned char)pat[j])) break;
        if (j == m) return 1;
    }
    return 0;
}

static int sameIgnoreCase(const char *a, const char *b)
{
    return strlen(a) == strlen(b) && containsIgnoreCase(a, b);
}

/* ------------------------------- dates ----------------------------------- */

static int isLeapYear(int y) { return (y % 4 == 0 && y % 100 != 0) || y % 400 == 0; }

static int daysInMonth(int m, int y)
{
    static const int days[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (m == 2 && isLeapYear(y)) return 29;
    return days[m - 1];
}

static Date today(void)
{
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    Date d = {1, 1, 2000};
    if (t != NULL) { d.day = t->tm_mday; d.month = t->tm_mon + 1; d.year = t->tm_year + 1900; }
    return d;
}

static int isFuture(Date d)
{
    Date t = today();
    if (d.year != t.year) return d.year > t.year;
    if (d.month != t.month) return d.month > t.month;
    return d.day > t.day;
}

static void readDate(Date *d)
{
    while (1) {
        d->day = readInt("Day (1-31): ", 1, 31);
        d->month = readInt("Month (1-12): ", 1, 12);
        d->year = readInt("Year (2000-2100): ", 2000, 2100);
        if (d->day <= daysInMonth(d->month, d->year)) return;
        printf("That date does not exist. Try again.\n");
    }
}

static double round2(double x) { return (double)((long)(x * 100.0 + 0.5)) / 100.0; }

/* ------------------------------ file helpers ----------------------------- */

/* Saving writes "<file>.tmp" first and only then replaces the real file,
   so a crash in the middle of saving cannot destroy the old data. */
static FILE *openSave(const char *file)
{
    char tmp[64];
    snprintf(tmp, sizeof tmp, "%s.tmp", file);
    return fopen(tmp, "w");
}

static void finishSave(FILE *f, const char *file)
{
    char tmp[64];
    int ok = (f != NULL) && (ferror(f) == 0);
    snprintf(tmp, sizeof tmp, "%s.tmp", file);
    if (f != NULL && fclose(f) != 0) ok = 0;
    if (!ok) { remove(tmp); printf("ERROR: could not write %s. Change is in memory only.\n", file); return; }
    remove(file);                              /* needed on Windows */
    if (rename(tmp, file) != 0) printf("ERROR: could not replace %s (new data is in %s).\n", file, tmp);
}

static void backupFile(const char *file)
{
    char bak[64], buf[512];
    size_t n;
    FILE *in = fopen(file, "rb"), *out;
    if (in == NULL) return;
    snprintf(bak, sizeof bak, "%s.bak", file);
    out = fopen(bak, "wb");
    if (out != NULL) {
        while ((n = fread(buf, 1, sizeof buf, in)) > 0) fwrite(buf, 1, n, out);
        fclose(out);
    }
    fclose(in);
}

static void reportBad(const char *file, int bad)
{
    if (bad > 0) {
        printf("Warning: %d damaged line(s) skipped in %s (original kept as %s.bak).\n", bad, file, file);
        backupFile(file);
    }
}

/* ------------------------- lookups and memory ---------------------------- */

static Employee *findEmployee(int id)
{
    Employee *e = head;
    while (e != NULL && e->id < id) e = e->next;     /* list is sorted: stop early */
    return (e != NULL && e->id == id) ? e : NULL;
}

static void insertEmployee(Employee *e)              /* keeps the list sorted by id */
{
    Employee *p = head, *prev = NULL;
    while (p != NULL && p->id < e->id) { prev = p; p = p->next; }
    e->next = p;
    if (prev == NULL) head = e; else prev->next = e;
    empCount++;
}

static int growAttendance(void)
{
    Attendance *p;
    int cap;
    if (attCount < attCap) return 1;
    cap = (attCap == 0) ? 16 : attCap * 2;
    p = realloc(att, (size_t)cap * sizeof *att);
    if (p == NULL) { printf("Out of memory.\n"); return 0; }
    att = p; attCap = cap;
    return 1;
}

static int growPayroll(void)
{
    Payroll *p;
    int cap;
    if (payCount < payCap) return 1;
    cap = (payCap == 0) ? 16 : payCap * 2;
    p = realloc(pay, (size_t)cap * sizeof *pay);
    if (p == NULL) { printf("Out of memory.\n"); return 0; }
    pay = p; payCap = cap;
    return 1;
}

static void cleanupSystem(void)                      /* explicit cleanup function */
{
    Employee *e = head, *next;
    while (e != NULL) { next = e->next; free(e); e = next; }
    head = NULL; empCount = 0;
    free(att); att = NULL; attCount = 0; attCap = 0;
    free(pay); pay = NULL; payCount = 0; payCap = 0;
}

static void quitSafely(void)
{
    logEvent("EXIT", 0);
    cleanupSystem();
    exit(EXIT_SUCCESS);
}

/* ------------------------- department / role tables ----------------------- */

static int findItem(const Table *t, int id)
{
    int i;
    for (i = 0; i < t->count; i++) if (t->items[i].id == id) return i;
    return -1;
}

static const char *itemName(const Table *t, int id)
{
    int i = findItem(t, id);
    return (i >= 0) ? t->items[i].name : "(missing)";
}

/* which field of an employee belongs to this table */
static int *itemField(Employee *e, const Table *t)
{
    return (t == &depts) ? &e->deptId : &e->roleId;
}

static int countUsing(const Table *t, int id)
{
    Employee *e;
    int n = 0;
    for (e = head; e != NULL; e = e->next) if (*itemField(e, t) == id) n++;
    return n;
}

static int nameTaken(const Table *t, const char *name, int exceptId)
{
    int i;
    for (i = 0; i < t->count; i++)
        if (t->items[i].id != exceptId && sameIgnoreCase(t->items[i].name, name)) return 1;
    return 0;
}

static int nextItemId(const Table *t)
{
    int i, max = 0;
    for (i = 0; i < t->count; i++) if (t->items[i].id > max) max = t->items[i].id;
    return max + 1;
}

static void saveTable(const Table *t)
{
    int i;
    FILE *f = openSave(t->file);
    if (f != NULL)
        for (i = 0; i < t->count; i++) fprintf(f, "%d|%s\n", t->items[i].id, t->items[i].name);
    finishSave(f, t->file);
}

static void loadTable(Table *t)
{
    char line[LINE_SIZE];
    Item it;
    int bad = 0;
    FILE *f = fopen(t->file, "r");
    if (f == NULL) return;                           /* missing file = start empty */
    while (fgets(line, sizeof line, f) != NULL) {
        if (sscanf(line, "%d|%29[^|\n]", &it.id, it.name) != 2 || it.id < 1 || !validText(it.name) ||
            t->count >= MAX_ITEMS || findItem(t, it.id) >= 0 || nameTaken(t, it.name, 0)) { bad++; continue; }
        t->items[t->count++] = it;
    }
    fclose(f);
    reportBad(t->file, bad);
}

/* returns the new id, or 0 if cancelled / full */
static int createItem(Table *t)
{
    char name[ITEM_SIZE], prompt[64];
    int id;
    if (t->count >= MAX_ITEMS) { printf("Limit of %d reached.\n", MAX_ITEMS); return 0; }
    snprintf(prompt, sizeof prompt, "New %s name (blank = cancel): ", t->label);
    while (1) {
        if (!readText(prompt, name, ITEM_SIZE, 1)) { printf("Cancelled.\n"); return 0; }
        if (!nameTaken(t, name, 0)) break;
        printf("That %s already exists.\n", t->label);
    }
    id = nextItemId(t);
    t->items[t->count].id = id;
    snprintf(t->items[t->count].name, ITEM_SIZE, "%s", name);
    t->count++;
    saveTable(t);
    logTable(t, "CREATE", id);
    printf("%s created with id %d.\n", t->label, id);
    return id;
}

static void listItems(const Table *t)
{
    int i;
    printf("\n%-6s %-30s %s\n", "ID", t->label, "Employees");
    for (i = 0; i < t->count; i++)
        printf("%-6d %-30s %d\n", t->items[i].id, t->items[i].name, countUsing(t, t->items[i].id));
}

/* Lets the user choose an existing id. With allowCreate, 0 creates a new one.
   Returns 0 when nothing was chosen. */
static int pickItem(Table *t, int allowCreate)
{
    int id;
    if (t->count == 0) {
        if (!allowCreate) { printf("No %s exists yet.\n", t->label); return 0; }
        printf("No %s exists yet - create the first one now.\n", t->label);
        return createItem(t);
    }
    listItems(t);
    while (1) {
        id = readInt(allowCreate ? "Choose id (0 = create new): " : "Choose id (0 = cancel): ", 0, MAX_ID);
        if (id == 0) return allowCreate ? createItem(t) : 0;
        if (findItem(t, id) >= 0) return id;
        printf("No %s with that id.\n", t->label);
    }
}

static void renameItem(Table *t)
{
    char name[ITEM_SIZE];
    int id = pickItem(t, 0);
    if (id == 0) return;
    while (1) {
        if (!readText("New name (blank = cancel): ", name, ITEM_SIZE, 1)) { printf("Cancelled.\n"); return; }
        if (!nameTaken(t, name, id)) break;
        printf("That name is already used.\n");
    }
    snprintf(t->items[findItem(t, id)].name, ITEM_SIZE, "%s", name);
    saveTable(t);
    logTable(t, "RENAME", id);
    printf("Renamed.\n");
}

static void deleteItem(Table *t)
{
    int i, id = pickItem(t, 0);
    if (id == 0) return;
    if (countUsing(t, id) > 0) {
        printf("Cannot delete: %d employee(s) still use it. Reassign them first.\n", countUsing(t, id));
        return;
    }
    if (!confirm("Delete it? (y/n): ")) return;
    i = findItem(t, id);
    for (; i < t->count - 1; i++) t->items[i] = t->items[i + 1];
    t->count--;
    saveTable(t);
    logTable(t, "DELETE", id);
    printf("Deleted.\n");
}

static void saveEmployees(void)
{
    Employee *e;
    FILE *f = openSave(EMP_FILE);
    if (f != NULL)
        for (e = head; e != NULL; e = e->next)
            fprintf(f, "%d|%s|%s|%d|%d|%.2f\n", e->id, e->name, e->phone, e->deptId, e->roleId, e->salary);
    finishSave(f, EMP_FILE);
}

static Employee *askEmployee(void)
{
    int id = readInt("Employee ID: ", 1, MAX_ID);
    Employee *e = findEmployee(id);
    if (e == NULL) printf("No employee with ID %d.\n", id);
    return e;
}

static void printEmployeeHeader(void)
{
    printf("\n%-7s %-20s %-11s %-16s %-16s %10s\n", "ID", "Name", "Phone", "Department", "Role", "Salary");
}

static void printEmployee(const Employee *e)
{
    printf("%-7d %-20.20s %-11s %-16.16s %-16.16s %10.2f\n", e->id, e->name, e->phone,
           itemName(&depts, e->deptId), itemName(&roles, e->roleId), e->salary);
}

static void assignItem(Table *t)       /* assign or change an employee's department / role */
{
    Employee *e = askEmployee();
    int id;
    if (e == NULL) return;
    printf("%s now: %s\n", t->label, itemName(t, *itemField(e, t)));
    id = pickItem(t, 1);
    if (id == 0) { printf("Nothing changed.\n"); return; }
    *itemField(e, t) = id;
    saveEmployees();
    logTable(t, "ASSIGN", e->id);
    printf("%s of employee %d set to %s.\n", t->label, e->id, itemName(t, id));
}

static void employeesOfItem(Table *t)  /* search employees department-wise / role-wise */
{
    Employee *e;
    int n = 0, id = pickItem(t, 0);
    if (id == 0) return;
    printEmployeeHeader();
    for (e = head; e != NULL; e = e->next)
        if (*itemField(e, t) == id) { printEmployee(e); n++; }
    printf("%d employee(s) in %s.\n", n, itemName(t, id));
}

static int allow(int needLevel);

static void tableMenu(Table *t)
{
    int c;
    do {
        printf("\n--- %s Management ---\n1. Create %s\n2. View all %s (with employee counts)\n3. Rename\n"
               "4. Delete\n5. Assign / change an employee's %s\n6. Search employees by %s\n0. Back\n",
               t->label, t->label, t->label, t->label, t->label);
        c = readInt("Choose: ", 0, 6);
        switch (c) {
            case 1: if (allow(LEVEL_HR)) createItem(t); break;
            case 2: if (t->count == 0) printf("No %s yet.\n", t->label); else listItems(t); break;
            case 3: if (allow(LEVEL_HR)) renameItem(t); break;
            case 4: if (allow(LEVEL_HR)) deleteItem(t); break;
            case 5: if (allow(LEVEL_HR)) assignItem(t); break;
            case 6: employeesOfItem(t); break;
            default: break;
        }
    } while (c != 0);
}

/* ---------------------------- attendance + payroll data ------------------- */

static int findAttendance(int empId, Date d)
{
    int i;
    for (i = 0; i < attCount; i++)
        if (att[i].empId == empId && att[i].date.day == d.day &&
            att[i].date.month == d.month && att[i].date.year == d.year) return i;
    return -1;
}

static int findPayroll(int empId, int month, int year)
{
    int i;
    for (i = 0; i < payCount; i++)
        if (pay[i].empId == empId && pay[i].month == month && pay[i].year == year) return i;
    return -1;
}

/* month == 0 means "all months" */
static void countAttendance(int empId, int month, int year, int *p, int *a, int *l)
{
    int i;
    *p = 0; *a = 0; *l = 0;
    for (i = 0; i < attCount; i++) {
        if (att[i].empId != empId) continue;
        if (month != 0 && (att[i].date.month != month || att[i].date.year != year)) continue;
        if (att[i].status == 'P') (*p)++;
        else if (att[i].status == 'A') (*a)++;
        else (*l)++;
    }
}

static void saveAttendance(void)
{
    int i;
    FILE *f = openSave(ATT_FILE);
    if (f != NULL)
        for (i = 0; i < attCount; i++)
            fprintf(f, "%d %d %d %d %c\n", att[i].empId, att[i].date.day, att[i].date.month,
                    att[i].date.year, att[i].status);
    finishSave(f, ATT_FILE);
}

static void savePayroll(void)
{
    int i;
    FILE *f = openSave(PAY_FILE);
    if (f != NULL)
        for (i = 0; i < payCount; i++)
            fprintf(f, "%d %d %d %.2f %.2f %.2f %d %.2f %.2f %.2f\n", pay[i].empId, pay[i].month,
                    pay[i].year, pay[i].sal.basic, pay[i].sal.allowance, pay[i].sal.deduction,
                    pay[i].absentDays, pay[i].sal.absentCut, pay[i].sal.gross, pay[i].sal.net);
    finishSave(f, PAY_FILE);
}

static void loadEmployees(void)
{
    char line[LINE_SIZE];
    Employee tmp, *e;
    int bad = 0;
    FILE *f = fopen(EMP_FILE, "r");
    if (f == NULL) return;
    while (fgets(line, sizeof line, f) != NULL) {
        if (sscanf(line, "%d|%49[^|]|%10[^|]|%d|%d|%lf", &tmp.id, tmp.name, tmp.phone,
                   &tmp.deptId, &tmp.roleId, &tmp.salary) != 6 ||
            tmp.id < 1 || tmp.id > MAX_ID || !validText(tmp.name) || !validPhone(tmp.phone) ||
            !(tmp.salary > 0 && tmp.salary <= MAX_MONEY) || findEmployee(tmp.id) != NULL ||
            findItem(&depts, tmp.deptId) < 0 || findItem(&roles, tmp.roleId) < 0) { bad++; continue; }
        e = malloc(sizeof *e);
        if (e == NULL) { printf("Out of memory while loading.\n"); break; }
        *e = tmp;
        insertEmployee(e);
    }
    fclose(f);
    reportBad(EMP_FILE, bad);
}

static void loadAttendance(void)
{
    char line[LINE_SIZE], st;
    Attendance a;
    int bad = 0;
    FILE *f = fopen(ATT_FILE, "r");
    if (f == NULL) return;
    while (fgets(line, sizeof line, f) != NULL) {
        if (sscanf(line, "%d %d %d %d %c", &a.empId, &a.date.day, &a.date.month, &a.date.year, &st) != 5 ||
            a.date.month < 1 || a.date.month > 12 || a.date.year < 2000 || a.date.year > 2100 ||
            a.date.day < 1 || a.date.day > daysInMonth(a.date.month, a.date.year) ||
            strchr("PAL", st) == NULL || findEmployee(a.empId) == NULL) { bad++; continue; }
        a.status = st;
        if (!growAttendance()) break;
        att[attCount++] = a;
    }
    fclose(f);
    reportBad(ATT_FILE, bad);
}

static void loadPayroll(void)
{
    char line[LINE_SIZE];
    Payroll p;
    int bad = 0;
    FILE *f = fopen(PAY_FILE, "r");
    if (f == NULL) return;
    while (fgets(line, sizeof line, f) != NULL) {
        if (sscanf(line, "%d %d %d %lf %lf %lf %d %lf %lf %lf", &p.empId, &p.month, &p.year,
                   &p.sal.basic, &p.sal.allowance, &p.sal.deduction, &p.absentDays,
                   &p.sal.absentCut, &p.sal.gross, &p.sal.net) != 10 ||
            p.month < 1 || p.month > 12 || p.year < 2000 || p.year > 2100 || p.absentDays < 0 ||
            p.sal.net < 0 || findEmployee(p.empId) == NULL ||
            findPayroll(p.empId, p.month, p.year) >= 0) { bad++; continue; }
        if (!growPayroll()) break;
        pay[payCount++] = p;
    }
    fclose(f);
    reportBad(PAY_FILE, bad);
}

/* ------------------------------ user accounts ----------------------------- */
/* NOTE: this simple salted hash is for a learning project. A real product
   would use a proper password library (bcrypt / argon2). */

static uint32_t hashPassword(const char *user, const char *pass)
{
    uint32_t h = 5381u;
    const char *s;
    for (s = user; *s != '\0'; s++) h = h * 33u + (uint32_t)tolower((unsigned char)*s);
    h = h * 33u + (uint32_t)':';
    for (s = pass; *s != '\0'; s++) h = h * 33u + (uint32_t)(unsigned char)*s;
    return h;
}

static int findUser(const char *name)
{
    int i;
    for (i = 0; i < userCount; i++) if (sameIgnoreCase(users[i].name, name)) return i;
    return -1;
}

static const char *levelName(int level)
{
    return (level == LEVEL_ADMIN) ? "Admin" : (level == LEVEL_HR) ? "HR" : "Viewer";
}

static void saveUsers(void)
{
    int i;
    FILE *f = openSave(USER_FILE);
    if (f != NULL)
        for (i = 0; i < userCount; i++)
            fprintf(f, "%s|%lu|%d\n", users[i].name, (unsigned long)users[i].hash, users[i].level);
    finishSave(f, USER_FILE);
}

static void loadUsers(void)
{
    char line[LINE_SIZE];
    User u;
    unsigned long h;
    int bad = 0;
    FILE *f = fopen(USER_FILE, "r");
    if (f == NULL) return;
    while (fgets(line, sizeof line, f) != NULL) {
        if (sscanf(line, "%19[^|]|%lu|%d", u.name, &h, &u.level) != 3 || !validUserName(u.name) ||
            u.level < LEVEL_ADMIN || u.level > LEVEL_VIEWER || userCount >= MAX_USERS ||
            findUser(u.name) >= 0) { bad++; continue; }
        u.hash = (uint32_t)h;
        users[userCount++] = u;
    }
    fclose(f);
    reportBad(USER_FILE, bad);
}

static int allow(int needLevel)          /* role-based access control check */
{
    if (current != NULL && current->level <= needLevel) return 1;
    printf("Access denied: a %s account cannot do this.\n", current ? levelName(current->level) : "?");
    logEvent("ACCESS_DENIED", 0);
    return 0;
}

static void readNewPassword(char *dest)
{
    char again[30];
    while (1) {
        readPassword("New password (6-29 chars): ", dest);
        readPassword("Type it again: ", again);
        if (strcmp(dest, again) == 0) return;
        printf("The two passwords do not match.\n");
    }
}

static void addUser(void)
{
    char name[USER_SIZE], pw[30];
    User u;
    if (userCount >= MAX_USERS) { printf("User limit reached.\n"); return; }
    while (1) {
        readText("New username (3-19 letters/digits/_): ", name, USER_SIZE, 0);
        if (!validUserName(name)) { printf("Use only letters, digits and _ (3 to 19 characters).\n"); continue; }
        if (findUser(name) >= 0) { printf("That username exists.\n"); continue; }
        break;
    }
    readNewPassword(pw);
    u.level = readInt("Access level (1 = Admin, 2 = HR, 3 = Viewer): ", 1, 3);
    snprintf(u.name, USER_SIZE, "%s", name);
    u.hash = hashPassword(name, pw);
    users[userCount++] = u;
    saveUsers();
    logEvent("USER_ADD", userCount);
    printf("User %s created as %s.\n", u.name, levelName(u.level));
}

static void listUsers(void)
{
    int i;
    printf("\n%-20s %s\n", "Username", "Access level");
    for (i = 0; i < userCount; i++) printf("%-20s %s\n", users[i].name, levelName(users[i].level));
}

static void deleteUser(void)
{
    char name[USER_SIZE];
    int i, admins = 0, idx, cur;
    readText("Username to delete: ", name, USER_SIZE, 0);
    idx = findUser(name);
    if (idx < 0) { printf("No such user.\n"); return; }
    if (&users[idx] == current) { printf("You cannot delete the account you are using.\n"); return; }
    for (i = 0; i < userCount; i++) if (users[i].level == LEVEL_ADMIN) admins++;
    if (users[idx].level == LEVEL_ADMIN && admins <= 1) { printf("Cannot delete the last Admin.\n"); return; }
    if (!confirm("Delete this user? (y/n): ")) return;
    cur = (int)(current - users);                      /* remember who is logged in */
    for (i = idx; i < userCount - 1; i++) users[i] = users[i + 1];
    userCount--;
    if (cur > idx) cur--;                              /* the array shifted down */
    current = &users[cur];
    saveUsers();
    logEvent("USER_DELETE", idx);
    printf("User deleted.\n");
}

static void changeOwnPassword(void)
{
    char old[30], pw[30];
    readPassword("Current password: ", old);
    if (hashPassword(current->name, old) != current->hash) { printf("Wrong current password.\n"); logEvent("PASSWORD_FAIL", 0); return; }
    readNewPassword(pw);
    current->hash = hashPassword(current->name, pw);
    saveUsers();
    logEvent("PASSWORD_CHANGE", 0);
    printf("Password changed.\n");
}

static void resetPassword(void)
{
    char name[USER_SIZE], pw[30];
    int idx;
    readText("Username: ", name, USER_SIZE, 0);
    idx = findUser(name);
    if (idx < 0) { printf("No such user.\n"); return; }
    readNewPassword(pw);
    users[idx].hash = hashPassword(users[idx].name, pw);
    saveUsers();
    logEvent("PASSWORD_RESET", idx);
    printf("Password reset.\n");
}

static void userMenu(void)
{
    int c;
    do {
        printf("\n--- User Accounts ---\n1. Change my password\n2. Add user (Admin)\n3. List users (Admin)\n"
               "4. Delete user (Admin)\n5. Reset a user's password (Admin)\n0. Back\n");
        c = readInt("Choose: ", 0, 5);
        switch (c) {
            case 1: changeOwnPassword(); break;
            case 2: if (allow(LEVEL_ADMIN)) addUser(); break;
            case 3: if (allow(LEVEL_ADMIN)) listUsers(); break;
            case 4: if (allow(LEVEL_ADMIN)) deleteUser(); break;
            case 5: if (allow(LEVEL_ADMIN)) resetPassword(); break;
            default: break;
        }
    } while (c != 0);
}

static int login(void)
{
    char name[USER_SIZE], pw[30];
    int tries, idx;

    if (userCount == 0) {                      /* very first run: no default password exists */
        printf("No user accounts yet. Create the first Admin account.\n");
        while (1) {
            readText("Admin username (3-19 letters/digits/_): ", name, USER_SIZE, 0);
            if (validUserName(name)) break;
            printf("Use only letters, digits and _ (3 to 19 characters).\n");
        }
        readNewPassword(pw);
        snprintf(users[0].name, USER_SIZE, "%s", name);
        users[0].hash = hashPassword(name, pw);
        users[0].level = LEVEL_ADMIN;
        userCount = 1;
        saveUsers();
    }
    for (tries = 1; tries <= 3; tries++) {
        printf("\n=== Login (attempt %d of 3) ===\n", tries);
        readText("Username: ", name, USER_SIZE, 0);
        readPassword("Password: ", pw);        /* note: typing is visible in a plain console */
        idx = findUser(name);
        if (idx >= 0 && users[idx].hash == hashPassword(users[idx].name, pw)) {
            current = &users[idx];
            logEvent("LOGIN", 0);
            printf("Welcome, %s (%s).\n", current->name, levelName(current->level));
            return 1;
        }
        printf("Wrong username or password.\n");
        logEvent("LOGIN_FAILED", tries);
    }
    printf("Too many failed attempts. Goodbye.\n");
    return 0;
}

/* -------------------------------- employees ------------------------------- */

static void addEmployee(void)
{
    Employee *e;
    int id = readInt("Employee ID (1-999999, 0 = cancel): ", 0, MAX_ID);
    if (id == 0) return;
    if (findEmployee(id) != NULL) { printf("That Employee ID already exists.\n"); return; }
    e = malloc(sizeof *e);
    if (e == NULL) { printf("Out of memory.\n"); return; }
    e->id = id;
    readText("Name: ", e->name, NAME_SIZE, 0);
    readPhone("Phone (10 digits): ", e->phone);
    printf("\nChoose a department:");
    e->deptId = pickItem(&depts, 1);
    if (e->deptId == 0) { printf("An employee needs a department. Cancelled.\n"); free(e); return; }
    printf("\nChoose a role:");
    e->roleId = pickItem(&roles, 1);
    if (e->roleId == 0) { printf("An employee needs a role. Cancelled.\n"); free(e); return; }
    e->salary = readMoney("Basic monthly salary: ", 0.01, MAX_MONEY);
    insertEmployee(e);
    saveEmployees();
    logEvent("EMPLOYEE_ADD", id);
    printf("Employee %d added.\n", id);
}

static void viewEmployees(void)
{
    Employee *e;
    if (head == NULL) { printf("No employees yet.\n"); return; }
    printEmployeeHeader();
    for (e = head; e != NULL; e = e->next) printEmployee(e);
    printf("Total: %d\n", empCount);
}

static void searchEmployeeById(void)
{
    Employee *e = askEmployee();
    if (e == NULL) return;
    printEmployeeHeader();
    printEmployee(e);
}

static void updateEmployee(void)
{
    Employee *e = askEmployee();
    int c;
    if (e == NULL) return;
    printEmployeeHeader();
    printEmployee(e);
    printf("1. Name\n2. Phone\n3. Salary\n4. Department\n5. Role\n0. Cancel\n");
    c = readInt("What to change: ", 0, 5);
    if (c == 0) return;
    if (c == 1) readText("New name: ", e->name, NAME_SIZE, 0);
    else if (c == 2) readPhone("New phone (10 digits): ", e->phone);
    else if (c == 3) e->salary = readMoney("New basic salary: ", 0.01, MAX_MONEY);
    else {
        int id = pickItem(c == 4 ? &depts : &roles, 1);
        if (id == 0) { printf("Nothing changed.\n"); return; }
        if (c == 4) e->deptId = id; else e->roleId = id;
    }
    saveEmployees();
    logEvent("EMPLOYEE_UPDATE", e->id);
    printf("Updated.\n");
}

static void deleteEmployee(void)
{
    Employee *e = askEmployee(), *p, *prev = NULL;
    int i, keep, id;
    if (e == NULL) return;
    printEmployeeHeader();
    printEmployee(e);
    if (!confirm("Delete this employee AND all their attendance and payroll? (y/n): ")) return;
    id = e->id;
    for (p = head; p != e; p = p->next) prev = p;     /* find the node before e */
    if (prev == NULL) head = e->next; else prev->next = e->next;
    free(e);
    empCount--;
    for (i = 0, keep = 0; i < attCount; i++) if (att[i].empId != id) att[keep++] = att[i];
    attCount = keep;
    for (i = 0, keep = 0; i < payCount; i++) if (pay[i].empId != id) pay[keep++] = pay[i];
    payCount = keep;
    saveEmployees(); saveAttendance(); savePayroll();
    logEvent("EMPLOYEE_DELETE", id);
    printf("Employee %d deleted.\n", id);
}

static void employeeMenu(void)
{
    int c;
    do {
        printf("\n--- Employee Management ---\n1. Add employee\n2. View all employees\n"
               "3. Search by Employee ID\n4. Update employee\n5. Delete employee\n0. Back\n");
        c = readInt("Choose: ", 0, 5);
        switch (c) {
            case 1: if (allow(LEVEL_HR)) addEmployee(); break;
            case 2: viewEmployees(); break;
            case 3: searchEmployeeById(); break;
            case 4: if (allow(LEVEL_HR)) updateEmployee(); break;
            case 5: if (allow(LEVEL_HR)) deleteEmployee(); break;
            default: break;
        }
    } while (c != 0);
}

/* ------------------------------- attendance ------------------------------- */

static const char *statusWord(char s)
{
    return (s == 'P') ? "Present" : (s == 'A') ? "Absent" : "Leave";
}

/* attendance of a month is frozen once payroll for that month exists */
static int monthLocked(int empId, Date d)
{
    if (findPayroll(empId, d.month, d.year) < 0) return 0;
    printf("Payroll for %02d/%d already exists. Delete that payroll first to change attendance.\n", d.month, d.year);
    return 1;
}

static void recordAttendance(void)
{
    Employee *e = askEmployee();
    Attendance a;
    if (e == NULL) return;
    readDate(&a.date);
    if (isFuture(a.date)) { printf("Cannot record attendance for a future date.\n"); return; }
    if (monthLocked(e->id, a.date)) return;
    if (findAttendance(e->id, a.date) >= 0) { printf("Already recorded for that day. Use Correct instead.\n"); return; }
    a.empId = e->id;
    a.status = readStatus();
    if (!growAttendance()) return;
    att[attCount++] = a;
    saveAttendance();
    logEvent("ATTENDANCE_ADD", e->id);
    printf("Recorded: %s on %02d/%02d/%d.\n", statusWord(a.status), a.date.day, a.date.month, a.date.year);
}

static void correctAttendance(void)
{
    Employee *e = askEmployee();
    Date d;
    int i;
    if (e == NULL) return;
    readDate(&d);
    if (monthLocked(e->id, d)) return;
    i = findAttendance(e->id, d);
    if (i < 0) { printf("No record for that day. Use Record instead.\n"); return; }
    printf("Current status: %s\n", statusWord(att[i].status));
    att[i].status = readStatus();
    saveAttendance();
    logEvent("ATTENDANCE_FIX", e->id);
    printf("Corrected.\n");
}

static void viewAttendance(void)
{
    Employee *e = askEmployee();
    int i, p, a, l;
    if (e == NULL) return;
    printf("\nAttendance of %s (ID %d)\n", e->name, e->id);
    for (i = 0; i < attCount; i++)
        if (att[i].empId == e->id)
            printf("  %02d/%02d/%d  %s\n", att[i].date.day, att[i].date.month, att[i].date.year, statusWord(att[i].status));
    countAttendance(e->id, 0, 0, &p, &a, &l);
    printf("Totals: Present %d, Absent %d, Leave %d\n", p, a, l);
}

static void attendanceSummary(void)                 /* employee-wise, for one month */
{
    Employee *e;
    int m = readInt("Month (1-12): ", 1, 12), y = readInt("Year (2000-2100): ", 2000, 2100);
    int p, a, l;
    if (head == NULL) { printf("No employees yet.\n"); return; }
    printf("\nAttendance summary for %02d/%d\n%-7s %-20s %8s %7s %6s\n", m, y, "ID", "Name", "Present", "Absent", "Leave");
    for (e = head; e != NULL; e = e->next) {
        countAttendance(e->id, m, y, &p, &a, &l);
        printf("%-7d %-20.20s %8d %7d %6d\n", e->id, e->name, p, a, l);
    }
}

static void attendanceMenu(void)
{
    int c;
    do {
        printf("\n--- Attendance Management ---\n1. Record attendance\n2. Correct attendance\n"
               "3. View an employee's attendance\n4. Employee-wise summary (one month)\n0. Back\n");
        c = readInt("Choose: ", 0, 4);
        switch (c) {
            case 1: if (allow(LEVEL_HR)) recordAttendance(); break;
            case 2: if (allow(LEVEL_HR)) correctAttendance(); break;
            case 3: viewAttendance(); break;
            case 4: attendanceSummary(); break;
            default: break;
        }
    } while (c != 0);
}

/* --------------------------------- payroll -------------------------------- */

static void printPayrollHeader(void)
{
    printf("\n%-7s %-9s %10s %10s %10s %6s %10s %10s %10s\n", "EmpID", "Month", "Basic", "Allowance",
           "Deduction", "AbsDay", "AbsCut", "Gross", "Net");
}

static void printPayroll(const Payroll *p)
{
    printf("%-7d %02d/%-6d %10.2f %10.2f %10.2f %6d %10.2f %10.2f %10.2f\n", p->empId, p->month, p->year,
           p->sal.basic, p->sal.allowance, p->sal.deduction, p->absentDays, p->sal.absentCut,
           p->sal.gross, p->sal.net);
}

static void generatePayroll(void)
{
    Employee *e = askEmployee();
    Payroll p;
    Date t = today();
    int present, absent, leave;
    double total;
    if (e == NULL) return;
    p.empId = e->id;
    p.month = readInt("Month (1-12): ", 1, 12);
    p.year = readInt("Year (2000-2100): ", 2000, 2100);
    if (p.year > t.year || (p.year == t.year && p.month > t.month)) { printf("Cannot create payroll for a future month.\n"); return; }
    if (findPayroll(p.empId, p.month, p.year) >= 0) { printf("Payroll for this employee and month already exists.\n"); return; }
    p.sal.basic = e->salary;                              /* taken from the employee record */
    p.sal.allowance = readMoney("Allowance: ", 0, MAX_MONEY);
    p.sal.deduction = readMoney("Other deduction: ", 0, MAX_MONEY);
    countAttendance(e->id, p.month, p.year, &present, &absent, &leave);
    p.absentDays = absent;                                /* Absent = unpaid, Leave = paid */
    p.sal.absentCut = round2(p.sal.basic / daysInMonth(p.month, p.year) * absent);
    p.sal.gross = p.sal.basic + p.sal.allowance;
    total = p.sal.deduction + p.sal.absentCut;
    if (total > p.sal.gross + 0.005) { printf("Deductions (%.2f) are more than gross (%.2f). Not saved.\n", total, p.sal.gross); return; }
    p.sal.net = p.sal.gross - total;
    if (p.sal.net < 0) p.sal.net = 0;
    printf("\nBasic %.2f + Allowance %.2f = Gross %.2f\n", p.sal.basic, p.sal.allowance, p.sal.gross);
    printf("Deduction %.2f + Absence cut %.2f (%d absent day(s)) = %.2f\nNet salary = %.2f\n",
           p.sal.deduction, p.sal.absentCut, absent, total, p.sal.net);
    if (!confirm("Save this payroll? (y/n): ")) return;
    if (!growPayroll()) return;
    pay[payCount++] = p;
    savePayroll();
    logEvent("PAYROLL_ADD", p.empId);
    printf("Payroll saved.\n");
}

static void viewPayrollByEmployee(void)
{
    Employee *e = askEmployee();
    int i, n = 0;
    if (e == NULL) return;
    printPayrollHeader();
    for (i = 0; i < payCount; i++) if (pay[i].empId == e->id) { printPayroll(&pay[i]); n++; }
    if (n == 0) printf("No payroll records for %s.\n", e->name);
}

static void payrollSummary(void)                        /* all employees, one month */
{
    int m = readInt("Month (1-12): ", 1, 12), y = readInt("Year (2000-2100): ", 2000, 2100);
    int i, n = 0;
    double gross = 0, net = 0;
    printPayrollHeader();
    for (i = 0; i < payCount; i++)
        if (pay[i].month == m && pay[i].year == y) {
            printPayroll(&pay[i]);
            gross += pay[i].sal.gross; net += pay[i].sal.net; n++;
        }
    if (n == 0) { printf("No payroll for %02d/%d.\n", m, y); return; }
    printf("Records: %d   Total gross: %.2f   Total net: %.2f\n", n, gross, net);
}

static void deletePayroll(void)
{
    Employee *e = askEmployee();
    int m, y, i;
    if (e == NULL) return;
    m = readInt("Month (1-12): ", 1, 12);
    y = readInt("Year (2000-2100): ", 2000, 2100);
    i = findPayroll(e->id, m, y);
    if (i < 0) { printf("No such payroll record.\n"); return; }
    printPayrollHeader();
    printPayroll(&pay[i]);
    if (!confirm("Delete this payroll record so it can be created again? (y/n): ")) return;
    for (; i < payCount - 1; i++) pay[i] = pay[i + 1];
    payCount--;
    savePayroll();
    logEvent("PAYROLL_DELETE", e->id);
    printf("Deleted.\n");
}

static void payrollMenu(void)
{
    int c;
    do {
        printf("\n--- Payroll Management ---\n1. Generate monthly payroll\n2. View payroll of an employee\n"
               "3. Monthly payroll summary\n4. Delete a payroll record (to redo it)\n0. Back\n");
        c = readInt("Choose: ", 0, 4);
        switch (c) {
            case 1: if (allow(LEVEL_HR)) generatePayroll(); break;
            case 2: viewPayrollByEmployee(); break;
            case 3: payrollSummary(); break;
            case 4: if (allow(LEVEL_HR)) deletePayroll(); break;
            default: break;
        }
    } while (c != 0);
}

/* --------------------------- search and reporting ------------------------- */

static void searchByName(void)
{
    char key[NAME_SIZE];
    Employee *e;
    int n = 0;
    readText("Name (or part of it): ", key, NAME_SIZE, 0);
    printEmployeeHeader();
    for (e = head; e != NULL; e = e->next)
        if (containsIgnoreCase(e->name, key)) { printEmployee(e); n++; }
    printf("%d match(es).\n", n);
}

static void employeeSummary(void)
{
    Employee *e;
    double total = 0;
    int i;
    for (e = head; e != NULL; e = e->next) total += e->salary;
    printf("\nEmployees: %d\nTotal monthly basic salary: %.2f\n", empCount, total);
    if (empCount > 0) printf("Average basic salary: %.2f\n", total / empCount);
    printf("\nBy department:\n");
    for (i = 0; i < depts.count; i++) printf("  %-30s %d\n", depts.items[i].name, countUsing(&depts, depts.items[i].id));
    printf("By role:\n");
    for (i = 0; i < roles.count; i++) printf("  %-30s %d\n", roles.items[i].name, countUsing(&roles, roles.items[i].id));
}

static void reportMenu(void)
{
    int c;
    do {
        printf("\n--- Search & Reporting ---\n1. Search by Employee ID\n2. Search by name\n"
               "3. Search by department\n4. Search by role\n5. Employee summary\n"
               "6. Attendance summary (one month)\n7. Payroll summary (one month)\n0. Back\n");
        c = readInt("Choose: ", 0, 7);
        switch (c) {
            case 1: searchEmployeeById(); break;
            case 2: searchByName(); break;
            case 3: employeesOfItem(&depts); break;
            case 4: employeesOfItem(&roles); break;
            case 5: employeeSummary(); break;
            case 6: attendanceSummary(); break;
            case 7: payrollSummary(); break;
            default: break;
        }
    } while (c != 0);
}

/* ---------------------------------- main ---------------------------------- */

static void initSystem(void)                            /* explicit initialization function */
{
    head = NULL; empCount = 0;
    att = NULL; attCount = 0; attCap = 0;
    pay = NULL; payCount = 0; payCap = 0;
    userCount = 0; current = NULL;
    depts.label = "Department"; depts.file = DEPT_FILE; depts.count = 0;
    roles.label = "Role";       roles.file = ROLE_FILE; roles.count = 0;
    loadUsers();
    loadTable(&depts);
    loadTable(&roles);
    loadEmployees();                                    /* needs departments and roles */
    loadAttendance();                                   /* needs employees */
    loadPayroll();
}

int main(void)
{
    int c;
    initSystem();
    if (!login()) { cleanupSystem(); return 1; }
    do {
        printf("\n===== EMPLOYEE MANAGEMENT SYSTEM =====\n");
        printf("User: %s (%s) | Employees: %d | Departments: %d | Roles: %d\n",
               current->name, levelName(current->level), empCount, depts.count, roles.count);
        printf("1. Employee\n2. Department\n3. Role\n4. Attendance\n5. Payroll\n"
               "6. Search & Reporting\n7. User accounts\n0. Save and exit\n");
        c = readInt("Choose: ", 0, 7);
        switch (c) {
            case 1: employeeMenu(); break;
            case 2: tableMenu(&depts); break;
            case 3: tableMenu(&roles); break;
            case 4: attendanceMenu(); break;
            case 5: payrollMenu(); break;
            case 6: reportMenu(); break;
            case 7: userMenu(); break;
            default: break;
        }
    } while (c != 0);
    printf("All data is saved. Goodbye.\n");
    quitSafely();
    return 0;
}
