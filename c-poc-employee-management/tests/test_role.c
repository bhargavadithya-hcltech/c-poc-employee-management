/** @file test_role.c  Unit tests for the Role module. */
#include "test_framework.h"



static void roleReset_EmptiesTable(void)
{
    ASSERT_EQ(roleAdd(10, "Accountant", NULL), EMS_OK);
    roleReset();
    EXPECT_EQ(getRoleCount(), 0);
    EXPECT_EQ(findRole(10), -1);
}

static void roleAdd_Valid(void)
{
    EXPECT_EQ(roleAdd(10, "Accountant", NULL), EMS_OK);
    EXPECT_EQ(getRoleCount(), 1);
    EXPECT_STREQ(getRoleAt(0)->title, "Accountant");
}

static void roleAdd_InvalidIdOrName(void)
{
    EXPECT_EQ(roleAdd(0, "Accountant", NULL), ERR_INVALID_INPUT);
    EXPECT_EQ(roleAdd(MAX_ID + 1, "Accountant", NULL), ERR_INVALID_INPUT);
    EXPECT_EQ(roleAdd(10, "", NULL), ERR_INVALID_INPUT);
    EXPECT_EQ(roleAdd(10, "Acc|ountant", NULL), ERR_INVALID_INPUT);
    EXPECT_EQ(roleAdd(10, NULL, NULL), ERR_INVALID_INPUT);
    EXPECT_EQ(roleAdd(10, repeatChar(TITLE_SIZE, 'x'), NULL), ERR_INVALID_INPUT);
    EXPECT_EQ(roleAdd(10, repeatChar(TITLE_SIZE - 1, 'x'), NULL), EMS_OK);
}

static void roleAdd_DuplicateIdOrName(void)
{
    ASSERT_EQ(roleAdd(10, "Accountant", NULL), EMS_OK);
    EXPECT_EQ(roleAdd(10, "Clerk", NULL), ERR_DUPLICATE_ID);
    EXPECT_EQ(roleAdd(11, "ACCOUNTANT", NULL), ERR_DUPLICATE_ID);
}

static void roleAdd_LimitReached(void)
{
    char name[16];
    for (int i = 1; i <= MAX_ROLES; i++) {
        snprintf(name, sizeof(name), "Role %d", i);
        ASSERT_EQ(roleAdd(i, name, NULL), EMS_OK);
    }
    EXPECT_EQ(roleAdd(5000, "Extra", NULL), ERR_LIMIT_REACHED);
}

static void findRole_FoundAndMissing(void)
{
    ASSERT_EQ(roleAdd(10, "Accountant", NULL), EMS_OK);
    ASSERT_EQ(roleAdd(11, "HR", NULL), EMS_OK);
    EXPECT_EQ(findRole(11), 1);
    EXPECT_EQ(findRole(99), -1);
}

static void findRoleByTitle_CaseInsensitive(void)
{
    ASSERT_EQ(roleAdd(10, "Accountant", NULL), EMS_OK);
    EXPECT_EQ(findRoleByTitle("accountant"), 0);
    EXPECT_EQ(findRoleByTitle("Acc"), -1);
}

static void getRoleTitle_KnownAndUnknown(void)
{
    ASSERT_EQ(roleAdd(10, "Accountant", NULL), EMS_OK);
    EXPECT_STREQ(getRoleTitle(10), "Accountant");
    EXPECT_STREQ(getRoleTitle(99), "(none)");
}

static void getRoleCount_Counts(void)
{
    EXPECT_EQ(getRoleCount(), 0);
    ASSERT_EQ(roleAdd(10, "Accountant", NULL), EMS_OK);
    EXPECT_EQ(getRoleCount(), 1);
}

static void getRoleAt_Bounds(void)
{
    ASSERT_EQ(roleAdd(10, "Accountant", NULL), EMS_OK);
    EXPECT_NOT_NULL(getRoleAt(0));
    EXPECT_NULL(getRoleAt(1));
    EXPECT_NULL(getRoleAt(-1));
}

static void viewRoles_EmptyMessage(void)
{
    EXPECT_EQ(viewRoles(), 0);
    EXPECT_CONTAINS(getOutput(), "No roles yet.");
}

static void viewRoles_ListsWithEmployeeCount(void)
{
    seed();
    EXPECT_EQ(viewRoles(), 2);
    const char *out = getOutput();
    EXPECT_CONTAINS(out, "Accountant");
    EXPECT_CONTAINS(out, "Executive");
}

static void createRole_AddsSavesAndLogs(void)
{
    feed("10\nAccountant\n");
    EXPECT_EQ(createRole(), EMS_OK);
    EXPECT_CONTAINS(readFile("roles.txt"), "10|Accountant|");
    EXPECT_EQ(validateTimestamp(getRoleAt(0)->createdAt), 1);
    EXPECT_CONTAINS(logText(), "[ROLE] Role 10 (Accountant) defined by admin");
}

static void createRole_DuplicateShowsE002(void)
{
    ASSERT_EQ(roleAdd(10, "Accountant", NULL), EMS_OK);
    feed("10\nClerk\n");
    EXPECT_EQ(createRole(), ERR_DUPLICATE_ID);
    EXPECT_CONTAINS(getOutput(), "E002");
}

static void createRole_LimitReached(void)
{
    char name[16];
    for (int i = 1; i <= MAX_ROLES; i++) {
        snprintf(name, sizeof(name), "Role %d", i);
        ASSERT_EQ(roleAdd(i, name, NULL), EMS_OK);
    }
    feed("5000\nExtra\n");
    EXPECT_EQ(createRole(), ERR_LIMIT_REACHED);
    EXPECT_CONTAINS(getOutput(), "Role limit of 100 reached.");
}

static void createRole_EndOfInput(void)
{
    feed("10\n");
    EXPECT_EQ(createRole(), ERR_END_OF_INPUT);
    EXPECT_EQ(getRoleCount(), 0);
}

static void roleMenu_CreateViewBack(void)
{
    feed("1\n10\nAccountant\n2\n9\n0\n");
    roleMenu();
    EXPECT_EQ(getRoleCount(), 1);
    const char *out = getOutput();
    EXPECT_CONTAINS(out, "Role defined.");
    EXPECT_CONTAINS(out, "Enter a number from 0 to 2.");
}

static void roleMenu_EndOfInputInsideCreate(void)
{
    feed("1\n10\n");
    roleMenu();
    EXPECT_EQ(getRoleCount(), 0);
}

static void roleAdd_TimestampFromCallerOrNow(void)
{
    EXPECT_EQ(roleAdd(20, "Accountant", "2025-01-01 09:00:00"), EMS_OK);
    EXPECT_STREQ(getRoleAt(0)->createdAt, "2025-01-01 09:00:00");
    EXPECT_EQ(roleAdd(21, "Executive", "bad time"), ERR_INVALID_INPUT);
    EXPECT_EQ(getRoleCount(), 1);
}

void run_role_tests(void)
{
    RUN_TEST(roleReset_EmptiesTable);
    RUN_TEST(roleAdd_Valid);
    RUN_TEST(roleAdd_InvalidIdOrName);
    RUN_TEST(roleAdd_DuplicateIdOrName);
    RUN_TEST(roleAdd_LimitReached);
    RUN_TEST(findRole_FoundAndMissing);
    RUN_TEST(findRoleByTitle_CaseInsensitive);
    RUN_TEST(getRoleTitle_KnownAndUnknown);
    RUN_TEST(getRoleCount_Counts);
    RUN_TEST(getRoleAt_Bounds);
    RUN_TEST(viewRoles_EmptyMessage);
    RUN_TEST(viewRoles_ListsWithEmployeeCount);
    RUN_TEST(createRole_AddsSavesAndLogs);
    RUN_TEST(createRole_DuplicateShowsE002);
    RUN_TEST(createRole_LimitReached);
    RUN_TEST(createRole_EndOfInput);
    RUN_TEST(roleMenu_CreateViewBack);
    RUN_TEST(roleMenu_EndOfInputInsideCreate);
    RUN_TEST(roleAdd_TimestampFromCallerOrNow);
}
