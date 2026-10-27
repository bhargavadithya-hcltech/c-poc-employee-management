/** @file test_department.c  Unit tests for the Department module. */
#include "test_framework.h"



static void deptReset_EmptiesTable(void)
{
    ASSERT_EQ(departmentAdd(10, "Finance", NULL), EMS_OK);
    deptReset();
    EXPECT_EQ(getDeptCount(), 0);
    EXPECT_EQ(findDept(10), -1);
}

static void departmentAdd_Valid(void)
{
    EXPECT_EQ(departmentAdd(10, "Finance", NULL), EMS_OK);
    EXPECT_EQ(getDeptCount(), 1);
    EXPECT_STREQ(getDeptAt(0)->name, "Finance");
}

static void departmentAdd_InvalidIdOrName(void)
{
    EXPECT_EQ(departmentAdd(0, "Finance", NULL), ERR_INVALID_INPUT);
    EXPECT_EQ(departmentAdd(MAX_ID + 1, "Finance", NULL), ERR_INVALID_INPUT);
    EXPECT_EQ(departmentAdd(10, "", NULL), ERR_INVALID_INPUT);
    EXPECT_EQ(departmentAdd(10, "Fin|ance", NULL), ERR_INVALID_INPUT);
    EXPECT_EQ(departmentAdd(10, NULL, NULL), ERR_INVALID_INPUT);
    EXPECT_EQ(departmentAdd(10, repeatChar(TITLE_SIZE, 'x'), NULL), ERR_INVALID_INPUT);
    EXPECT_EQ(departmentAdd(10, repeatChar(TITLE_SIZE - 1, 'x'), NULL), EMS_OK);
}

static void departmentAdd_DuplicateIdOrName(void)
{
    ASSERT_EQ(departmentAdd(10, "Finance", NULL), EMS_OK);
    EXPECT_EQ(departmentAdd(10, "Sales", NULL), ERR_DUPLICATE_ID);
    EXPECT_EQ(departmentAdd(11, "FINANCE", NULL), ERR_DUPLICATE_ID);
}

static void departmentAdd_LimitReached(void)
{
    char name[16];
    for (int i = 1; i <= MAX_DEPARTMENTS; i++) {
        snprintf(name, sizeof(name), "Dept %d", i);
        ASSERT_EQ(departmentAdd(i, name, NULL), EMS_OK);
    }
    EXPECT_EQ(departmentAdd(5000, "Extra", NULL), ERR_LIMIT_REACHED);
}

static void findDept_FoundAndMissing(void)
{
    ASSERT_EQ(departmentAdd(10, "Finance", NULL), EMS_OK);
    ASSERT_EQ(departmentAdd(11, "HR", NULL), EMS_OK);
    EXPECT_EQ(findDept(11), 1);
    EXPECT_EQ(findDept(99), -1);
}

static void findDeptByName_CaseInsensitive(void)
{
    ASSERT_EQ(departmentAdd(10, "Finance", NULL), EMS_OK);
    EXPECT_EQ(findDeptByName("finance"), 0);
    EXPECT_EQ(findDeptByName("Fin"), -1);
}

static void getDeptName_KnownAndUnknown(void)
{
    ASSERT_EQ(departmentAdd(10, "Finance", NULL), EMS_OK);
    EXPECT_STREQ(getDeptName(10), "Finance");
    EXPECT_STREQ(getDeptName(99), "(none)");
}

static void getDeptCount_Counts(void)
{
    EXPECT_EQ(getDeptCount(), 0);
    ASSERT_EQ(departmentAdd(10, "Finance", NULL), EMS_OK);
    EXPECT_EQ(getDeptCount(), 1);
}

static void getDeptAt_Bounds(void)
{
    ASSERT_EQ(departmentAdd(10, "Finance", NULL), EMS_OK);
    EXPECT_NOT_NULL(getDeptAt(0));
    EXPECT_NULL(getDeptAt(1));
    EXPECT_NULL(getDeptAt(-1));
}

static void viewDepartments_EmptyMessage(void)
{
    EXPECT_EQ(viewDepartments(), 0);
    EXPECT_CONTAINS(getOutput(), "No departments yet.");
}

static void viewDepartments_ListsWithEmployeeCount(void)
{
    seed();
    EXPECT_EQ(viewDepartments(), 2);
    const char *out = getOutput();
    EXPECT_CONTAINS(out, "Finance");
    EXPECT_CONTAINS(out, "HR");
}

static void createDepartment_AddsSavesAndLogs(void)
{
    feed("10\nFinance\n");
    EXPECT_EQ(createDepartment(), EMS_OK);
    EXPECT_CONTAINS(readFile("departments.txt"), "10|Finance|");
    EXPECT_EQ(validateTimestamp(getDeptAt(0)->createdAt), 1);
    EXPECT_CONTAINS(logText(), "[DEPARTMENT] Department 10 (Finance) created by admin");
}

static void createDepartment_DuplicateShowsE002(void)
{
    ASSERT_EQ(departmentAdd(10, "Finance", NULL), EMS_OK);
    feed("10\nSales\n");
    EXPECT_EQ(createDepartment(), ERR_DUPLICATE_ID);
    EXPECT_CONTAINS(getOutput(), "E002");
}

static void createDepartment_LimitReached(void)
{
    char name[16];
    for (int i = 1; i <= MAX_DEPARTMENTS; i++) {
        snprintf(name, sizeof(name), "Dept %d", i);
        ASSERT_EQ(departmentAdd(i, name, NULL), EMS_OK);
    }
    feed("5000\nExtra\n");
    EXPECT_EQ(createDepartment(), ERR_LIMIT_REACHED);
    EXPECT_CONTAINS(getOutput(), "Department limit of 100 reached.");
}

static void createDepartment_EndOfInput(void)
{
    feed("10\n");
    EXPECT_EQ(createDepartment(), ERR_END_OF_INPUT);
    EXPECT_EQ(getDeptCount(), 0);
}

static void departmentMenu_CreateViewBack(void)
{
    feed("1\n10\nFinance\n2\n9\n0\n");
    departmentMenu();
    EXPECT_EQ(getDeptCount(), 1);
    const char *out = getOutput();
    EXPECT_CONTAINS(out, "Department created.");
    EXPECT_CONTAINS(out, "Enter a number from 0 to 2.");
}

static void departmentMenu_EndOfInputInsideCreate(void)
{
    feed("1\n10\n");
    departmentMenu();
    EXPECT_EQ(getDeptCount(), 0);
}

static void departmentAdd_TimestampFromCallerOrNow(void)
{
    EXPECT_EQ(departmentAdd(10, "Finance", "2025-01-01 09:00:00"), EMS_OK);
    EXPECT_STREQ(getDeptAt(0)->createdAt, "2025-01-01 09:00:00");
    EXPECT_EQ(departmentAdd(11, "HR", NULL), EMS_OK);
    EXPECT_EQ(validateTimestamp(getDeptAt(1)->createdAt), 1);
    EXPECT_EQ(departmentAdd(12, "Sales", "bad time"), ERR_INVALID_INPUT);
    EXPECT_EQ(getDeptCount(), 2);
}

void run_department_tests(void)
{
    RUN_TEST(deptReset_EmptiesTable);
    RUN_TEST(departmentAdd_Valid);
    RUN_TEST(departmentAdd_InvalidIdOrName);
    RUN_TEST(departmentAdd_DuplicateIdOrName);
    RUN_TEST(departmentAdd_LimitReached);
    RUN_TEST(findDept_FoundAndMissing);
    RUN_TEST(findDeptByName_CaseInsensitive);
    RUN_TEST(getDeptName_KnownAndUnknown);
    RUN_TEST(getDeptCount_Counts);
    RUN_TEST(getDeptAt_Bounds);
    RUN_TEST(viewDepartments_EmptyMessage);
    RUN_TEST(viewDepartments_ListsWithEmployeeCount);
    RUN_TEST(createDepartment_AddsSavesAndLogs);
    RUN_TEST(createDepartment_DuplicateShowsE002);
    RUN_TEST(createDepartment_LimitReached);
    RUN_TEST(createDepartment_EndOfInput);
    RUN_TEST(departmentMenu_CreateViewBack);
    RUN_TEST(departmentMenu_EndOfInputInsideCreate);
    RUN_TEST(departmentAdd_TimestampFromCallerOrNow);
}
