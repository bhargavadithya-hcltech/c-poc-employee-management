/**
 * @file test_cunit.c
 * @brief CUnit test runner (pure C) for the Employee Management System.
 *
 * One CUnit suite per module, each with usual, unusual and edge cases.
 * It reuses tests/test_shim.c, so every test starts from a clean system
 * in its own temporary data folder.
 *
 *   make cunit  ->  verbose CUnit output on the terminal and in cunit_report.txt
 */
#include <CUnit/Basic.h>
#include <stdlib.h>
#include <string.h>
#include "../test_shim.h"

static int suiteSetUp(void) { return 0; }
static int suiteTearDown(void) { return 0; }
static void setUpEach(void) { test_reset_system(); }
static void setUpEachSeeded(void) { test_reset_system(); (void)test_seed_data(); }
static void tearDownEach(void) { test_cleanup_system(); }

/* ------------------------------------------------------------ validation */
static void validation_usual_validFields(void)
{
    CU_ASSERT_EQUAL(validatePhone("9876543210"), 1);
    CU_ASSERT_EQUAL(validateEmail("ravi@corp.com"), 1);
    CU_ASSERT_EQUAL(validateDate(29, 2, 2024), 1);
}
static void validation_unusual_badFields(void)
{
    CU_ASSERT_EQUAL(validatePhone("98765x3210"), 0);
    CU_ASSERT_EQUAL(validateEmail("ravi.corp.com"), 0);
    CU_ASSERT_EQUAL(validateDate(29, 2, 2025), 0);
}
static void validation_edge_limitsAndNull(void)
{
    CU_ASSERT_EQUAL(validateId(MAX_ID), 1);
    CU_ASSERT_EQUAL(validateId(MAX_ID + 1), 0);
    CU_ASSERT_EQUAL(validateName(NULL), 0);
    CU_ASSERT_EQUAL(validateTimestamp("2100-12-31 23:59:59"), 1);
}

/* ------------------------------------------------------------ logger */
static void logger_usual_timestampedLine(void)
{
    CU_ASSERT_EQUAL(logMessage(LOG_INFO, "CUNIT", "hello"), EMS_OK);
    CU_ASSERT_PTR_NOT_NULL(strstr(logText(), "[INFO] [CUNIT] hello"));
}
static void logger_unusual_logClosed(void)
{
    logClose();
    CU_ASSERT_EQUAL(logMessage(LOG_INFO, "CUNIT", "x"), ERR_FILE);
}
static void logger_edge_errorCodes(void)
{
    CU_ASSERT_STRING_EQUAL(errorCodeText(ERR_INVALID_INPUT), "E001");
    CU_ASSERT_STRING_EQUAL(errorCodeText(ERR_ACCESS_DENIED), "E010");
    CU_ASSERT_STRING_EQUAL(errorCodeText(ERR_ACCESS_DENIED + 1), "E000");
}

/* ------------------------------------------------------------ auth */
static void auth_usual_loginRole(void)
{
    CU_ASSERT_EQUAL(addUserRecord("admin", "admin123", ROLE_ADMIN), EMS_OK);
    CU_ASSERT_EQUAL(verifyCredentials("ADMIN", "admin123"), ROLE_ADMIN);
}
static void auth_unusual_wrongPasswordAndAccess(void)
{
    CU_ASSERT_EQUAL(addUserRecord("hr_user", "secret11", ROLE_OPERATOR), EMS_OK);
    CU_ASSERT_EQUAL(verifyCredentials("hr_user", "secret12"), ROLE_NONE);
    setSession("hr_user", ROLE_OPERATOR);
    CU_ASSERT_EQUAL(checkAccess(ROLE_ADMIN), 0);
}
static void auth_edge_userLimit(void)
{
    char name[16];
    int i;
    for (i = 0; i < MAX_USERS; i++) {
        (void)snprintf(name, sizeof(name), "user%02d", i);
        CU_ASSERT_EQUAL(addUserRecord(name, "secret11", ROLE_OPERATOR), EMS_OK);
    }
    CU_ASSERT_EQUAL(addUserRecord("onemore", "secret11", ROLE_OPERATOR), ERR_LIMIT_REACHED);
}

/* ------------------------------------------------------------ department / role */
static void department_usual_add(void)
{
    CU_ASSERT_EQUAL(departmentAdd(10, "Finance", NULL), EMS_OK);
    CU_ASSERT_STRING_EQUAL(getDeptName(10), "Finance");
    CU_ASSERT_EQUAL(validateTimestamp(getDeptAt(0)->createdAt), 1);
}
static void department_unusual_duplicate(void)
{
    CU_ASSERT_EQUAL(departmentAdd(10, "Finance", NULL), EMS_OK);
    CU_ASSERT_EQUAL(departmentAdd(11, "FINANCE", NULL), ERR_DUPLICATE_ID);
}
static void department_edge_full(void)
{
    char name[16];
    int i;
    for (i = 1; i <= MAX_DEPARTMENTS; i++) {
        (void)snprintf(name, sizeof(name), "Dept %d", i);
        CU_ASSERT_EQUAL(departmentAdd(i, name, NULL), EMS_OK);
    }
    CU_ASSERT_EQUAL(departmentAdd(5000, "Extra", NULL), ERR_LIMIT_REACHED);
}
static void role_usual_add(void)
{
    CU_ASSERT_EQUAL(roleAdd(20, "Accountant", NULL), EMS_OK);
    CU_ASSERT_STRING_EQUAL(getRoleTitle(20), "Accountant");
}
static void role_unusual_badTitle(void)
{
    CU_ASSERT_EQUAL(roleAdd(20, "Acc|ountant", NULL), ERR_INVALID_INPUT);
}
static void role_edge_unknownId(void)
{
    CU_ASSERT_STRING_EQUAL(getRoleTitle(999), "(none)");
}

/* ------------------------------------------------------------ employee */
static void employee_usual_sortedInsert(void)
{
    Employee e = makeEmployee(50);
    CU_ASSERT_EQUAL(employeeInsert(&e), EMS_OK);
    CU_ASSERT_EQUAL(getEmployeeHead()->id, 50);
    CU_ASSERT_EQUAL(getEmployeeCount(), 3);
}
static void employee_unusual_duplicateAndMissingDept(void)
{
    Employee dup = makeEmployee(101);
    Employee bad = makeEmployeeFull(200, "Bad Dept", 99, 20, 1000.0);
    CU_ASSERT_EQUAL(employeeInsert(&dup), ERR_DUPLICATE_ID);
    CU_ASSERT_EQUAL(employeeInsert(&bad), ERR_DEPT_ROLE_NOT_FOUND);
}
static void employee_edge_removeAllNodes(void)
{
    CU_ASSERT_EQUAL(employeeRemove(101), EMS_OK);
    CU_ASSERT_EQUAL(employeeRemove(102), EMS_OK);
    CU_ASSERT_PTR_NULL(getEmployeeHead());
    CU_ASSERT_EQUAL(employeeRemove(101), ERR_NOT_FOUND);
}

/* ------------------------------------------------------------ searching */
static void searching_usual_byName(void)
{
    CU_ASSERT_EQUAL(employeeSearch(SEARCH_BY_NAME, 0, "priya", NULL), 1);
}
static void searching_unusual_noMatch(void)
{
    CU_ASSERT_EQUAL(employeeSearch(SEARCH_BY_DEPT, 99, NULL, NULL), 0);
}
static void searching_edge_emptyKeyMatchesAll(void)
{
    CU_ASSERT_EQUAL(employeeSearch(SEARCH_BY_NAME, 0, "", NULL), 2);
}

/* ------------------------------------------------------------ attendance */
static void attendance_usual_record(void)
{
    Date d = makeDate(5, 3, 2025);
    CU_ASSERT_EQUAL(attendanceAdd(101, &d, 'P', NULL), EMS_OK);
    CU_ASSERT_EQUAL(validateTimestamp(getAttendanceAt(0)->recordedAt), 1);
}
static void attendance_unusual_duplicateAndFuture(void)
{
    Date d = makeDate(5, 3, 2025);
    Date future = makeDate(1, 1, MAX_YEAR);
    CU_ASSERT_EQUAL(attendanceAdd(101, &d, 'P', NULL), EMS_OK);
    CU_ASSERT_EQUAL(attendanceAdd(101, &d, 'A', NULL), ERR_INVALID_ATTENDANCE);
    CU_ASSERT_EQUAL(attendanceAdd(101, &future, 'P', NULL), ERR_INVALID_ATTENDANCE);
}
static void attendance_edge_leapDayAndSummary(void)
{
    Date leap = makeDate(29, 2, 2024);
    int p = 0;
    int a = 0;
    int l = 0;
    CU_ASSERT_EQUAL(attendanceAdd(102, &leap, 'L', NULL), EMS_OK);
    getMonthlySummary(102, 2, 2024, &p, &a, &l);
    CU_ASSERT_EQUAL(p + a, 0);
    CU_ASSERT_EQUAL(l, 1);
}

/* ------------------------------------------------------------ payroll */
static void payroll_usual_netWithLop(void)
{
    Date d = makeDate(3, 3, 2025);
    Payroll pay;
    CU_ASSERT_EQUAL(attendanceAdd(101, &d, 'A', NULL), EMS_OK);
    CU_ASSERT_EQUAL(payrollCompute(101, 3, 2025, 5000.0, 2000.0, &pay), EMS_OK);
    CU_ASSERT_DOUBLE_EQUAL(pay.salary.lop, 1000.0, 0.001);
    CU_ASSERT_DOUBLE_EQUAL(pay.salary.net, 32000.0, 0.001);
}
static void payroll_unusual_secondRunSameMonth(void)
{
    Payroll pay;
    CU_ASSERT_EQUAL(payrollCompute(101, 3, 2025, 0.0, 0.0, &pay), EMS_OK);
    CU_ASSERT_EQUAL(payrollAdd(&pay), EMS_OK);
    CU_ASSERT_EQUAL(payrollCompute(101, 3, 2025, 0.0, 0.0, &pay), ERR_PAYROLL_EXISTS);
}
static void payroll_edge_lopCapAndZeroNet(void)
{
    CU_ASSERT_DOUBLE_EQUAL(calculateLop(30000.0, 31), 30000.0, 0.001);
    CU_ASSERT_DOUBLE_EQUAL(calculateNet(100.0, 500.0, 0.0), 0.0, 0.001);
}

/* ------------------------------------------------------------ report */
static void report_usual_employeeReport(void)
{
    CU_ASSERT_EQUAL(generateEmployeeReport(getOutputStream()), 2);
    CU_ASSERT_PTR_NOT_NULL(strstr(getOutput(), "Total Employees : 2"));
}
static void report_unusual_operatorPayrollDenied(void)
{
    setSession("hr_user", ROLE_OPERATOR);
    CU_ASSERT_EQUAL(generateReport(REPORT_PAYROLL), ERR_ACCESS_DENIED);
}
static void report_edge_nullStream(void)
{
    CU_ASSERT_EQUAL(generatePayrollReport(NULL, 3, 2025), 0);
}

/* ------------------------------------------------------------ file manager */
static void file_usual_roundTrip(void)
{
    CU_ASSERT_EQUAL(saveAllRecords(), EMS_OK);
    freeAllMemory();
    CU_ASSERT_EQUAL(loadAllRecords(), EMS_OK);
    CU_ASSERT_EQUAL(getEmployeeCount(), 2);
}
static void file_unusual_damagedLines(void)
{
    writeFile("departments.txt", "garbage\n10|Finance|2025-01-01 09:00:00\n");
    deptReset();
    CU_ASSERT_EQUAL(loadDepartments(), EMS_OK);
    CU_ASSERT_EQUAL(getDeptCount(), 1);
    CU_ASSERT_EQUAL(fileExists("departments.txt.bak"), 1);
}
static void file_edge_missingFolder(void)
{
    CU_ASSERT_EQUAL(setDataDirectory("/no/such/folder"), EMS_OK);
    CU_ASSERT_EQUAL(saveAllRecords(), ERR_FILE);
}

/* ------------------------------------------------------------ io + app */
static void io_usual_readNumber(void)
{
    int v = 0;
    feed("42\n");
    CU_ASSERT_EQUAL(readInt("", 1, 100, &v), EMS_OK);
    CU_ASSERT_EQUAL(v, 42);
}
static void io_unusual_retryOnText(void)
{
    int v = 0;
    feed("abc\n7\n");
    CU_ASSERT_EQUAL(readInt("", 1, 100, &v), EMS_OK);
    CU_ASSERT_EQUAL(v, 7);
}
static void io_edge_endOfInput(void)
{
    char buf[8];
    feed("");
    CU_ASSERT_EQUAL(readLine("", buf, sizeof(buf)), ERR_END_OF_INPUT);
}
static void app_usual_fullSession(void)
{
    feed("boss\nadmin123\nadmin123\nboss\nadmin123\n2\n1\n10\nFinance\n0\n7\n");
    CU_ASSERT_EQUAL(runApplication(testDir(), testPath("test.log")), EXIT_SUCCESS);
    CU_ASSERT_PTR_NOT_NULL(strstr(readFile("departments.txt"), "10|Finance|"));
}
static void app_unusual_loginFails(void)
{
    feed("boss\nadmin123\nadmin123\nboss\nx\nboss\ny\nboss\nz\n");
    CU_ASSERT_EQUAL(runApplication(testDir(), testPath("test.log")), EXIT_FAILURE);
}
static void app_edge_inputClosed(void)
{
    feed("");
    CU_ASSERT_EQUAL(runApplication(testDir(), testPath("test.log")), EXIT_FAILURE);
}

typedef struct {
    const char *name;
    CU_TestFunc fn;
} TestEntry;

static int addSuite(const char *name, int seeded, const TestEntry *tests, int count)
{
    int i;
    int status = 0;
    CU_pSuite suite = CU_add_suite_with_setup_and_teardown(name, suiteSetUp, suiteTearDown,
                                                          (seeded != 0) ? setUpEachSeeded : setUpEach,
                                                          tearDownEach);
    if (suite == NULL) {
        status = -1;
    }
    for (i = 0; (i < count) && (status == 0); i++) {
        if (CU_add_test(suite, tests[i].name, tests[i].fn) == NULL) {
            status = -1;
        }
    }
    return status;
}

#define COUNT(a) ((int)(sizeof(a) / sizeof((a)[0])))

int main(void)
{
    static const TestEntry validation[] = {{"usual: valid fields", validation_usual_validFields},
        {"unusual: bad fields", validation_unusual_badFields}, {"edge: limits and NULL", validation_edge_limitsAndNull}};
    static const TestEntry logger[] = {{"usual: timestamped line", logger_usual_timestampedLine},
        {"unusual: log closed", logger_unusual_logClosed}, {"edge: E001 and E010", logger_edge_errorCodes}};
    static const TestEntry auth[] = {{"usual: login role", auth_usual_loginRole},
        {"unusual: wrong password, access denied", auth_unusual_wrongPasswordAndAccess}, {"edge: user limit", auth_edge_userLimit}};
    static const TestEntry dept[] = {{"usual: add", department_usual_add},
        {"unusual: duplicate name", department_unusual_duplicate}, {"edge: table full", department_edge_full}};
    static const TestEntry role[] = {{"usual: add", role_usual_add},
        {"unusual: '|' in title", role_unusual_badTitle}, {"edge: unknown id", role_edge_unknownId}};
    static const TestEntry emp[] = {{"usual: sorted insert", employee_usual_sortedInsert},
        {"unusual: duplicate, missing dept", employee_unusual_duplicateAndMissingDept}, {"edge: remove all nodes", employee_edge_removeAllNodes}};
    static const TestEntry search[] = {{"usual: by name", searching_usual_byName},
        {"unusual: no match", searching_unusual_noMatch}, {"edge: empty key", searching_edge_emptyKeyMatchesAll}};
    static const TestEntry att[] = {{"usual: record", attendance_usual_record},
        {"unusual: duplicate and future", attendance_unusual_duplicateAndFuture}, {"edge: leap day", attendance_edge_leapDayAndSummary}};
    static const TestEntry pay[] = {{"usual: net with LOP", payroll_usual_netWithLop},
        {"unusual: same month twice", payroll_unusual_secondRunSameMonth}, {"edge: LOP cap, zero net", payroll_edge_lopCapAndZeroNet}};
    static const TestEntry rep[] = {{"usual: employee report", report_usual_employeeReport},
        {"unusual: operator payroll denied", report_unusual_operatorPayrollDenied}, {"edge: NULL stream", report_edge_nullStream}};
    static const TestEntry file[] = {{"usual: save and reload", file_usual_roundTrip},
        {"unusual: damaged lines", file_unusual_damagedLines}, {"edge: missing folder", file_edge_missingFolder}};
    static const TestEntry io[] = {{"usual: read number", io_usual_readNumber},
        {"unusual: retry on text", io_unusual_retryOnText}, {"edge: end of input", io_edge_endOfInput}};
    static const TestEntry app[] = {{"usual: full session", app_usual_fullSession},
        {"unusual: login fails 3 times", app_unusual_loginFails}, {"edge: input closed", app_edge_inputClosed}};
    unsigned int failures;
    int status = 0;

    if (CU_initialize_registry() != CUE_SUCCESS) {
        return (int)CU_get_error();
    }
    status |= addSuite("Validation", 0, validation, COUNT(validation));
    status |= addSuite("Logger", 0, logger, COUNT(logger));
    status |= addSuite("Session & Auth", 0, auth, COUNT(auth));
    status |= addSuite("Department", 0, dept, COUNT(dept));
    status |= addSuite("Role", 0, role, COUNT(role));
    status |= addSuite("Employee", 1, emp, COUNT(emp));
    status |= addSuite("Searching", 1, search, COUNT(search));
    status |= addSuite("Attendance", 1, att, COUNT(att));
    status |= addSuite("Payroll", 1, pay, COUNT(pay));
    status |= addSuite("Report", 1, rep, COUNT(rep));
    status |= addSuite("File Manager", 1, file, COUNT(file));
    status |= addSuite("Console I/O", 0, io, COUNT(io));
    status |= addSuite("Application", 0, app, COUNT(app));
    if (status != 0) {
        CU_cleanup_registry();
        return (int)CU_get_error();
    }
    CU_basic_set_mode(CU_BRM_VERBOSE);
    (void)CU_basic_run_tests();
    failures = CU_get_number_of_failures();
    CU_cleanup_registry();
    return (failures == 0U) ? 0 : 1;
}
