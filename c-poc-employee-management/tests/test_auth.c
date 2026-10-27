/** @file test_auth.c  Unit tests for the Session & Auth module. */
#include "test_framework.h"



static void authReset_ClearsUsersAndSession(void)
{
    ASSERT_EQ(addUserRecord("admin", "admin123", ROLE_ADMIN), EMS_OK);
    authReset();
    EXPECT_EQ(getUserCount(), 0);
    EXPECT_EQ(getCurrentRole(), ROLE_NONE);
}

static void hashPassword_IsDeterministicAndSalted(void)
{
    EXPECT_EQ(hashPassword("admin", "admin123"), hashPassword("admin", "admin123"));
    EXPECT_EQ(hashPassword("ADMIN", "admin123"), hashPassword("admin", "admin123"));
    EXPECT_NE(hashPassword("admin", "admin123"), hashPassword("admin", "admin124"));
    EXPECT_NE(hashPassword("admin", "secret11"), hashPassword("hr_user", "secret11"));
}

static void hashPassword_FitsIn32BitsAndHandlesNull(void)
{
    EXPECT_LE(hashPassword("someone", repeatChar(29, 'z')), 0xFFFFFFFFUL);
    EXPECT_EQ(hashPassword(NULL, "x"), 5381UL);
    EXPECT_EQ(hashPassword("x", NULL), 5381UL);
}

static void findUser_CaseInsensitive(void)
{
    ASSERT_EQ(addUserRecord("Admin", "admin123", ROLE_ADMIN), EMS_OK);
    EXPECT_EQ(findUser("admin"), 0);
    EXPECT_EQ(findUser("ADMIN"), 0);
    EXPECT_EQ(findUser("nobody"), -1);
    EXPECT_EQ(findUser(NULL), -1);
}

static void addUserRecord_ValidUser(void)
{
    EXPECT_EQ(addUserRecord("hr_user", "secret11", ROLE_OPERATOR), EMS_OK);
    EXPECT_EQ(getUserCount(), 1);
    EXPECT_EQ(getUserAt(0)->role, ROLE_OPERATOR);
}

static void addUserRecord_RejectsBadInput(void)
{
    EXPECT_EQ(addUserRecord("ab", "secret11", ROLE_ADMIN), ERR_INVALID_INPUT);
    EXPECT_EQ(addUserRecord("valid_user", "123", ROLE_ADMIN), ERR_INVALID_INPUT);
    EXPECT_EQ(addUserRecord("valid_user", "secret11", 7), ERR_INVALID_INPUT);
    EXPECT_EQ(getUserCount(), 0);
}

static void addUserRecord_RejectsDuplicate(void)
{
    ASSERT_EQ(addUserRecord("admin", "admin123", ROLE_ADMIN), EMS_OK);
    EXPECT_EQ(addUserRecord("ADMIN", "other123", ROLE_OPERATOR), ERR_DUPLICATE_ID);
}

static void addUserRecord_LimitReached(void)
{
    char name[16];
    for (int i = 0; i < MAX_USERS; i++) {
        snprintf(name, sizeof(name), "user%02d", i);
        ASSERT_EQ(addUserRecord(name, "secret11", ROLE_OPERATOR), EMS_OK);
    }
    EXPECT_EQ(addUserRecord("oneMore", "secret11", ROLE_OPERATOR), ERR_LIMIT_REACHED);
}

static void addUserHashed_StoresGivenHash(void)
{
    EXPECT_EQ(addUserHashed("loaded", 12345UL, ROLE_ADMIN, NULL), EMS_OK);
    EXPECT_EQ(getUserAt(0)->passwordHash, 12345UL);
    EXPECT_EQ(addUserHashed("x", 1UL, ROLE_ADMIN, NULL), ERR_INVALID_INPUT);
}

static void verifyCredentials_RightAndWrongPassword(void)
{
    ASSERT_EQ(addUserRecord("admin", "admin123", ROLE_ADMIN), EMS_OK);
    ASSERT_EQ(addUserRecord("hr_user", "secret11", ROLE_OPERATOR), EMS_OK);
    EXPECT_EQ(verifyCredentials("admin", "admin123"), ROLE_ADMIN);
    EXPECT_EQ(verifyCredentials("HR_USER", "secret11"), ROLE_OPERATOR);
    EXPECT_EQ(verifyCredentials("admin", "Admin123"), ROLE_NONE);
    EXPECT_EQ(verifyCredentials("ghost", "admin123"), ROLE_NONE);
    EXPECT_EQ(verifyCredentials("admin", NULL), ROLE_NONE);
}

static void setSession_ValidAndInvalidRole(void)
{
    setSession("hr_user", ROLE_OPERATOR);
    EXPECT_STREQ(getCurrentUser(), "hr_user");
    EXPECT_EQ(getCurrentRole(), ROLE_OPERATOR);
    setSession("x", 9);
    EXPECT_EQ(getCurrentRole(), ROLE_NONE);
    setSession(NULL, ROLE_ADMIN);
    EXPECT_STREQ(getCurrentUser(), "-");
}

static void logout_ClearsSession(void)
{
    logout();
    EXPECT_EQ(getCurrentRole(), ROLE_NONE);
    EXPECT_STREQ(getCurrentUser(), "-");
}

static void getCurrentRole_ReflectsSession(void)
{
    EXPECT_EQ(getCurrentRole(), ROLE_ADMIN);
}

static void getCurrentUser_DashWhenLoggedOut(void)
{
    EXPECT_STREQ(getCurrentUser(), "admin");
    logout();
    EXPECT_STREQ(getCurrentUser(), "-");
}

static void roleName_AllRoles(void)
{
    EXPECT_STREQ(roleName(ROLE_ADMIN), "Administrator");
    EXPECT_STREQ(roleName(ROLE_OPERATOR), "HR Operator");
    EXPECT_STREQ(roleName(ROLE_NONE), "Unknown");
}

static void getUserCount_CountsUsers(void)
{
    EXPECT_EQ(getUserCount(), 0);
    ASSERT_EQ(addUserRecord("admin", "admin123", ROLE_ADMIN), EMS_OK);
    EXPECT_EQ(getUserCount(), 1);
}

static void getUserAt_Bounds(void)
{
    ASSERT_EQ(addUserRecord("admin", "admin123", ROLE_ADMIN), EMS_OK);
    EXPECT_NOT_NULL(getUserAt(0));
    EXPECT_NULL(getUserAt(1));
    EXPECT_NULL(getUserAt(-1));
}

static void checkAccess_AdminCanDoEverything(void)
{
    EXPECT_EQ(checkAccess(ROLE_ADMIN), 1);
    EXPECT_EQ(checkAccess(ROLE_OPERATOR), 1);
}

static void checkAccess_OperatorBlockedFromAdminTasks(void)
{
    setSession("hr_user", ROLE_OPERATOR);
    EXPECT_EQ(checkAccess(ROLE_OPERATOR), 1);
    EXPECT_EQ(checkAccess(ROLE_ADMIN), 0);
    EXPECT_CONTAINS(getOutput(), "E010: Access denied for your role.");
    EXPECT_CONTAINS(logText(), "E010");
}

static void checkAccess_NobodyLoggedIn(void)
{
    logout();
    EXPECT_EQ(checkAccess(ROLE_OPERATOR), 0);
}

static void loadUsers_MissingFileMeansNoUsers(void)
{
    EXPECT_EQ(loadUsers(), EMS_OK);
    EXPECT_EQ(getUserCount(), 0);
    EXPECT_CONTAINS(logText(), "first-run setup needed");
}

static void loadUsers_ReadsValidAndSkipsDamagedLines(void)
{
    char text[200];
    (void)snprintf(text, sizeof(text), "admin|%lu|1|2025-01-01 09:00:00\ngarbage line\nbad|1|9|2025-01-01 09:00:00\n",
                   hashPassword("admin", "admin123"));
    writeFile("users.txt", text);
    EXPECT_EQ(loadUsers(), EMS_OK);
    EXPECT_EQ(getUserCount(), 1);
    EXPECT_EQ(verifyCredentials("admin", "admin123"), ROLE_ADMIN);
    EXPECT_TRUE(fileExists("users.txt.bak"));
    EXPECT_CONTAINS(getOutput(), "2 damaged line(s)");
}

static void loadUsers_MissingFolderTreatedAsFirstRun(void)
{
    ASSERT_EQ(setDataDirectory("/no/such/folder"), EMS_OK);
    EXPECT_EQ(loadUsers(), EMS_OK);
    EXPECT_EQ(getUserCount(), 0);
}

static void saveUsers_WritesHashNotPassword(void)
{
    ASSERT_EQ(addUserRecord("admin", "admin123", ROLE_ADMIN), EMS_OK);
    EXPECT_EQ(saveUsers(), EMS_OK);
    const char *text = readFile("users.txt");
    EXPECT_CONTAINS(text, "admin|");
    EXPECT_NOT_CONTAINS(text, "admin123");
    authReset();
    EXPECT_EQ(loadUsers(), EMS_OK);
    EXPECT_EQ(verifyCredentials("admin", "admin123"), ROLE_ADMIN);
}

static void saveUsers_FailsWhenFolderMissing(void)
{
    ASSERT_EQ(setDataDirectory("/no/such/folder"), EMS_OK);
    EXPECT_EQ(saveUsers(), ERR_FILE);
}

static void readNewPassword_MatchingPasswords(void)
{
    char pw[PASSWORD_SIZE];
    feed("secret11\nsecret11\n");
    EXPECT_EQ(readNewPassword(pw, sizeof(pw)), EMS_OK);
    EXPECT_STREQ(pw, "secret11");
}

static void readNewPassword_RetriesOnMismatchAndWeak(void)
{
    char pw[PASSWORD_SIZE];
    feed("123\nsecret11\nsecret12\nsecret11\nsecret11\n");
    EXPECT_EQ(readNewPassword(pw, sizeof(pw)), EMS_OK);
    const char *out = getOutput();
    EXPECT_CONTAINS(out, "Password must be 6-29 characters");
    EXPECT_CONTAINS(out, "do not match");
}

static void readNewPassword_EndOfInput(void)
{
    char pw[PASSWORD_SIZE];
    feed("secret11\n");
    EXPECT_EQ(readNewPassword(pw, sizeof(pw)), ERR_END_OF_INPUT);
}

static void createFirstAdmin_CreatesAndSaves(void)
{
    feed("boss\nadmin123\nadmin123\n");
    EXPECT_EQ(createFirstAdmin(), EMS_OK);
    EXPECT_EQ(verifyCredentials("boss", "admin123"), ROLE_ADMIN);
    EXPECT_TRUE(fileExists("users.txt"));
}

static void createFirstAdmin_InvalidUsernameRetriedThenEof(void)
{
    feed("a b\n");
    EXPECT_EQ(createFirstAdmin(), ERR_END_OF_INPUT);
    EXPECT_EQ(getUserCount(), 0);
}

static void login_SuccessSetsSession(void)
{
    logout();
    ASSERT_EQ(addUserRecord("hr_user", "secret11", ROLE_OPERATOR), EMS_OK);
    feed("HR_USER\nsecret11\n");
    EXPECT_EQ(login(), EMS_OK);
    EXPECT_STREQ(getCurrentUser(), "hr_user");
    EXPECT_EQ(getCurrentRole(), ROLE_OPERATOR);
    EXPECT_CONTAINS(logText(), "hr_user logged in as HR Operator");
}

static void login_WrongThenRight(void)
{
    logout();
    ASSERT_EQ(addUserRecord("admin", "admin123", ROLE_ADMIN), EMS_OK);
    feed("admin\nwrong999\nadmin\nadmin123\n");
    EXPECT_EQ(login(), EMS_OK);
    EXPECT_CONTAINS(getOutput(), "E009");
    EXPECT_CONTAINS(logText(), "failed login for 'admin' (attempt 1)");
}

static void login_ThreeFailuresGiveE009(void)
{
    logout();
    ASSERT_EQ(addUserRecord("admin", "admin123", ROLE_ADMIN), EMS_OK);
    feed("admin\nx\nadmin\ny\nadmin\nz\nadmin\nadmin123\n");
    EXPECT_EQ(login(), ERR_LOGIN);
    EXPECT_EQ(getCurrentRole(), ROLE_NONE);
    EXPECT_CONTAINS(getOutput(), "Too many failed attempts");
}

static void login_FirstRunCreatesAdminThenLogsIn(void)
{
    logout();
    feed("boss\nadmin123\nadmin123\nboss\nadmin123\n");
    EXPECT_EQ(login(), EMS_OK);
    EXPECT_EQ(getCurrentRole(), ROLE_ADMIN);
}

static void login_TooLongUsernameCountsAsFailure(void)
{
    logout();
    ASSERT_EQ(addUserRecord("admin", "admin123", ROLE_ADMIN), EMS_OK);
    char text[100];
    (void)snprintf(text, sizeof(text), "%s\nadmin\nadmin123\n", repeatChar(60, 'a'));
    feed(text);
    EXPECT_EQ(login(), EMS_OK);
    EXPECT_EQ(getCurrentRole(), ROLE_ADMIN);
}

static void login_EndOfInput(void)
{
    logout();
    ASSERT_EQ(addUserRecord("admin", "admin123", ROLE_ADMIN), EMS_OK);
    feed("admin\n");
    EXPECT_EQ(login(), ERR_END_OF_INPUT);
}

static void addUser_CreatesOperator(void)
{
    feed("hr_user\nsecret11\nsecret11\n2\n");
    EXPECT_EQ(addUser(), EMS_OK);
    EXPECT_EQ(verifyCredentials("hr_user", "secret11"), ROLE_OPERATOR);
    EXPECT_CONTAINS(logText(), "User hr_user (HR Operator) created by admin");
}

static void addUser_DuplicateUsername(void)
{
    ASSERT_EQ(addUserRecord("hr_user", "secret11", ROLE_OPERATOR), EMS_OK);
    feed("HR_user\n");
    EXPECT_EQ(addUser(), ERR_DUPLICATE_ID);
    EXPECT_CONTAINS(getOutput(), "E002");
}

static void addUser_LimitReached(void)
{
    char name[16];
    for (int i = 0; i < MAX_USERS; i++) {
        snprintf(name, sizeof(name), "user%02d", i);
        ASSERT_EQ(addUserRecord(name, "secret11", ROLE_OPERATOR), EMS_OK);
    }
    feed("newbie\nsecret11\nsecret11\n1\n");
    EXPECT_EQ(addUser(), ERR_LIMIT_REACHED);
}

static void addUser_EndOfInput(void)
{
    feed("");
    EXPECT_EQ(addUser(), ERR_END_OF_INPUT);
}

static void addUserHashed_TimestampFromFileOrRejected(void)
{
    EXPECT_EQ(addUserHashed("loaded", 1UL, ROLE_ADMIN, "2025-01-01 09:00:00"), EMS_OK);
    EXPECT_STREQ(getUserAt(0)->createdAt, "2025-01-01 09:00:00");
    EXPECT_EQ(addUserHashed("other", 1UL, ROLE_ADMIN, "bad"), ERR_INVALID_INPUT);
}

static void logout_LogsWhoLoggedOut(void)
{
    logout();
    EXPECT_CONTAINS(logText(), "[AUTH] admin logged out");
}

void run_auth_tests(void)
{
    RUN_TEST(authReset_ClearsUsersAndSession);
    RUN_TEST(hashPassword_IsDeterministicAndSalted);
    RUN_TEST(hashPassword_FitsIn32BitsAndHandlesNull);
    RUN_TEST(findUser_CaseInsensitive);
    RUN_TEST(addUserRecord_ValidUser);
    RUN_TEST(addUserRecord_RejectsBadInput);
    RUN_TEST(addUserRecord_RejectsDuplicate);
    RUN_TEST(addUserRecord_LimitReached);
    RUN_TEST(addUserHashed_StoresGivenHash);
    RUN_TEST(verifyCredentials_RightAndWrongPassword);
    RUN_TEST(setSession_ValidAndInvalidRole);
    RUN_TEST(logout_ClearsSession);
    RUN_TEST(getCurrentRole_ReflectsSession);
    RUN_TEST(getCurrentUser_DashWhenLoggedOut);
    RUN_TEST(roleName_AllRoles);
    RUN_TEST(getUserCount_CountsUsers);
    RUN_TEST(getUserAt_Bounds);
    RUN_TEST(checkAccess_AdminCanDoEverything);
    RUN_TEST(checkAccess_OperatorBlockedFromAdminTasks);
    RUN_TEST(checkAccess_NobodyLoggedIn);
    RUN_TEST(loadUsers_MissingFileMeansNoUsers);
    RUN_TEST(loadUsers_ReadsValidAndSkipsDamagedLines);
    RUN_TEST(loadUsers_MissingFolderTreatedAsFirstRun);
    RUN_TEST(saveUsers_WritesHashNotPassword);
    RUN_TEST(saveUsers_FailsWhenFolderMissing);
    RUN_TEST(readNewPassword_MatchingPasswords);
    RUN_TEST(readNewPassword_RetriesOnMismatchAndWeak);
    RUN_TEST(readNewPassword_EndOfInput);
    RUN_TEST(createFirstAdmin_CreatesAndSaves);
    RUN_TEST(createFirstAdmin_InvalidUsernameRetriedThenEof);
    RUN_TEST(login_SuccessSetsSession);
    RUN_TEST(login_WrongThenRight);
    RUN_TEST(login_ThreeFailuresGiveE009);
    RUN_TEST(login_FirstRunCreatesAdminThenLogsIn);
    RUN_TEST(login_TooLongUsernameCountsAsFailure);
    RUN_TEST(login_EndOfInput);
    RUN_TEST(addUser_CreatesOperator);
    RUN_TEST(addUser_DuplicateUsername);
    RUN_TEST(addUser_LimitReached);
    RUN_TEST(addUser_EndOfInput);
    RUN_TEST(addUserHashed_TimestampFromFileOrRejected);
    RUN_TEST(logout_LogsWhoLoggedOut);
}
