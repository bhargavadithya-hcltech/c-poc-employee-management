/** @file test_logger.c  Unit tests for the logger module. */
#include "test_framework.h"



static void logInit_NullPathReturnsInvalidInput(void)
{
    EXPECT_EQ(logInit(NULL), ERR_INVALID_INPUT);
    EXPECT_EQ(logIsOpen(), 0);
}

static void logInit_MissingFolderReturnsFileError(void)
{
    EXPECT_EQ(logInit("/no/such/folder/ems.log"), ERR_FILE);
}

static void logInit_ValidPathOpensLog(void)
{
    EXPECT_EQ(logInit(testPath("other.log")), EMS_OK);
    EXPECT_EQ(logIsOpen(), 1);
}

static void logClose_ClosesAndIsSafeTwice(void)
{
    logClose();
    EXPECT_EQ(logIsOpen(), 0);
    logClose();
    EXPECT_EQ(logIsOpen(), 0);
}

static void logIsOpen_ReportsState(void)
{
    EXPECT_EQ(logIsOpen(), 1);
    logClose();
    EXPECT_EQ(logIsOpen(), 0);
}

static void getTimestamp_FormatsDateAndTime(void)
{
    char buf[32];
    ASSERT_EQ(getTimestamp(buf, sizeof(buf)), EMS_OK);
    EXPECT_EQ(strlen(buf), 19u);
    EXPECT_EQ(buf[4], '-');
    EXPECT_EQ(buf[10], ' ');
    EXPECT_EQ(buf[13], ':');
}

static void getTimestamp_RejectsNullOrSmallBuffer(void)
{
    char small[10];
    EXPECT_EQ(getTimestamp(NULL, 32), ERR_INVALID_INPUT);
    EXPECT_EQ(getTimestamp(small, sizeof(small)), ERR_INVALID_INPUT);
}

static void logLevelName_AllLevels(void)
{
    EXPECT_STREQ(logLevelName(LOG_INFO), "INFO");
    EXPECT_STREQ(logLevelName(LOG_WARN), "WARN");
    EXPECT_STREQ(logLevelName(LOG_ERROR), "ERROR");
    EXPECT_STREQ(logLevelName(99), "INFO");
}

static void logMessage_WritesTimestampLevelAndModule(void)
{
    EXPECT_EQ(logMessage(LOG_WARN, "TEST", "hello"), EMS_OK);
    const char *logTxt = logText();
    EXPECT_CONTAINS(logTxt, "[WARN] [TEST] hello");
    EXPECT_EQ(logTxt[4], '-');
}

static void logMessage_RejectsNullArguments(void)
{
    EXPECT_EQ(logMessage(LOG_INFO, NULL, "x"), ERR_INVALID_INPUT);
    EXPECT_EQ(logMessage(LOG_INFO, "X", NULL), ERR_INVALID_INPUT);
}

static void logMessage_FailsWhenLogClosed(void)
{
    logClose();
    EXPECT_EQ(logMessage(LOG_INFO, "X", "y"), ERR_FILE);
}

static void logError_WritesCodeAndDetail(void)
{
    EXPECT_EQ(logError("EMPLOYEE", ERR_NOT_FOUND, "id 5"), EMS_OK);
    const char *logTxt = logText();
    EXPECT_CONTAINS(logTxt, "[ERROR] [EMPLOYEE] E003 Record not found. | id 5");
}

static void logError_WorksWithoutDetail(void)
{
    EXPECT_EQ(logError("AUTH", ERR_LOGIN, NULL), EMS_OK);
    EXPECT_CONTAINS(logText(), "E009 Invalid username or password.");
}

static void errorCodeText_AllCodes(void)
{
    EXPECT_STREQ(errorCodeText(EMS_OK), "E000");
    EXPECT_STREQ(errorCodeText(ERR_INVALID_INPUT), "E001");
    EXPECT_STREQ(errorCodeText(ERR_DUPLICATE_ID), "E002");
    EXPECT_STREQ(errorCodeText(ERR_NOT_FOUND), "E003");
    EXPECT_STREQ(errorCodeText(ERR_INVALID_ATTENDANCE), "E004");
    EXPECT_STREQ(errorCodeText(ERR_PAYROLL_EXISTS), "E005");
    EXPECT_STREQ(errorCodeText(ERR_DEPT_ROLE_NOT_FOUND), "E006");
    EXPECT_STREQ(errorCodeText(ERR_FILE), "E007");
    EXPECT_STREQ(errorCodeText(ERR_MEMORY), "E008");
    EXPECT_STREQ(errorCodeText(ERR_LOGIN), "E009");
    EXPECT_STREQ(errorCodeText(ERR_ACCESS_DENIED), "E010");
}

static void errorCodeText_UnknownAndInternalCodes(void)
{
    EXPECT_STREQ(errorCodeText(ERR_END_OF_INPUT), "E000");
    EXPECT_STREQ(errorCodeText((EmsStatus)(-1)), "E000");
}

static void errorMessage_AllCodesHaveText(void)
{
    for (int c = EMS_OK; c <= ERR_LIMIT_REACHED; c++) {
        const char *m = errorMessage((EmsStatus)(c));
        ASSERT_NOT_NULL(m);
        EXPECT_GT(strlen(m), 3u);
    }
    EXPECT_STREQ(errorMessage(ERR_ACCESS_DENIED), "Access denied for your role.");
    EXPECT_STREQ(errorMessage((EmsStatus)(77)), "Unknown error.");
}

static void showError_PrintsAndLogs(void)
{
    showError("PAYROLL", ERR_PAYROLL_EXISTS);
    EXPECT_CONTAINS(getOutput(), "E005: Payroll already generated for this month.");
    EXPECT_CONTAINS(logText(), "[PAYROLL] E005");
}

static void showError_NullModuleUsesDefault(void)
{
    showError(NULL, ERR_FILE);
    EXPECT_CONTAINS(logText(), "[EMS] E007");
}

static void setTimestamp_EmptyMeansNow(void)
{
    char ts[TIMESTAMP_SIZE];
    EXPECT_EQ(setTimestamp(ts, NULL), EMS_OK);
    EXPECT_EQ(validateTimestamp(ts), 1);
    EXPECT_EQ(setTimestamp(ts, ""), EMS_OK);
    EXPECT_EQ(validateTimestamp(ts), 1);
}

static void setTimestamp_KeepsGivenValidTime(void)
{
    char ts[TIMESTAMP_SIZE];
    EXPECT_EQ(setTimestamp(ts, "2025-01-01 09:00:00"), EMS_OK);
    EXPECT_STREQ(ts, "2025-01-01 09:00:00");
}

static void setTimestamp_RejectsBadTimeAndNull(void)
{
    char ts[TIMESTAMP_SIZE];
    EXPECT_EQ(setTimestamp(ts, "yesterday"), ERR_INVALID_INPUT);
    EXPECT_EQ(setTimestamp(NULL, NULL), ERR_INVALID_INPUT);
}

void run_logger_tests(void)
{
    RUN_TEST(logInit_NullPathReturnsInvalidInput);
    RUN_TEST(logInit_MissingFolderReturnsFileError);
    RUN_TEST(logInit_ValidPathOpensLog);
    RUN_TEST(logClose_ClosesAndIsSafeTwice);
    RUN_TEST(logIsOpen_ReportsState);
    RUN_TEST(getTimestamp_FormatsDateAndTime);
    RUN_TEST(getTimestamp_RejectsNullOrSmallBuffer);
    RUN_TEST(logLevelName_AllLevels);
    RUN_TEST(logMessage_WritesTimestampLevelAndModule);
    RUN_TEST(logMessage_RejectsNullArguments);
    RUN_TEST(logMessage_FailsWhenLogClosed);
    RUN_TEST(logError_WritesCodeAndDetail);
    RUN_TEST(logError_WorksWithoutDetail);
    RUN_TEST(errorCodeText_AllCodes);
    RUN_TEST(errorCodeText_UnknownAndInternalCodes);
    RUN_TEST(errorMessage_AllCodesHaveText);
    RUN_TEST(showError_PrintsAndLogs);
    RUN_TEST(showError_NullModuleUsesDefault);
    RUN_TEST(setTimestamp_EmptyMeansNow);
    RUN_TEST(setTimestamp_KeepsGivenValidTime);
    RUN_TEST(setTimestamp_RejectsBadTimeAndNull);
}
