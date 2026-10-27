/** @file test_filemanager.c  Unit tests for the File Management module. */
#include "test_framework.h"



static void setDataDirectory_ValidAndInvalid(void)
{
    EXPECT_EQ(setDataDirectory("somewhere"), EMS_OK);
    EXPECT_STREQ(getDataDirectory(), "somewhere");
    EXPECT_EQ(setDataDirectory(NULL), ERR_INVALID_INPUT);
    EXPECT_EQ(setDataDirectory(""), ERR_INVALID_INPUT);
    EXPECT_EQ(setDataDirectory(repeatChar(PATH_SIZE, 'x')), ERR_INVALID_INPUT);
    EXPECT_STREQ(getDataDirectory(), "somewhere");
}

static void getDataDirectory_ReturnsCurrent(void)
{
    EXPECT_STREQ(getDataDirectory(), testDir());
}

static void buildDataPath_JoinsFolderAndName(void)
{
    char buf[PATH_SIZE];
    EXPECT_EQ(buildDataPath("x.txt", buf, sizeof(buf)), EMS_OK);
    EXPECT_STREQ(buf, testPath("x.txt"));
}

static void buildDataPath_RejectsNullAndSmallBuffer(void)
{
    char small[8];
    EXPECT_EQ(buildDataPath(NULL, small, sizeof(small)), ERR_INVALID_INPUT);
    EXPECT_EQ(buildDataPath("x", NULL, 10), ERR_INVALID_INPUT);
    EXPECT_EQ(buildDataPath("x", small, 0), ERR_INVALID_INPUT);
    EXPECT_EQ(buildDataPath("x.txt", small, sizeof(small)), ERR_INVALID_INPUT);
}

static void beginSave_CreatesTempFile(void)
{
    char tmp[PATH_SIZE];
    FILE *fp = beginSave("a.txt", tmp, sizeof(tmp));
    ASSERT_NOT_NULL(fp);
    EXPECT_STREQ(tmp, testPath("a.txt.tmp"));
    fclose(fp);
    EXPECT_TRUE(fileExists("a.txt.tmp"));
}

static void beginSave_FailsForMissingFolderOrNull(void)
{
    char tmp[PATH_SIZE];
    char small[4];
    EXPECT_NULL(beginSave("a.txt", NULL, 10));
    EXPECT_NULL(beginSave("a.txt", small, sizeof(small)));
    ASSERT_EQ(setDataDirectory("/no/such/folder"), EMS_OK);
    EXPECT_NULL(beginSave("a.txt", tmp, sizeof(tmp)));
}

static void commitSave_RenamesTempOverReal(void)
{
    char tmp[PATH_SIZE];
    writeFile("a.txt", "old\n");
    FILE *fp = beginSave("a.txt", tmp, sizeof(tmp));
    ASSERT_NOT_NULL(fp);
    fputs("new\n", fp);
    EXPECT_EQ(commitSave(fp, "a.txt", tmp), EMS_OK);
    EXPECT_STREQ(readFile("a.txt"), "new\n");
    EXPECT_FALSE(fileExists("a.txt.tmp"));
}

static void commitSave_NullFileReportsE007(void)
{
    EXPECT_EQ(commitSave(NULL, "a.txt", "x"), ERR_FILE);
    EXPECT_CONTAINS(getOutput(), "E007");
    EXPECT_CONTAINS(logText(), "could not save a.txt");
}

static void commitSave_NullFileNameRemovesTemp(void)
{
    char tmp[PATH_SIZE];
    FILE *fp = beginSave("b.txt", tmp, sizeof(tmp));
    ASSERT_NOT_NULL(fp);
    EXPECT_EQ(commitSave(fp, NULL, tmp), ERR_FILE);
    EXPECT_FALSE(fileExists("b.txt.tmp"));
}

static void backupFile_CopiesFile(void)
{
    writeFile("data.txt", "line1\nline2\n");
    EXPECT_EQ(backupFile("data.txt"), EMS_OK);
    EXPECT_STREQ(readFile("data.txt.bak"), "line1\nline2\n");
}

static void backupFile_MissingFile(void)
{
    EXPECT_EQ(backupFile("none.txt"), ERR_FILE);
    EXPECT_EQ(backupFile(NULL), ERR_INVALID_INPUT);
}

static void reportBadLines_WarnsAndBacksUp(void)
{
    writeFile("x.txt", "bad\n");
    reportBadLines("x.txt", 0);
    EXPECT_FALSE(fileExists("x.txt.bak"));
    reportBadLines("x.txt", 1);
    EXPECT_TRUE(fileExists("x.txt.bak"));
    EXPECT_CONTAINS(getOutput(), "Warning: 1 damaged line(s) skipped in x.txt");
    EXPECT_CONTAINS(logText(), "[WARN] [FILE]");
}

static void openForLoad_MissingFileIsNotAnError(void)
{
    int status = -1;
    EXPECT_NULL(openForLoad("missing.txt", &status));
    EXPECT_EQ(status, EMS_OK);
    EXPECT_CONTAINS(logText(), "missing.txt not found");
}

static void openForLoad_ExistingFile(void)
{
    int status = -1;
    writeFile("here.txt", "x\n");
    FILE *fp = openForLoad("here.txt", &status);
    ASSERT_NOT_NULL(fp);
    fclose(fp);
    EXPECT_EQ(status, EMS_OK);
}

static void loadDepartments_ValidAndDamaged(void)
{
    writeFile("departments.txt", "10|Finance|2025-01-01 09:00:00\nxx\n11|HR|2025-01-01 09:00:00\n10|Duplicate|2025-01-01 09:00:00\n12|Sales|not-a-time\n");
    EXPECT_EQ(loadDepartments(), EMS_OK);
    EXPECT_EQ(getDeptCount(), 2);
    EXPECT_STREQ(getDeptAt(0)->createdAt, "2025-01-01 09:00:00");
    EXPECT_TRUE(fileExists("departments.txt.bak"));
}

static void saveDepartments_RoundTrip(void)
{
    ASSERT_EQ(departmentAdd(10, "Finance", NULL), EMS_OK);
    EXPECT_EQ(saveDepartments(), EMS_OK);
    deptReset();
    EXPECT_EQ(loadDepartments(), EMS_OK);
    EXPECT_STREQ(getDeptName(10), "Finance");
}

static void loadRoles_ValidAndDamaged(void)
{
    writeFile("roles.txt", "20|Accountant|2025-01-01 09:00:00\n|\n");
    EXPECT_EQ(loadRoles(), EMS_OK);
    EXPECT_EQ(getRoleCount(), 1);
}

static void saveRoles_RoundTrip(void)
{
    ASSERT_EQ(roleAdd(20, "Accountant", NULL), EMS_OK);
    EXPECT_EQ(saveRoles(), EMS_OK);
    EXPECT_CONTAINS(readFile("roles.txt"), "20|Accountant|");
}

static void loadEmployees_SkipsUnknownDepartmentAndBadLines(void)
{
    ASSERT_EQ(departmentAdd(10, "Finance", NULL), EMS_OK);
    ASSERT_EQ(roleAdd(20, "Accountant", NULL), EMS_OK);
    writeFile("employees.txt",
              "101|Ravi Kumar|9876543210|ravi@corp.com|10|20|30000.00|2025-01-01 09:00:00|2025-01-01 09:00:00\n"
              "102|Priya|9876543210|p@corp.com|99|20|20000.00|2025-01-01 09:00:00|2025-01-01 09:00:00\n"
              "103|Old Format|9876543210|o@corp.com|10|20|20000.00\n"
              "junk\n");
    EXPECT_EQ(loadEmployees(), EMS_OK);
    EXPECT_EQ(getEmployeeCount(), 1);
    EXPECT_NOT_NULL(findEmployee(101));
    EXPECT_STREQ(findEmployee(101)->createdAt, "2025-01-01 09:00:00");
}

static void saveEmployees_RoundTrip(void)
{
    seed();
    EXPECT_EQ(saveEmployees(), EMS_OK);
    employeeReset();
    EXPECT_EQ(loadEmployees(), EMS_OK);
    EXPECT_EQ(getEmployeeCount(), 2);
    EXPECT_STREQ(findEmployee(102)->email, "ravi@corp.com");
}

static void loadAttendance_SkipsUnknownEmployee(void)
{
    seed();
    writeFile("attendance.txt", "101|1|3|2025|P|2025-01-01 09:00:00\n555|1|3|2025|P|2025-01-01 09:00:00\n101|2|3|2025|X|2025-01-01 09:00:00\n"
                                "101|3|3|2025|P|2025-13-01 09:00:00\nbad\n");
    EXPECT_EQ(loadAttendance(), EMS_OK);
    EXPECT_EQ(getAttendanceCount(), 1);
}

static void saveAttendance_RoundTrip(void)
{
    seed();
    Date d = makeDate(1, 3, 2025);
    ASSERT_EQ(attendanceAdd(102, &d, 'L', NULL), EMS_OK);
    EXPECT_EQ(saveAttendance(), EMS_OK);
    EXPECT_CONTAINS(readFile("attendance.txt"), "102|1|3|2025|L|");
}

static void loadPayroll_SkipsDuplicateAndBadLines(void)
{
    seed();
    writeFile("payroll.txt",
              "101|3|2025|30000.00|0.00|0.00|0|0.00|30000.00|30000.00|2025-01-01 09:00:00\n"
              "101|3|2025|30000.00|0.00|0.00|0|0.00|30000.00|30000.00|2025-01-01 09:00:00\n"
              "101|4|2025|30000.00|0.00|0.00|0|0.00|30000.00|30000.00|yesterday\n"
              "short line\n");
    EXPECT_EQ(loadPayroll(), EMS_OK);
    EXPECT_EQ(getPayrollCount(), 1);
    EXPECT_STREQ(getPayrollAt(0)->processedAt, "2025-01-01 09:00:00");
}

static void savePayroll_RoundTrip(void)
{
    seed();
    Payroll p;
    ASSERT_EQ(payrollCompute(101, 3, 2025, 100.0, 50.0, &p), EMS_OK);
    ASSERT_EQ(payrollAdd(&p), EMS_OK);
    EXPECT_EQ(savePayroll(), EMS_OK);
    payrollReset();
    EXPECT_EQ(loadPayroll(), EMS_OK);
    EXPECT_DBL_EQ(getPayrollAt(0)->salary.net, 30050.0);
}

static void loadAllRecords_FullRoundTrip(void)
{
    seed();
    Date d = makeDate(1, 3, 2025);
    ASSERT_EQ(attendanceAdd(101, &d, 'A', NULL), EMS_OK);
    Payroll p;
    ASSERT_EQ(payrollCompute(101, 3, 2025, 0.0, 0.0, &p), EMS_OK);
    ASSERT_EQ(payrollAdd(&p), EMS_OK);
    ASSERT_EQ(saveAllRecords(), EMS_OK);
    freeAllMemory();
    EXPECT_EQ(loadAllRecords(), EMS_OK);
    EXPECT_EQ(getDeptCount(), 2);
    EXPECT_EQ(getRoleCount(), 2);
    EXPECT_EQ(getEmployeeCount(), 2);
    EXPECT_EQ(getAttendanceCount(), 1);
    EXPECT_EQ(getPayrollCount(), 1);
    EXPECT_CONTAINS(logText(), "All records loaded");
}

static void loadAllRecords_EmptyFolderStartsEmpty(void)
{
    EXPECT_EQ(loadAllRecords(), EMS_OK);
    EXPECT_EQ(getEmployeeCount(), 0);
}

static void saveAllRecords_FailsWhenFolderMissing(void)
{
    ASSERT_EQ(setDataDirectory("/no/such/folder"), EMS_OK);
    EXPECT_EQ(saveAllRecords(), ERR_FILE);
}

static void freeAllMemory_EmptiesEverything(void)
{
    seed();
    ASSERT_EQ(addUserRecord("admin", "admin123", ROLE_ADMIN), EMS_OK);
    freeAllMemory();
    EXPECT_EQ(getEmployeeCount(), 0);
    EXPECT_EQ(getDeptCount(), 0);
    EXPECT_EQ(getRoleCount(), 0);
    EXPECT_EQ(getUserCount(), 0);
    EXPECT_EQ(getAttendanceCount(), 0);
    EXPECT_EQ(getPayrollCount(), 0);
}

void run_filemanager_tests(void)
{
    RUN_TEST(setDataDirectory_ValidAndInvalid);
    RUN_TEST(getDataDirectory_ReturnsCurrent);
    RUN_TEST(buildDataPath_JoinsFolderAndName);
    RUN_TEST(buildDataPath_RejectsNullAndSmallBuffer);
    RUN_TEST(beginSave_CreatesTempFile);
    RUN_TEST(beginSave_FailsForMissingFolderOrNull);
    RUN_TEST(commitSave_RenamesTempOverReal);
    RUN_TEST(commitSave_NullFileReportsE007);
    RUN_TEST(commitSave_NullFileNameRemovesTemp);
    RUN_TEST(backupFile_CopiesFile);
    RUN_TEST(backupFile_MissingFile);
    RUN_TEST(reportBadLines_WarnsAndBacksUp);
    RUN_TEST(openForLoad_MissingFileIsNotAnError);
    RUN_TEST(openForLoad_ExistingFile);
    RUN_TEST(loadDepartments_ValidAndDamaged);
    RUN_TEST(saveDepartments_RoundTrip);
    RUN_TEST(loadRoles_ValidAndDamaged);
    RUN_TEST(saveRoles_RoundTrip);
    RUN_TEST(loadEmployees_SkipsUnknownDepartmentAndBadLines);
    RUN_TEST(saveEmployees_RoundTrip);
    RUN_TEST(loadAttendance_SkipsUnknownEmployee);
    RUN_TEST(saveAttendance_RoundTrip);
    RUN_TEST(loadPayroll_SkipsDuplicateAndBadLines);
    RUN_TEST(savePayroll_RoundTrip);
    RUN_TEST(loadAllRecords_FullRoundTrip);
    RUN_TEST(loadAllRecords_EmptyFolderStartsEmpty);
    RUN_TEST(saveAllRecords_FailsWhenFolderMissing);
    RUN_TEST(freeAllMemory_EmptiesEverything);
}
