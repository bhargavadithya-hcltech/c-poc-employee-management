/** @file test_app.c  Unit and end-to-end tests for the application layer. */
#include "test_framework.h"

static void writeAdmin(void)
{
    char text[256];

    (void)snprintf(text, sizeof(text), "admin|%lu|1|2025-01-01 09:00:00\nhr_user|%lu|2|2025-01-01 09:00:00\n",
                   hashPassword("admin", "admin123"), hashPassword("hr_user", "secret11"));
    writeFile("users.txt", text);
}


static void systemInit_LoadsUsersAndOpensLog(void)
{
    writeAdmin();
    EXPECT_EQ(systemInit(testDir(), testPath("test.log")), EMS_OK);
    EXPECT_EQ(getUserCount(), 2);
    EXPECT_EQ(logIsOpen(), 1);
    EXPECT_CONTAINS(logText(), "System started");
}

static void systemInit_BadLogPathStillRuns(void)
{
    EXPECT_EQ(systemInit(testDir(), "/no/such/folder/x.log"), EMS_OK);
    EXPECT_CONTAINS(getOutput(), "log file could not be opened");
}

static void systemInit_BadDataFolderName(void)
{
    EXPECT_EQ(systemInit("", testPath("test.log")), ERR_INVALID_INPUT);
}

static void systemCleanup_FreesAndClosesLog(void)
{
    seed();
    systemCleanup();
    EXPECT_EQ(getEmployeeCount(), 0);
    EXPECT_EQ(logIsOpen(), 0);
    EXPECT_CONTAINS(logText(), "System stopped, memory released");
}

static void displayMainMenu_ShowsUserAndOptions(void)
{
    seed();
    displayMainMenu();
    const char *out = getOutput();
    EXPECT_CONTAINS(out, "Logged in as : admin (Administrator)");
    EXPECT_CONTAINS(out, "Employees: 2 | Departments: 2 | Roles: 2");
    EXPECT_CONTAINS(out, "7. Save and Exit");
}

static void mainMenu_AdminVisitsEveryMenu(void)
{
    feed("1\n0\n2\n0\n3\n0\n4\n0\n5\n0\n6\n0\n8\nhr_two\nsecret11\nsecret11\n2\n7\n");
    EXPECT_EQ(mainMenu(), EMS_OK);
    EXPECT_GE(findUser("hr_two"), 0);
}

static void mainMenu_OperatorBlockedFromAdminMenus(void)
{
    setSession("hr_user", ROLE_OPERATOR);
    feed("2\n3\n5\n8\n7\n");
    EXPECT_EQ(mainMenu(), EMS_OK);
    EXPECT_EQ(countOccurrences(getOutput(), "E010"), 4);
}

static void mainMenu_EndOfInput(void)
{
    feed("");
    EXPECT_EQ(mainMenu(), ERR_END_OF_INPUT);
}

static void runApplication_FirstRunFullWorkflow(void)
{
    /* first-run admin -> login -> dept -> role -> employee -> attendance -> payroll -> report -> exit */
    feed("boss\nadmin123\nadmin123\n"
         "boss\nadmin123\n"
         "2\n1\n10\nFinance\n0\n"
         "3\n1\n20\nAccountant\n0\n"
         "1\n1\n101\nRavi Kumar\n9876543210\nravi@corp.com\n10\n20\n35000\n0\n"
         "4\n1\n101\n5\n3\n2025\nA\n0\n"
         "5\n1\n101\n3\n2025\n5000\n2000\ny\n0\n"
         "6\n3\n3\n2025\n0\n"
         "7\n");
    EXPECT_EQ(runApplication(testDir(), testPath("test.log")), EXIT_SUCCESS);
    EXPECT_CONTAINS(readFile("payroll.txt"), "101|3|2025|35000.00|5000.00|2000.00|1|1166.67|40000.00|36833.33|");
    EXPECT_CONTAINS(getOutput(), "Total Net Payout : 36833.33");
    EXPECT_EQ(getEmployeeCount(), 0);       /* memory released at exit */
    const char *logTxt = logText();
    EXPECT_CONTAINS(logTxt, "boss logged in as Administrator");
    EXPECT_CONTAINS(logTxt, "All records saved");
    EXPECT_CONTAINS(logTxt, "System stopped, memory released");
}

static void runApplication_LoginFailureExits(void)
{
    writeAdmin();
    feed("admin\nbad1\nadmin\nbad2\nadmin\nbad3\n");
    EXPECT_EQ(runApplication(testDir(), testPath("test.log")), EXIT_FAILURE);
    EXPECT_CONTAINS(logText(), "too many failed attempts");
}

static void runApplication_EndOfInputSavesSafely(void)
{
    writeAdmin();
    writeFile("departments.txt", "10|Finance|2025-01-01 09:00:00\n");
    feed("admin\nadmin123\n");
    EXPECT_EQ(runApplication(testDir(), testPath("test.log")), EXIT_SUCCESS);
    EXPECT_CONTAINS(getOutput(), "Input closed. Saving and exiting safely.");
    EXPECT_STREQ(readFile("departments.txt"), "10|Finance|2025-01-01 09:00:00\n");
}

static void runApplication_InputClosedDuringFirstRun(void)
{
    feed("boss\n");
    EXPECT_EQ(runApplication(testDir(), testPath("test.log")), EXIT_FAILURE);
    EXPECT_EQ(getUserCount(), 0);
}

void run_app_tests(void)
{
    RUN_TEST(systemInit_LoadsUsersAndOpensLog);
    RUN_TEST(systemInit_BadLogPathStillRuns);
    RUN_TEST(systemInit_BadDataFolderName);
    RUN_TEST(systemCleanup_FreesAndClosesLog);
    RUN_TEST(displayMainMenu_ShowsUserAndOptions);
    RUN_TEST(mainMenu_AdminVisitsEveryMenu);
    RUN_TEST(mainMenu_OperatorBlockedFromAdminMenus);
    RUN_TEST(mainMenu_EndOfInput);
    RUN_TEST(runApplication_FirstRunFullWorkflow);
    RUN_TEST(runApplication_LoginFailureExits);
    RUN_TEST(runApplication_EndOfInputSavesSafely);
    RUN_TEST(runApplication_InputClosedDuringFirstRun);
}
