/**
 * @file test_framework.c
 * @brief Runner, checks and per-test fixture for the C unit tests.
 */
#define _POSIX_C_SOURCE 200809L
#include <dirent.h>
#include <stdlib.h>
#include <unistd.h>
#include "test_framework.h"

static int g_failed = 0;
static char g_dir[64];
static FILE *g_in = NULL;
static FILE *g_out = NULL;
static char g_inputText[8192];
static char g_outputText[65536];
static char g_fileText[16384];
static char g_pathText[4][PATH_SIZE];
static int g_pathSlot = 0;
static char g_repeatText[4][300];
static int g_repeatSlot = 0;

/* ------------------------------------------------------------ checks */
void testFail(const char *file, int line, const char *message)
{
    (void)printf("    %s:%d: Failure: %s\n", file, line, message);
    g_failed = 1;
}

void testFailInt(const char *file, int line, const char *expr, long long expected, long long actual)
{
    char msg[256];

    (void)snprintf(msg, sizeof(msg), "%s (expected %lld, got %lld)", expr, expected, actual);
    testFail(file, line, msg);
}

void testFailStr(const char *file, int line, const char *expr, const char *expected, const char *actual)
{
    char msg[512];

    (void)snprintf(msg, sizeof(msg), "%s (expected \"%s\", got \"%s\")", expr,
                   (expected != NULL) ? expected : "NULL", (actual != NULL) ? actual : "NULL");
    testFail(file, line, msg);
}

int testCurrentFailed(void)
{
    return g_failed;
}

/* ------------------------------------------------------------ fixture */
static void removeTestDir(void)
{
    DIR *d = opendir(g_dir);
    struct dirent *ent;
    char path[PATH_SIZE + 300];

    if (d != NULL) {
        ent = readdir(d);
        while (ent != NULL) {
            if ((strcmp(ent->d_name, ".") != 0) && (strcmp(ent->d_name, "..") != 0)) {
                (void)snprintf(path, sizeof(path), "%s/%s", g_dir, ent->d_name);
                (void)remove(path);
            }
            ent = readdir(d);
        }
        (void)closedir(d);
    }
    (void)rmdir(g_dir);
}

static void setUp(void)
{
    g_failed = 0;
    (void)snprintf(g_dir, sizeof(g_dir), "%s", "/tmp/ems_test_XXXXXX");
    if (mkdtemp(g_dir) == NULL) {
        testFail(__FILE__, __LINE__, "mkdtemp failed");
    }
    freeAllMemory();
    (void)setDataDirectory(g_dir);
    (void)logInit(testPath("test.log"));
    g_out = tmpfile();
    setOutputStream(g_out);
    setSession("admin", ROLE_ADMIN);
}

static void tearDown(void)
{
    setInputStream(NULL);
    if (g_in != NULL) {
        (void)fclose(g_in);
        g_in = NULL;
    }
    setOutputStream(NULL);
    if (g_out != NULL) {
        (void)fclose(g_out);
        g_out = NULL;
    }
    freeAllMemory();
    logClose();
    removeTestDir();
}

void runTest(const char *name, TestFn test, TestFn extraSetUp)
{
    (void)printf("[ RUN      ] %s\n", name);
    (void)fflush(stdout);
    setUp();
    if ((extraSetUp != NULL) && (g_failed == 0)) {
        extraSetUp();
    }
    if (g_failed == 0) {
        test();
    }
    tearDown();
    if (g_failed == 0) {
        (void)printf("[       OK ] %s\n", name);
    } else {
        (void)printf("[  FAILED  ] %s\n", name);
    }
    recordTestResult(name, g_failed);
}

const char *testDir(void)
{
    return g_dir;
}

const char *testPath(const char *name)
{
    g_pathSlot = (g_pathSlot + 1) % 4;
    (void)snprintf(g_pathText[g_pathSlot], sizeof(g_pathText[g_pathSlot]), "%s/%s", g_dir, name);
    return g_pathText[g_pathSlot];
}

FILE *testInput(void)
{
    return g_in;
}

FILE *testOutput(void)
{
    return g_out;
}

void feed(const char *text)
{
    size_t len = strlen(text);

    if (g_in != NULL) {
        (void)fclose(g_in);
        g_in = NULL;
    }
    if (len >= sizeof(g_inputText)) {
        testFail(__FILE__, __LINE__, "feed(): input too long");
        len = 0U;
    }
    (void)memcpy(g_inputText, text, len);
    g_inputText[len] = '\0';
    if (len == 0U) {
        g_in = fopen("/dev/null", "r");
    } else {
        g_in = fmemopen(g_inputText, len, "r");
    }
    setInputStream(g_in);
}

const char *getOutput(void)
{
    size_t got;
    long size;

    (void)fflush(g_out);
    size = ftell(g_out);
    if ((size < 0) || ((size_t)size >= sizeof(g_outputText))) {
        size = (long)sizeof(g_outputText) - 1L;
    }
    rewind(g_out);
    got = fread(g_outputText, 1U, (size_t)size, g_out);
    g_outputText[got] = '\0';
    (void)fseek(g_out, 0L, SEEK_END);
    return g_outputText;
}

const char *readFile(const char *name)
{
    FILE *fp;
    size_t got = 0U;

    (void)fflush(NULL);
    fp = fopen(testPath(name), "r");
    if (fp != NULL) {
        got = fread(g_fileText, 1U, sizeof(g_fileText) - 1U, fp);
        (void)fclose(fp);
    }
    g_fileText[got] = '\0';
    return g_fileText;
}

const char *logText(void)
{
    return readFile("test.log");
}

void writeFile(const char *name, const char *text)
{
    FILE *fp = fopen(testPath(name), "w");

    if (fp != NULL) {
        (void)fputs(text, fp);
        (void)fclose(fp);
    }
}

int fileExists(const char *name)
{
    return (access(testPath(name), F_OK) == 0) ? 1 : 0;
}

const char *repeatChar(int n, char c)
{
    int i;

    g_repeatSlot = (g_repeatSlot + 1) % 4;
    for (i = 0; (i < n) && (i < 299); i++) {
        g_repeatText[g_repeatSlot][i] = c;
    }
    g_repeatText[g_repeatSlot][i] = '\0';
    return g_repeatText[g_repeatSlot];
}

int countOccurrences(const char *text, const char *part)
{
    int count = 0;
    const char *pos = strstr(text, part);

    while (pos != NULL) {
        count++;
        pos = strstr(pos + 1, part);
    }
    return count;
}

Employee makeEmployeeFull(int id, const char *name, int dept, int role, double salary)
{
    Employee e;

    (void)memset(&e, 0, sizeof(e));
    e.id = id;
    (void)snprintf(e.name, sizeof(e.name), "%s", name);
    (void)snprintf(e.phone, sizeof(e.phone), "%s", "9876543210");
    (void)snprintf(e.email, sizeof(e.email), "%s", "ravi@corp.com");
    e.deptId = dept;
    e.roleId = role;
    e.basicSalary = salary;
    return e;
}

Employee makeEmployee(int id)
{
    return makeEmployeeFull(id, "Ravi Kumar", 10, 20, 30000.0);
}

Date makeDate(int day, int month, int year)
{
    Date d;

    d.day = day;
    d.month = month;
    d.year = year;
    return d;
}

void seed(void)
{
    Employee a = makeEmployeeFull(101, "Ravi Kumar", 10, 20, 30000.0);
    Employee b = makeEmployeeFull(102, "Priya Sharma", 11, 21, 24000.0);

    EXPECT_EQ(departmentAdd(10, "Finance", NULL), EMS_OK);
    EXPECT_EQ(departmentAdd(11, "HR", NULL), EMS_OK);
    EXPECT_EQ(roleAdd(20, "Accountant", NULL), EMS_OK);
    EXPECT_EQ(roleAdd(21, "Executive", NULL), EMS_OK);
    EXPECT_EQ(employeeInsert(&a), EMS_OK);
    EXPECT_EQ(employeeInsert(&b), EMS_OK);
}
