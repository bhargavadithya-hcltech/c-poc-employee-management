/**
 * @file test_ems.cpp
 * @brief GoogleTest suite for the Employee Management System.
 *
 * One fixture per module. Every test starts from test_reset_system():
 * empty tables, a fresh temporary data folder, a fresh log and an
 * Administrator session. Test names follow functionUnderTest_Scenario.
 * Each function has positive, negative and boundary cases where they apply.
 * The last section holds the cross-module integration tests.
 */
#include <gtest/gtest.h>
#include <cstring>
#include <cstdlib>
#include "test_shim.h"

/* text checks used throughout */
#define EXPECT_CONTAINS(text, part) EXPECT_NE(std::strstr((text), (part)), nullptr) << "missing: " << (part)
#define EXPECT_NOT_CONTAINS(text, part) EXPECT_EQ(std::strstr((text), (part)), nullptr) << "unexpected: " << (part)

/* ========================================================================= */
/* Test fixtures                                                             */
/* ========================================================================= */
class EMSTest : public ::testing::Test {
protected:
    void SetUp() override { test_reset_system(); }
    void TearDown() override { test_cleanup_system(); }
};

/* Fixture with departments, roles and two employees already loaded */
class SeededTest : public EMSTest {
protected:
    void SetUp() override
    {
        EMSTest::SetUp();
        ASSERT_EQ(test_seed_data(), EMS_OK);
    }
};

class LoggerTest : public EMSTest {};
class IoTest : public EMSTest {};
class ValidationTest : public EMSTest {};
class AuthTest : public EMSTest {};
class DepartmentTest : public EMSTest {};
class RoleTest : public EMSTest {};
class EmployeeTest : public SeededTest {};
class SearchingTest : public SeededTest {};
class AttendanceTest : public SeededTest {};
class PayrollTest : public SeededTest {};
class FileManagerTest : public EMSTest {};
class AppTest : public EMSTest {};
class IntegrationTest : public EMSTest {};

/* Report tests also need attendance and one payroll record for March 2025 */
class ReportTest : public SeededTest {
protected:
    void SetUp() override
    {
        SeededTest::SetUp();
        Date d1 = makeDate(1, 3, 2025);
        Date d2 = makeDate(2, 3, 2025);
        Payroll p;
        ASSERT_EQ(attendanceAdd(101, &d1, 'P', nullptr), EMS_OK);
        ASSERT_EQ(attendanceAdd(101, &d2, 'A', nullptr), EMS_OK);
        ASSERT_EQ(payrollCompute(101, 3, 2025, 5000.0, 2000.0, &p), EMS_OK);
        ASSERT_EQ(payrollAdd(&p), EMS_OK);
    }
};

/* ========================================================================= */
/* Helpers                                                                   */
/* ========================================================================= */
static void mark(int emp, int d, int m, int y, char s)
{
    Date date = makeDate(d, m, y);
    EXPECT_EQ(attendanceAdd(emp, &date, s, nullptr), EMS_OK);
}

static void absent(int emp, int d)
{
    Date date = makeDate(d, 3, 2025);
    EXPECT_EQ(attendanceAdd(emp, &date, 'A', nullptr), EMS_OK);
}

static void addPayroll(int emp, int m, int y)
{
    Payroll p;
    EXPECT_EQ(payrollCompute(emp, m, y, 1000.0, 500.0, &p), EMS_OK);
    EXPECT_EQ(payrollAdd(&p), EMS_OK);
}

static void writeAdmin(void)
{
    char text[256];

    (void)snprintf(text, sizeof(text), "admin|%lu|1|2025-01-01 09:00:00\nhr_user|%lu|2|2025-01-01 09:00:00\n",
                   hashPassword("admin", "admin123"), hashPassword("hr_user", "secret11"));
    writeFile("users.txt", text);
}

/* ========================================================================= */
/* Logger Module (logger.c)                                                  */
/* ========================================================================= */

TEST_F(LoggerTest, logInit_NullPathReturnsInvalidInput)
{
    EXPECT_EQ(logInit(nullptr), ERR_INVALID_INPUT);
    EXPECT_EQ(logIsOpen(), 0);
}

TEST_F(LoggerTest, logInit_MissingFolderReturnsFileError)
{
    EXPECT_EQ(logInit("/no/such/folder/ems.log"), ERR_FILE);
}

TEST_F(LoggerTest, logInit_ValidPathOpensLog)
{
    EXPECT_EQ(logInit(testPath("other.log")), EMS_OK);
    EXPECT_EQ(logIsOpen(), 1);
}

TEST_F(LoggerTest, logClose_ClosesAndIsSafeTwice)
{
    logClose();
    EXPECT_EQ(logIsOpen(), 0);
    logClose();
    EXPECT_EQ(logIsOpen(), 0);
}

TEST_F(LoggerTest, logIsOpen_ReportsState)
{
    EXPECT_EQ(logIsOpen(), 1);
    logClose();
    EXPECT_EQ(logIsOpen(), 0);
}

TEST_F(LoggerTest, getTimestamp_FormatsDateAndTime)
{
    char buf[32];
    ASSERT_EQ(getTimestamp(buf, sizeof(buf)), EMS_OK);
    EXPECT_EQ(strlen(buf), 19u);
    EXPECT_EQ(buf[4], '-');
    EXPECT_EQ(buf[10], ' ');
    EXPECT_EQ(buf[13], ':');
}

TEST_F(LoggerTest, getTimestamp_RejectsNullOrSmallBuffer)
{
    char small[10];
    EXPECT_EQ(getTimestamp(nullptr, 32), ERR_INVALID_INPUT);
    EXPECT_EQ(getTimestamp(small, sizeof(small)), ERR_INVALID_INPUT);
}

TEST_F(LoggerTest, logLevelName_AllLevels)
{
    EXPECT_STREQ(logLevelName(LOG_INFO), "INFO");
    EXPECT_STREQ(logLevelName(LOG_WARN), "WARN");
    EXPECT_STREQ(logLevelName(LOG_ERROR), "ERROR");
    EXPECT_STREQ(logLevelName(99), "INFO");
}

TEST_F(LoggerTest, logMessage_WritesTimestampLevelAndModule)
{
    EXPECT_EQ(logMessage(LOG_WARN, "TEST", "hello"), EMS_OK);
    const char *logTxt = logText();
    EXPECT_CONTAINS(logTxt, "[WARN] [TEST] hello");
    EXPECT_EQ(logTxt[4], '-');
}

TEST_F(LoggerTest, logMessage_RejectsNullArguments)
{
    EXPECT_EQ(logMessage(LOG_INFO, nullptr, "x"), ERR_INVALID_INPUT);
    EXPECT_EQ(logMessage(LOG_INFO, "X", nullptr), ERR_INVALID_INPUT);
}

TEST_F(LoggerTest, logMessage_FailsWhenLogClosed)
{
    logClose();
    EXPECT_EQ(logMessage(LOG_INFO, "X", "y"), ERR_FILE);
}

TEST_F(LoggerTest, logError_WritesCodeAndDetail)
{
    EXPECT_EQ(logError("EMPLOYEE", ERR_NOT_FOUND, "id 5"), EMS_OK);
    const char *logTxt = logText();
    EXPECT_CONTAINS(logTxt, "[ERROR] [EMPLOYEE] E003 Record not found. | id 5");
}

TEST_F(LoggerTest, logError_WorksWithoutDetail)
{
    EXPECT_EQ(logError("AUTH", ERR_LOGIN, nullptr), EMS_OK);
    EXPECT_CONTAINS(logText(), "E009 Invalid username or password.");
}

TEST_F(LoggerTest, errorCodeText_AllCodes)
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

TEST_F(LoggerTest, errorCodeText_UnknownAndInternalCodes)
{
    EXPECT_STREQ(errorCodeText(ERR_END_OF_INPUT), "E000");
    EXPECT_STREQ(errorCodeText((EmsStatus)(-1)), "E000");
}

TEST_F(LoggerTest, errorMessage_AllCodesHaveText)
{
    for (int c = EMS_OK; c <= ERR_LIMIT_REACHED; c++) {
        const char *m = errorMessage((EmsStatus)(c));
        ASSERT_NE(m, nullptr);
        EXPECT_GT(strlen(m), 3u);
    }
    EXPECT_STREQ(errorMessage(ERR_ACCESS_DENIED), "Access denied for your role.");
    EXPECT_STREQ(errorMessage((EmsStatus)(77)), "Unknown error.");
}

TEST_F(LoggerTest, showError_PrintsAndLogs)
{
    showError("PAYROLL", ERR_PAYROLL_EXISTS);
    EXPECT_CONTAINS(getOutput(), "E005: Payroll already generated for this month.");
    EXPECT_CONTAINS(logText(), "[PAYROLL] E005");
}

TEST_F(LoggerTest, showError_NullModuleUsesDefault)
{
    showError(nullptr, ERR_FILE);
    EXPECT_CONTAINS(logText(), "[EMS] E007");
}

TEST_F(LoggerTest, setTimestamp_EmptyMeansNow)
{
    char ts[TIMESTAMP_SIZE];
    EXPECT_EQ(setTimestamp(ts, nullptr), EMS_OK);
    EXPECT_EQ(validateTimestamp(ts), 1);
    EXPECT_EQ(setTimestamp(ts, ""), EMS_OK);
    EXPECT_EQ(validateTimestamp(ts), 1);
}

TEST_F(LoggerTest, setTimestamp_KeepsGivenValidTime)
{
    char ts[TIMESTAMP_SIZE];
    EXPECT_EQ(setTimestamp(ts, "2025-01-01 09:00:00"), EMS_OK);
    EXPECT_STREQ(ts, "2025-01-01 09:00:00");
}

TEST_F(LoggerTest, setTimestamp_RejectsBadTimeAndNull)
{
    char ts[TIMESTAMP_SIZE];
    EXPECT_EQ(setTimestamp(ts, "yesterday"), ERR_INVALID_INPUT);
    EXPECT_EQ(setTimestamp(nullptr, nullptr), ERR_INVALID_INPUT);
}

/* ========================================================================= */
/* Console I/O Module (io.c)                                                 */
/* ========================================================================= */

TEST_F(IoTest, setInputStream_NullRestoresStdin)
{
    feed("x\n");
    EXPECT_EQ(getInputStream(), testInput());
    setInputStream(nullptr);
    EXPECT_EQ(getInputStream(), stdin);
}

TEST_F(IoTest, getInputStream_DefaultsToStdin)
{
    setInputStream(nullptr);
    EXPECT_EQ(getInputStream(), stdin);
}

TEST_F(IoTest, setOutputStream_NullRestoresStdout)
{
    setOutputStream(nullptr);
    EXPECT_EQ(getOutputStream(), stdout);
    setOutputStream(testOutput());
    EXPECT_EQ(getOutputStream(), testOutput());
}

TEST_F(IoTest, getOutputStream_ReturnsRedirectedStream)
{
    EXPECT_EQ(getOutputStream(), testOutput());
}

TEST_F(IoTest, trimString_RemovesBothEnds)
{
    char s[] = "   hello world \t ";
    trimString(s);
    EXPECT_STREQ(s, "hello world");
}

TEST_F(IoTest, trimString_AllSpacesBecomesEmpty)
{
    char s[] = "    ";
    trimString(s);
    EXPECT_STREQ(s, "");
}

TEST_F(IoTest, trimString_NullAndEmptyAreSafe)
{
    char s[] = "";
    trimString(nullptr);
    trimString(s);
    EXPECT_STREQ(s, "");
}

TEST_F(IoTest, readLine_ReadsAndTrimsLine)
{
    char buf[32];
    feed("  Ravi  \n");
    EXPECT_EQ(readLine("Name: ", buf, sizeof(buf)), EMS_OK);
    EXPECT_STREQ(buf, "Ravi");
    EXPECT_CONTAINS(getOutput(), "Name: ");
}

TEST_F(IoTest, readLine_EndOfInput)
{
    char buf[32];
    feed("");
    EXPECT_EQ(readLine(nullptr, buf, sizeof(buf)), ERR_END_OF_INPUT);
    EXPECT_STREQ(buf, "");
}

TEST_F(IoTest, readLine_TooLongLineIsDiscarded)
{
    char buf[8];
    feed("abcdefghijklmnop\nok\n");
    EXPECT_EQ(readLine("", buf, sizeof(buf)), ERR_INVALID_INPUT);
    EXPECT_EQ(readLine("", buf, sizeof(buf)), EMS_OK);
    EXPECT_STREQ(buf, "ok");
}

TEST_F(IoTest, readLine_RejectsBadBuffer)
{
    char buf[1];
    EXPECT_EQ(readLine("", nullptr, 10), ERR_INVALID_INPUT);
    EXPECT_EQ(readLine("", buf, sizeof(buf)), ERR_INVALID_INPUT);
}

TEST_F(IoTest, readLine_LastLineWithoutNewline)
{
    char buf[16];
    feed("last");
    EXPECT_EQ(readLine("", buf, sizeof(buf)), EMS_OK);
    EXPECT_STREQ(buf, "last");
}

TEST_F(IoTest, readInt_ValidNumber)
{
    int v = 0;
    feed("42\n");
    EXPECT_EQ(readInt("n: ", 1, 100, &v), EMS_OK);
    EXPECT_EQ(v, 42);
}

TEST_F(IoTest, readInt_RetriesOnTextAndRange)
{
    int v = 0;
    feed("abc\n12x\n\n500\n7\n");
    EXPECT_EQ(readInt("n: ", 1, 100, &v), EMS_OK);
    EXPECT_EQ(v, 7);
    const char *out = getOutput();
    EXPECT_CONTAINS(out, "Please enter a whole number.");
    EXPECT_CONTAINS(out, "Enter a number from 1 to 100.");
}

TEST_F(IoTest, readInt_AcceptsBoundaries)
{
    int v = 0;
    feed("1\n100\n");
    EXPECT_EQ(readInt("", 1, 100, &v), EMS_OK);
    EXPECT_EQ(v, 1);
    EXPECT_EQ(readInt("", 1, 100, &v), EMS_OK);
    EXPECT_EQ(v, 100);
}

TEST_F(IoTest, readInt_RejectsJustOutsideBoundaries)
{
    int v = 0;
    feed("0\n101\n");
    EXPECT_EQ(readInt("", 1, 100, &v), ERR_END_OF_INPUT);
}

TEST_F(IoTest, readInt_NullValueAndEof)
{
    int v = 0;
    EXPECT_EQ(readInt("", 1, 2, nullptr), ERR_INVALID_INPUT);
    feed("");
    EXPECT_EQ(readInt("", 1, 2, &v), ERR_END_OF_INPUT);
}

TEST_F(IoTest, readFloat_ValidAmount)
{
    double v = 0;
    feed("1234.50\n");
    EXPECT_EQ(readFloat("", 0.0, MAX_MONEY, &v), EMS_OK);
    EXPECT_DOUBLE_EQ(v, 1234.5);
}

TEST_F(IoTest, readFloat_RetriesOnBadInput)
{
    double v = 0;
    feed("ten\n-5\n10.5\n");
    EXPECT_EQ(readFloat("", 0.0, 100.0, &v), EMS_OK);
    EXPECT_DOUBLE_EQ(v, 10.5);
    const char *out = getOutput();
    EXPECT_CONTAINS(out, "Please enter a valid amount.");
    EXPECT_CONTAINS(out, "Amount must be from");
}

TEST_F(IoTest, readFloat_NullValueAndEof)
{
    double v = 0;
    EXPECT_EQ(readFloat("", 0, 1, nullptr), ERR_INVALID_INPUT);
    feed("");
    EXPECT_EQ(readFloat("", 0, 1, &v), ERR_END_OF_INPUT);
}

TEST_F(IoTest, readString_ValidText)
{
    char buf[16];
    feed("Finance\n");
    EXPECT_EQ(readString("", buf, sizeof(buf), 0), EMS_OK);
    EXPECT_STREQ(buf, "Finance");
}

TEST_F(IoTest, readString_BlankAllowed)
{
    char buf[16] = "old";
    feed("\n");
    EXPECT_EQ(readString("", buf, sizeof(buf), 1), EMS_OK);
    EXPECT_STREQ(buf, "");
}

TEST_F(IoTest, readString_RejectsBlankPipeAndTooLong)
{
    char buf[6];
    feed("\na|b\nabcdefgh\nabc\n");
    EXPECT_EQ(readString("", buf, sizeof(buf), 0), EMS_OK);
    EXPECT_STREQ(buf, "abc");
    const char *out = getOutput();
    EXPECT_CONTAINS(out, "Cannot be blank or contain the | character.");
    EXPECT_CONTAINS(out, "Too long (maximum 5 characters).");
}

TEST_F(IoTest, readString_NullBufferAndEof)
{
    char buf[8];
    EXPECT_EQ(readString("", nullptr, 8, 0), ERR_INVALID_INPUT);
    feed("");
    EXPECT_EQ(readString("", buf, sizeof(buf), 0), ERR_END_OF_INPUT);
}

TEST_F(IoTest, readChar_ConvertsToUpperCase)
{
    char c = ' ';
    feed("a\n");
    EXPECT_EQ(readChar("", "PAL", &c), EMS_OK);
    EXPECT_EQ(c, 'A');
}

TEST_F(IoTest, readChar_RetriesOnInvalid)
{
    char c = ' ';
    feed("x\nPA\nl\n");
    EXPECT_EQ(readChar("", "PAL", &c), EMS_OK);
    EXPECT_EQ(c, 'L');
    EXPECT_CONTAINS(getOutput(), "Type one of: PAL");
}

TEST_F(IoTest, readChar_NullArgumentsAndEof)
{
    char c;
    EXPECT_EQ(readChar("", nullptr, &c), ERR_INVALID_INPUT);
    EXPECT_EQ(readChar("", "PAL", nullptr), ERR_INVALID_INPUT);
    feed("");
    EXPECT_EQ(readChar("", "PAL", &c), ERR_END_OF_INPUT);
}

TEST_F(IoTest, readYesNo_YesAndNo)
{
    int a = -1;
    feed("y\nN\n");
    EXPECT_EQ(readYesNo("", &a), EMS_OK);
    EXPECT_EQ(a, 1);
    EXPECT_EQ(readYesNo("", &a), EMS_OK);
    EXPECT_EQ(a, 0);
}

TEST_F(IoTest, readYesNo_RetriesAndNull)
{
    int a = -1;
    feed("maybe\ny\n");
    EXPECT_EQ(readYesNo("", &a), EMS_OK);
    EXPECT_EQ(a, 1);
    EXPECT_EQ(readYesNo("", nullptr), ERR_INVALID_INPUT);
}

TEST_F(IoTest, readDate_ValidDate)
{
    Date d;
    feed("29\n2\n2024\n");
    EXPECT_EQ(readDate(&d), EMS_OK);
    EXPECT_EQ(d.day, 29);
    EXPECT_EQ(d.month, 2);
    EXPECT_EQ(d.year, 2024);
}

TEST_F(IoTest, readDate_RejectsImpossibleDate)
{
    Date d;
    feed("31\n2\n2025\n28\n2\n2025\n");
    EXPECT_EQ(readDate(&d), EMS_OK);
    EXPECT_EQ(d.day, 28);
    EXPECT_CONTAINS(getOutput(), "That date does not exist.");
}

TEST_F(IoTest, readDate_NullAndEof)
{
    Date d;
    EXPECT_EQ(readDate(nullptr), ERR_INVALID_INPUT);
    feed("5\n");
    EXPECT_EQ(readDate(&d), ERR_END_OF_INPUT);
}

TEST_F(IoTest, readValidated_RetriesUntilValid)
{
    char buf[PHONE_SIZE];
    feed("12345\n9876543210\n");
    EXPECT_EQ(readValidated("", buf, sizeof(buf), validatePhone, "bad phone"), EMS_OK);
    EXPECT_STREQ(buf, "9876543210");
    EXPECT_CONTAINS(getOutput(), "bad phone");
}

TEST_F(IoTest, readValidated_DefaultErrorTextNullAndEof)
{
    char buf[16];
    EXPECT_EQ(readValidated("", buf, sizeof(buf), nullptr, nullptr), ERR_INVALID_INPUT);
    feed("12\n");
    EXPECT_EQ(readValidated("", buf, sizeof(buf), validatePhone, nullptr), ERR_END_OF_INPUT);
    EXPECT_CONTAINS(getOutput(), "Invalid value.");
}

/* ========================================================================= */
/* Validation Module (validation.c)                                          */
/* ========================================================================= */

TEST_F(ValidationTest, validateText_AcceptsNormalText)
{
    EXPECT_EQ(validateText("Finance Team"), 1);
}

TEST_F(ValidationTest, validateText_RejectsBlankPipeControlAndNull)
{
    EXPECT_EQ(validateText(""), 0);
    EXPECT_EQ(validateText("   "), 0);
    EXPECT_EQ(validateText("a|b"), 0);
    EXPECT_EQ(validateText("ab\tc"), 0);
    EXPECT_EQ(validateText(nullptr), 0);
}

TEST_F(ValidationTest, validateName_AcceptsLettersSpacesDotsApostrophes)
{
    EXPECT_EQ(validateName("Ravi Kumar"), 1);
    EXPECT_EQ(validateName("A. D'Souza"), 1);
}

TEST_F(ValidationTest, validateName_RejectsDigitsSymbolsAndLength)
{
    EXPECT_EQ(validateName("Ravi2"), 0);
    EXPECT_EQ(validateName("Ravi@"), 0);
    EXPECT_EQ(validateName(""), 0);
    EXPECT_EQ(validateName(nullptr), 0);
    EXPECT_EQ(validateName(repeatChar(49, 'a')), 1);
    EXPECT_EQ(validateName(repeatChar(50, 'a')), 0);
}

TEST_F(ValidationTest, validatePhone_ExactlyTenDigits)
{
    EXPECT_EQ(validatePhone("9876543210"), 1);
    EXPECT_EQ(validatePhone("987654321"), 0);
    EXPECT_EQ(validatePhone("98765432101"), 0);
    EXPECT_EQ(validatePhone("98765x3210"), 0);
    EXPECT_EQ(validatePhone(nullptr), 0);
}

TEST_F(ValidationTest, validateEmail_AcceptsNormalAddress)
{
    EXPECT_EQ(validateEmail("ravi@corp.com"), 1);
    EXPECT_EQ(validateEmail("a.b@x.co.in"), 1);
}

TEST_F(ValidationTest, validateEmail_RejectsBadShapes)
{
    EXPECT_EQ(validateEmail("ravi.corp.com"), 0);
    EXPECT_EQ(validateEmail("@corp.com"), 0);
    EXPECT_EQ(validateEmail("ravi@corp"), 0);
    EXPECT_EQ(validateEmail("ravi@.com"), 0);
    EXPECT_EQ(validateEmail("ravi@corp."), 0);
    EXPECT_EQ(validateEmail("ra vi@corp.com"), 0);
    EXPECT_EQ(validateEmail("a@b@c.com"), 0);
    EXPECT_EQ(validateEmail("r|v@corp.com"), 0);
    EXPECT_EQ(validateEmail("a@b"), 0);
    EXPECT_EQ(validateEmail(nullptr), 0);
    char tooLong[64];
    (void)snprintf(tooLong, sizeof(tooLong), "%s@x.com", repeatChar(45, 'a'));
    EXPECT_EQ(validateEmail(tooLong), 0);
}

TEST_F(ValidationTest, validateUsername_LengthAndCharacters)
{
    EXPECT_EQ(validateUsername("admin"), 1);
    EXPECT_EQ(validateUsername("hr_01"), 1);
    EXPECT_EQ(validateUsername("ab"), 0);
    EXPECT_EQ(validateUsername("abcdefghijklmnopqrst"), 0);
    EXPECT_EQ(validateUsername("bad name"), 0);
    EXPECT_EQ(validateUsername(nullptr), 0);
}

TEST_F(ValidationTest, validatePassword_LengthAndCharacters)
{
    EXPECT_EQ(validatePassword("secret1"), 1);
    EXPECT_EQ(validatePassword("12345"), 0);
    EXPECT_EQ(validatePassword("has space"), 0);
    EXPECT_EQ(validatePassword("pipe|123"), 0);
    EXPECT_EQ(validatePassword(repeatChar(30, 'x')), 0);
    EXPECT_EQ(validatePassword(repeatChar(29, 'x')), 1);
    EXPECT_EQ(validatePassword(nullptr), 0);
}

TEST_F(ValidationTest, validateId_Boundaries)
{
    EXPECT_EQ(validateId(1), 1);
    EXPECT_EQ(validateId(MAX_ID), 1);
    EXPECT_EQ(validateId(0), 0);
    EXPECT_EQ(validateId(MAX_ID + 1), 0);
    EXPECT_EQ(validateId(-5), 0);
}

TEST_F(ValidationTest, validateSalary_MustBePositive)
{
    EXPECT_EQ(validateSalary(0.01), 1);
    EXPECT_EQ(validateSalary(MAX_MONEY), 1);
    EXPECT_EQ(validateSalary(0.0), 0);
    EXPECT_EQ(validateSalary(-1.0), 0);
    EXPECT_EQ(validateSalary(MAX_MONEY + 1.0), 0);
}

TEST_F(ValidationTest, validateAmount_ZeroAllowed)
{
    EXPECT_EQ(validateAmount(0.0), 1);
    EXPECT_EQ(validateAmount(500.0), 1);
    EXPECT_EQ(validateAmount(-0.01), 0);
    EXPECT_EQ(validateAmount(MAX_MONEY + 1.0), 0);
}

TEST_F(ValidationTest, isLeapYear_Rules)
{
    EXPECT_EQ(isLeapYear(2024), 1);
    EXPECT_EQ(isLeapYear(2000), 1);
    EXPECT_EQ(isLeapYear(2100), 0);
    EXPECT_EQ(isLeapYear(2025), 0);
}

TEST_F(ValidationTest, daysInMonth_AllCases)
{
    EXPECT_EQ(daysInMonth(1, 2025), 31);
    EXPECT_EQ(daysInMonth(4, 2025), 30);
    EXPECT_EQ(daysInMonth(2, 2025), 28);
    EXPECT_EQ(daysInMonth(2, 2024), 29);
    EXPECT_EQ(daysInMonth(0, 2025), 0);
    EXPECT_EQ(daysInMonth(13, 2025), 0);
}

TEST_F(ValidationTest, validateDate_ValidAndInvalid)
{
    EXPECT_EQ(validateDate(29, 2, 2024), 1);
    EXPECT_EQ(validateDate(29, 2, 2025), 0);
    EXPECT_EQ(validateDate(31, 4, 2025), 0);
    EXPECT_EQ(validateDate(0, 1, 2025), 0);
    EXPECT_EQ(validateDate(1, 1, MIN_YEAR - 1), 0);
    EXPECT_EQ(validateDate(1, 1, MAX_YEAR + 1), 0);
    EXPECT_EQ(validateDate(1, 1, MIN_YEAR), 1);
}

TEST_F(ValidationTest, validateStatus_OnlyPAL)
{
    EXPECT_EQ(validateStatus('P'), 1);
    EXPECT_EQ(validateStatus('A'), 1);
    EXPECT_EQ(validateStatus('L'), 1);
    EXPECT_EQ(validateStatus('p'), 0);
    EXPECT_EQ(validateStatus('X'), 0);
}

TEST_F(ValidationTest, containsIgnoreCase_FindsSubstring)
{
    EXPECT_EQ(containsIgnoreCase("Ravi Kumar", "KUM"), 1);
    EXPECT_EQ(containsIgnoreCase("Ravi Kumar", ""), 1);
    EXPECT_EQ(containsIgnoreCase("Ravi", "Ravi Kumar"), 0);
    EXPECT_EQ(containsIgnoreCase("Ravi", "xyz"), 0);
    EXPECT_EQ(containsIgnoreCase(nullptr, "a"), 0);
    EXPECT_EQ(containsIgnoreCase("a", nullptr), 0);
}

TEST_F(ValidationTest, equalsIgnoreCase_WholeStringOnly)
{
    EXPECT_EQ(equalsIgnoreCase("Finance", "fINANCE"), 1);
    EXPECT_EQ(equalsIgnoreCase("Finance", "Fin"), 0);
    EXPECT_EQ(equalsIgnoreCase(nullptr, "x"), 0);
}

TEST_F(ValidationTest, getToday_ReturnsRealDate)
{
    Date d;
    getToday(&d);
    EXPECT_EQ(validateDate(d.day, d.month, d.year), 1);
    getToday(nullptr);
}

TEST_F(ValidationTest, isFutureDate_PastTodayFuture)
{
    Date today;
    getToday(&today);
    Date past = makeDate(1, 1, 2020);
    Date future = makeDate(1, 1, MAX_YEAR);
    Date nextDay = today;
    nextDay.day = today.day + 1;
    Date nextMonth = today;
    nextMonth.month = today.month + 1;
    EXPECT_EQ(isFutureDate(&past), 0);
    EXPECT_EQ(isFutureDate(&today), 0);
    EXPECT_EQ(isFutureDate(&future), 1);
    EXPECT_EQ(isFutureDate(&nextDay), 1);
    EXPECT_EQ(isFutureDate(&nextMonth), 1);
    EXPECT_EQ(isFutureDate(nullptr), 0);
}

TEST_F(ValidationTest, isFutureMonth_PastCurrentFuture)
{
    Date today;
    getToday(&today);
    EXPECT_EQ(isFutureMonth(today.month, today.year), 0);
    EXPECT_EQ(isFutureMonth(1, 2020), 0);
    EXPECT_EQ(isFutureMonth(today.month + 1, today.year), 1);
    EXPECT_EQ(isFutureMonth(1, MAX_YEAR), 1);
}

TEST_F(ValidationTest, validateTimestamp_AcceptsRealDateTime)
{
    EXPECT_EQ(validateTimestamp("2025-01-01 09:00:00"), 1);
    EXPECT_EQ(validateTimestamp("2024-02-29 23:59:59"), 1);
}

TEST_F(ValidationTest, validateTimestamp_RejectsBadShapesAndValues)
{
    EXPECT_EQ(validateTimestamp(nullptr), 0);
    EXPECT_EQ(validateTimestamp(""), 0);
    EXPECT_EQ(validateTimestamp("2025-01-01"), 0);
    EXPECT_EQ(validateTimestamp("2025/01/01 09:00:00"), 0);
    EXPECT_EQ(validateTimestamp("2025-01-01T09:00:00"), 0);
    EXPECT_EQ(validateTimestamp("2025-0a-01 09:00:00"), 0);
    EXPECT_EQ(validateTimestamp("2025-02-30 09:00:00"), 0);
    EXPECT_EQ(validateTimestamp("2025-01-01 24:00:00"), 0);
    EXPECT_EQ(validateTimestamp("2025-01-01 09:60:00"), 0);
    EXPECT_EQ(validateTimestamp("2025-01-01 09:00:60"), 0);
    EXPECT_EQ(validateTimestamp("2025-01-01 09:00:000"), 0);
}

TEST_F(ValidationTest, digitsValue_ConvertsDigitRun)
{
    EXPECT_EQ(digitsValue("2025-03-07", 0U, 4U), 2025);
    EXPECT_EQ(digitsValue("2025-03-07", 5U, 2U), 3);
    EXPECT_EQ(digitsValue("2025-03-07", 8U, 2U), 7);
}

/* ========================================================================= */
/* Session & Auth Module (auth.c)                                            */
/* ========================================================================= */

TEST_F(AuthTest, authReset_ClearsUsersAndSession)
{
    ASSERT_EQ(addUserRecord("admin", "admin123", ROLE_ADMIN), EMS_OK);
    authReset();
    EXPECT_EQ(getUserCount(), 0);
    EXPECT_EQ(getCurrentRole(), ROLE_NONE);
}

TEST_F(AuthTest, hashPassword_IsDeterministicAndSalted)
{
    EXPECT_EQ(hashPassword("admin", "admin123"), hashPassword("admin", "admin123"));
    EXPECT_EQ(hashPassword("ADMIN", "admin123"), hashPassword("admin", "admin123"));
    EXPECT_NE(hashPassword("admin", "admin123"), hashPassword("admin", "admin124"));
    EXPECT_NE(hashPassword("admin", "secret11"), hashPassword("hr_user", "secret11"));
}

TEST_F(AuthTest, hashPassword_FitsIn32BitsAndHandlesNull)
{
    EXPECT_LE(hashPassword("someone", repeatChar(29, 'z')), 0xFFFFFFFFUL);
    EXPECT_EQ(hashPassword(nullptr, "x"), 5381UL);
    EXPECT_EQ(hashPassword("x", nullptr), 5381UL);
}

TEST_F(AuthTest, findUser_CaseInsensitive)
{
    ASSERT_EQ(addUserRecord("Admin", "admin123", ROLE_ADMIN), EMS_OK);
    EXPECT_EQ(findUser("admin"), 0);
    EXPECT_EQ(findUser("ADMIN"), 0);
    EXPECT_EQ(findUser("nobody"), -1);
    EXPECT_EQ(findUser(nullptr), -1);
}

TEST_F(AuthTest, addUserRecord_ValidUser)
{
    EXPECT_EQ(addUserRecord("hr_user", "secret11", ROLE_OPERATOR), EMS_OK);
    EXPECT_EQ(getUserCount(), 1);
    EXPECT_EQ(getUserAt(0)->role, ROLE_OPERATOR);
}

TEST_F(AuthTest, addUserRecord_RejectsBadInput)
{
    EXPECT_EQ(addUserRecord("ab", "secret11", ROLE_ADMIN), ERR_INVALID_INPUT);
    EXPECT_EQ(addUserRecord("valid_user", "123", ROLE_ADMIN), ERR_INVALID_INPUT);
    EXPECT_EQ(addUserRecord("valid_user", "secret11", 7), ERR_INVALID_INPUT);
    EXPECT_EQ(getUserCount(), 0);
}

TEST_F(AuthTest, addUserRecord_RejectsDuplicate)
{
    ASSERT_EQ(addUserRecord("admin", "admin123", ROLE_ADMIN), EMS_OK);
    EXPECT_EQ(addUserRecord("ADMIN", "other123", ROLE_OPERATOR), ERR_DUPLICATE_ID);
}

TEST_F(AuthTest, addUserRecord_LimitReached)
{
    char name[16];
    for (int i = 0; i < MAX_USERS; i++) {
        snprintf(name, sizeof(name), "user%02d", i);
        ASSERT_EQ(addUserRecord(name, "secret11", ROLE_OPERATOR), EMS_OK);
    }
    EXPECT_EQ(addUserRecord("oneMore", "secret11", ROLE_OPERATOR), ERR_LIMIT_REACHED);
}

TEST_F(AuthTest, addUserHashed_StoresGivenHash)
{
    EXPECT_EQ(addUserHashed("loaded", 12345UL, ROLE_ADMIN, nullptr), EMS_OK);
    EXPECT_EQ(getUserAt(0)->passwordHash, 12345UL);
    EXPECT_EQ(addUserHashed("x", 1UL, ROLE_ADMIN, nullptr), ERR_INVALID_INPUT);
}

TEST_F(AuthTest, verifyCredentials_RightAndWrongPassword)
{
    ASSERT_EQ(addUserRecord("admin", "admin123", ROLE_ADMIN), EMS_OK);
    ASSERT_EQ(addUserRecord("hr_user", "secret11", ROLE_OPERATOR), EMS_OK);
    EXPECT_EQ(verifyCredentials("admin", "admin123"), ROLE_ADMIN);
    EXPECT_EQ(verifyCredentials("HR_USER", "secret11"), ROLE_OPERATOR);
    EXPECT_EQ(verifyCredentials("admin", "Admin123"), ROLE_NONE);
    EXPECT_EQ(verifyCredentials("ghost", "admin123"), ROLE_NONE);
    EXPECT_EQ(verifyCredentials("admin", nullptr), ROLE_NONE);
}

TEST_F(AuthTest, setSession_ValidAndInvalidRole)
{
    setSession("hr_user", ROLE_OPERATOR);
    EXPECT_STREQ(getCurrentUser(), "hr_user");
    EXPECT_EQ(getCurrentRole(), ROLE_OPERATOR);
    setSession("x", 9);
    EXPECT_EQ(getCurrentRole(), ROLE_NONE);
    setSession(nullptr, ROLE_ADMIN);
    EXPECT_STREQ(getCurrentUser(), "-");
}

TEST_F(AuthTest, logout_ClearsSession)
{
    logout();
    EXPECT_EQ(getCurrentRole(), ROLE_NONE);
    EXPECT_STREQ(getCurrentUser(), "-");
}

TEST_F(AuthTest, getCurrentRole_ReflectsSession)
{
    EXPECT_EQ(getCurrentRole(), ROLE_ADMIN);
}

TEST_F(AuthTest, getCurrentUser_DashWhenLoggedOut)
{
    EXPECT_STREQ(getCurrentUser(), "admin");
    logout();
    EXPECT_STREQ(getCurrentUser(), "-");
}

TEST_F(AuthTest, roleName_AllRoles)
{
    EXPECT_STREQ(roleName(ROLE_ADMIN), "Administrator");
    EXPECT_STREQ(roleName(ROLE_OPERATOR), "HR Operator");
    EXPECT_STREQ(roleName(ROLE_NONE), "Unknown");
}

TEST_F(AuthTest, getUserCount_CountsUsers)
{
    EXPECT_EQ(getUserCount(), 0);
    ASSERT_EQ(addUserRecord("admin", "admin123", ROLE_ADMIN), EMS_OK);
    EXPECT_EQ(getUserCount(), 1);
}

TEST_F(AuthTest, getUserAt_Bounds)
{
    ASSERT_EQ(addUserRecord("admin", "admin123", ROLE_ADMIN), EMS_OK);
    EXPECT_NE(getUserAt(0), nullptr);
    EXPECT_EQ(getUserAt(1), nullptr);
    EXPECT_EQ(getUserAt(-1), nullptr);
}

TEST_F(AuthTest, checkAccess_AdminCanDoEverything)
{
    EXPECT_EQ(checkAccess(ROLE_ADMIN), 1);
    EXPECT_EQ(checkAccess(ROLE_OPERATOR), 1);
}

TEST_F(AuthTest, checkAccess_OperatorBlockedFromAdminTasks)
{
    setSession("hr_user", ROLE_OPERATOR);
    EXPECT_EQ(checkAccess(ROLE_OPERATOR), 1);
    EXPECT_EQ(checkAccess(ROLE_ADMIN), 0);
    EXPECT_CONTAINS(getOutput(), "E010: Access denied for your role.");
    EXPECT_CONTAINS(logText(), "E010");
}

TEST_F(AuthTest, checkAccess_NobodyLoggedIn)
{
    logout();
    EXPECT_EQ(checkAccess(ROLE_OPERATOR), 0);
}

TEST_F(AuthTest, loadUsers_MissingFileMeansNoUsers)
{
    EXPECT_EQ(loadUsers(), EMS_OK);
    EXPECT_EQ(getUserCount(), 0);
    EXPECT_CONTAINS(logText(), "first-run setup needed");
}

TEST_F(AuthTest, loadUsers_ReadsValidAndSkipsDamagedLines)
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

TEST_F(AuthTest, loadUsers_MissingFolderTreatedAsFirstRun)
{
    ASSERT_EQ(setDataDirectory("/no/such/folder"), EMS_OK);
    EXPECT_EQ(loadUsers(), EMS_OK);
    EXPECT_EQ(getUserCount(), 0);
}

TEST_F(AuthTest, saveUsers_WritesHashNotPassword)
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

TEST_F(AuthTest, saveUsers_FailsWhenFolderMissing)
{
    ASSERT_EQ(setDataDirectory("/no/such/folder"), EMS_OK);
    EXPECT_EQ(saveUsers(), ERR_FILE);
}

TEST_F(AuthTest, readNewPassword_MatchingPasswords)
{
    char pw[PASSWORD_SIZE];
    feed("secret11\nsecret11\n");
    EXPECT_EQ(readNewPassword(pw, sizeof(pw)), EMS_OK);
    EXPECT_STREQ(pw, "secret11");
}

TEST_F(AuthTest, readNewPassword_RetriesOnMismatchAndWeak)
{
    char pw[PASSWORD_SIZE];
    feed("123\nsecret11\nsecret12\nsecret11\nsecret11\n");
    EXPECT_EQ(readNewPassword(pw, sizeof(pw)), EMS_OK);
    const char *out = getOutput();
    EXPECT_CONTAINS(out, "Password must be 6-29 characters");
    EXPECT_CONTAINS(out, "do not match");
}

TEST_F(AuthTest, readNewPassword_EndOfInput)
{
    char pw[PASSWORD_SIZE];
    feed("secret11\n");
    EXPECT_EQ(readNewPassword(pw, sizeof(pw)), ERR_END_OF_INPUT);
}

TEST_F(AuthTest, createFirstAdmin_CreatesAndSaves)
{
    feed("boss\nadmin123\nadmin123\n");
    EXPECT_EQ(createFirstAdmin(), EMS_OK);
    EXPECT_EQ(verifyCredentials("boss", "admin123"), ROLE_ADMIN);
    EXPECT_TRUE(fileExists("users.txt"));
}

TEST_F(AuthTest, createFirstAdmin_InvalidUsernameRetriedThenEof)
{
    feed("a b\n");
    EXPECT_EQ(createFirstAdmin(), ERR_END_OF_INPUT);
    EXPECT_EQ(getUserCount(), 0);
}

TEST_F(AuthTest, login_SuccessSetsSession)
{
    logout();
    ASSERT_EQ(addUserRecord("hr_user", "secret11", ROLE_OPERATOR), EMS_OK);
    feed("HR_USER\nsecret11\n");
    EXPECT_EQ(login(), EMS_OK);
    EXPECT_STREQ(getCurrentUser(), "hr_user");
    EXPECT_EQ(getCurrentRole(), ROLE_OPERATOR);
    EXPECT_CONTAINS(logText(), "hr_user logged in as HR Operator");
}

TEST_F(AuthTest, login_WrongThenRight)
{
    logout();
    ASSERT_EQ(addUserRecord("admin", "admin123", ROLE_ADMIN), EMS_OK);
    feed("admin\nwrong999\nadmin\nadmin123\n");
    EXPECT_EQ(login(), EMS_OK);
    EXPECT_CONTAINS(getOutput(), "E009");
    EXPECT_CONTAINS(logText(), "failed login for 'admin' (attempt 1)");
}

TEST_F(AuthTest, login_ThreeFailuresGiveE009)
{
    logout();
    ASSERT_EQ(addUserRecord("admin", "admin123", ROLE_ADMIN), EMS_OK);
    feed("admin\nx\nadmin\ny\nadmin\nz\nadmin\nadmin123\n");
    EXPECT_EQ(login(), ERR_LOGIN);
    EXPECT_EQ(getCurrentRole(), ROLE_NONE);
    EXPECT_CONTAINS(getOutput(), "Too many failed attempts");
}

TEST_F(AuthTest, login_FirstRunCreatesAdminThenLogsIn)
{
    logout();
    feed("boss\nadmin123\nadmin123\nboss\nadmin123\n");
    EXPECT_EQ(login(), EMS_OK);
    EXPECT_EQ(getCurrentRole(), ROLE_ADMIN);
}

TEST_F(AuthTest, login_TooLongUsernameCountsAsFailure)
{
    logout();
    ASSERT_EQ(addUserRecord("admin", "admin123", ROLE_ADMIN), EMS_OK);
    char text[100];
    (void)snprintf(text, sizeof(text), "%s\nadmin\nadmin123\n", repeatChar(60, 'a'));
    feed(text);
    EXPECT_EQ(login(), EMS_OK);
    EXPECT_EQ(getCurrentRole(), ROLE_ADMIN);
}

TEST_F(AuthTest, login_EndOfInput)
{
    logout();
    ASSERT_EQ(addUserRecord("admin", "admin123", ROLE_ADMIN), EMS_OK);
    feed("admin\n");
    EXPECT_EQ(login(), ERR_END_OF_INPUT);
}

TEST_F(AuthTest, addUser_CreatesOperator)
{
    feed("hr_user\nsecret11\nsecret11\n2\n");
    EXPECT_EQ(addUser(), EMS_OK);
    EXPECT_EQ(verifyCredentials("hr_user", "secret11"), ROLE_OPERATOR);
    EXPECT_CONTAINS(logText(), "User hr_user (HR Operator) created by admin");
}

TEST_F(AuthTest, addUser_DuplicateUsername)
{
    ASSERT_EQ(addUserRecord("hr_user", "secret11", ROLE_OPERATOR), EMS_OK);
    feed("HR_user\n");
    EXPECT_EQ(addUser(), ERR_DUPLICATE_ID);
    EXPECT_CONTAINS(getOutput(), "E002");
}

TEST_F(AuthTest, addUser_LimitReached)
{
    char name[16];
    for (int i = 0; i < MAX_USERS; i++) {
        snprintf(name, sizeof(name), "user%02d", i);
        ASSERT_EQ(addUserRecord(name, "secret11", ROLE_OPERATOR), EMS_OK);
    }
    feed("newbie\nsecret11\nsecret11\n1\n");
    EXPECT_EQ(addUser(), ERR_LIMIT_REACHED);
}

TEST_F(AuthTest, addUser_EndOfInput)
{
    feed("");
    EXPECT_EQ(addUser(), ERR_END_OF_INPUT);
}

TEST_F(AuthTest, addUserHashed_TimestampFromFileOrRejected)
{
    EXPECT_EQ(addUserHashed("loaded", 1UL, ROLE_ADMIN, "2025-01-01 09:00:00"), EMS_OK);
    EXPECT_STREQ(getUserAt(0)->createdAt, "2025-01-01 09:00:00");
    EXPECT_EQ(addUserHashed("other", 1UL, ROLE_ADMIN, "bad"), ERR_INVALID_INPUT);
}

TEST_F(AuthTest, logout_LogsWhoLoggedOut)
{
    logout();
    EXPECT_CONTAINS(logText(), "[AUTH] admin logged out");
}

/* ========================================================================= */
/* Department Module (department.c)                                          */
/* ========================================================================= */

TEST_F(DepartmentTest, deptReset_EmptiesTable)
{
    ASSERT_EQ(departmentAdd(10, "Finance", nullptr), EMS_OK);
    deptReset();
    EXPECT_EQ(getDeptCount(), 0);
    EXPECT_EQ(findDept(10), -1);
}

TEST_F(DepartmentTest, departmentAdd_Valid)
{
    EXPECT_EQ(departmentAdd(10, "Finance", nullptr), EMS_OK);
    EXPECT_EQ(getDeptCount(), 1);
    EXPECT_STREQ(getDeptAt(0)->name, "Finance");
}

TEST_F(DepartmentTest, departmentAdd_InvalidIdOrName)
{
    EXPECT_EQ(departmentAdd(0, "Finance", nullptr), ERR_INVALID_INPUT);
    EXPECT_EQ(departmentAdd(MAX_ID + 1, "Finance", nullptr), ERR_INVALID_INPUT);
    EXPECT_EQ(departmentAdd(10, "", nullptr), ERR_INVALID_INPUT);
    EXPECT_EQ(departmentAdd(10, "Fin|ance", nullptr), ERR_INVALID_INPUT);
    EXPECT_EQ(departmentAdd(10, nullptr, nullptr), ERR_INVALID_INPUT);
    EXPECT_EQ(departmentAdd(10, repeatChar(TITLE_SIZE, 'x'), nullptr), ERR_INVALID_INPUT);
    EXPECT_EQ(departmentAdd(10, repeatChar(TITLE_SIZE - 1, 'x'), nullptr), EMS_OK);
}

TEST_F(DepartmentTest, departmentAdd_DuplicateIdOrName)
{
    ASSERT_EQ(departmentAdd(10, "Finance", nullptr), EMS_OK);
    EXPECT_EQ(departmentAdd(10, "Sales", nullptr), ERR_DUPLICATE_ID);
    EXPECT_EQ(departmentAdd(11, "FINANCE", nullptr), ERR_DUPLICATE_ID);
}

TEST_F(DepartmentTest, departmentAdd_LimitReached)
{
    char name[16];
    for (int i = 1; i <= MAX_DEPARTMENTS; i++) {
        snprintf(name, sizeof(name), "Dept %d", i);
        ASSERT_EQ(departmentAdd(i, name, nullptr), EMS_OK);
    }
    EXPECT_EQ(departmentAdd(5000, "Extra", nullptr), ERR_LIMIT_REACHED);
}

TEST_F(DepartmentTest, findDept_FoundAndMissing)
{
    ASSERT_EQ(departmentAdd(10, "Finance", nullptr), EMS_OK);
    ASSERT_EQ(departmentAdd(11, "HR", nullptr), EMS_OK);
    EXPECT_EQ(findDept(11), 1);
    EXPECT_EQ(findDept(99), -1);
}

TEST_F(DepartmentTest, findDeptByName_CaseInsensitive)
{
    ASSERT_EQ(departmentAdd(10, "Finance", nullptr), EMS_OK);
    EXPECT_EQ(findDeptByName("finance"), 0);
    EXPECT_EQ(findDeptByName("Fin"), -1);
}

TEST_F(DepartmentTest, getDeptName_KnownAndUnknown)
{
    ASSERT_EQ(departmentAdd(10, "Finance", nullptr), EMS_OK);
    EXPECT_STREQ(getDeptName(10), "Finance");
    EXPECT_STREQ(getDeptName(99), "(none)");
}

TEST_F(DepartmentTest, getDeptCount_Counts)
{
    EXPECT_EQ(getDeptCount(), 0);
    ASSERT_EQ(departmentAdd(10, "Finance", nullptr), EMS_OK);
    EXPECT_EQ(getDeptCount(), 1);
}

TEST_F(DepartmentTest, getDeptAt_Bounds)
{
    ASSERT_EQ(departmentAdd(10, "Finance", nullptr), EMS_OK);
    EXPECT_NE(getDeptAt(0), nullptr);
    EXPECT_EQ(getDeptAt(1), nullptr);
    EXPECT_EQ(getDeptAt(-1), nullptr);
}

TEST_F(DepartmentTest, viewDepartments_EmptyMessage)
{
    EXPECT_EQ(viewDepartments(), 0);
    EXPECT_CONTAINS(getOutput(), "No departments yet.");
}

TEST_F(DepartmentTest, viewDepartments_ListsWithEmployeeCount)
{
    ASSERT_EQ(test_seed_data(), EMS_OK);
    EXPECT_EQ(viewDepartments(), 2);
    const char *out = getOutput();
    EXPECT_CONTAINS(out, "Finance");
    EXPECT_CONTAINS(out, "HR");
}

TEST_F(DepartmentTest, createDepartment_AddsSavesAndLogs)
{
    feed("10\nFinance\n");
    EXPECT_EQ(createDepartment(), EMS_OK);
    EXPECT_CONTAINS(readFile("departments.txt"), "10|Finance|");
    EXPECT_EQ(validateTimestamp(getDeptAt(0)->createdAt), 1);
    EXPECT_CONTAINS(logText(), "[DEPARTMENT] Department 10 (Finance) created by admin");
}

TEST_F(DepartmentTest, createDepartment_DuplicateShowsE002)
{
    ASSERT_EQ(departmentAdd(10, "Finance", nullptr), EMS_OK);
    feed("10\nSales\n");
    EXPECT_EQ(createDepartment(), ERR_DUPLICATE_ID);
    EXPECT_CONTAINS(getOutput(), "E002");
}

TEST_F(DepartmentTest, createDepartment_LimitReached)
{
    char name[16];
    for (int i = 1; i <= MAX_DEPARTMENTS; i++) {
        snprintf(name, sizeof(name), "Dept %d", i);
        ASSERT_EQ(departmentAdd(i, name, nullptr), EMS_OK);
    }
    feed("5000\nExtra\n");
    EXPECT_EQ(createDepartment(), ERR_LIMIT_REACHED);
    EXPECT_CONTAINS(getOutput(), "Department limit of 100 reached.");
}

TEST_F(DepartmentTest, createDepartment_EndOfInput)
{
    feed("10\n");
    EXPECT_EQ(createDepartment(), ERR_END_OF_INPUT);
    EXPECT_EQ(getDeptCount(), 0);
}

TEST_F(DepartmentTest, departmentMenu_CreateViewBack)
{
    feed("1\n10\nFinance\n2\n9\n0\n");
    departmentMenu();
    EXPECT_EQ(getDeptCount(), 1);
    const char *out = getOutput();
    EXPECT_CONTAINS(out, "Department created.");
    EXPECT_CONTAINS(out, "Enter a number from 0 to 2.");
}

TEST_F(DepartmentTest, departmentMenu_EndOfInputInsideCreate)
{
    feed("1\n10\n");
    departmentMenu();
    EXPECT_EQ(getDeptCount(), 0);
}

TEST_F(DepartmentTest, departmentAdd_TimestampFromCallerOrNow)
{
    EXPECT_EQ(departmentAdd(10, "Finance", "2025-01-01 09:00:00"), EMS_OK);
    EXPECT_STREQ(getDeptAt(0)->createdAt, "2025-01-01 09:00:00");
    EXPECT_EQ(departmentAdd(11, "HR", nullptr), EMS_OK);
    EXPECT_EQ(validateTimestamp(getDeptAt(1)->createdAt), 1);
    EXPECT_EQ(departmentAdd(12, "Sales", "bad time"), ERR_INVALID_INPUT);
    EXPECT_EQ(getDeptCount(), 2);
}

/* ========================================================================= */
/* Role Module (role.c)                                                      */
/* ========================================================================= */

TEST_F(RoleTest, roleReset_EmptiesTable)
{
    ASSERT_EQ(roleAdd(10, "Accountant", nullptr), EMS_OK);
    roleReset();
    EXPECT_EQ(getRoleCount(), 0);
    EXPECT_EQ(findRole(10), -1);
}

TEST_F(RoleTest, roleAdd_Valid)
{
    EXPECT_EQ(roleAdd(10, "Accountant", nullptr), EMS_OK);
    EXPECT_EQ(getRoleCount(), 1);
    EXPECT_STREQ(getRoleAt(0)->title, "Accountant");
}

TEST_F(RoleTest, roleAdd_InvalidIdOrName)
{
    EXPECT_EQ(roleAdd(0, "Accountant", nullptr), ERR_INVALID_INPUT);
    EXPECT_EQ(roleAdd(MAX_ID + 1, "Accountant", nullptr), ERR_INVALID_INPUT);
    EXPECT_EQ(roleAdd(10, "", nullptr), ERR_INVALID_INPUT);
    EXPECT_EQ(roleAdd(10, "Acc|ountant", nullptr), ERR_INVALID_INPUT);
    EXPECT_EQ(roleAdd(10, nullptr, nullptr), ERR_INVALID_INPUT);
    EXPECT_EQ(roleAdd(10, repeatChar(TITLE_SIZE, 'x'), nullptr), ERR_INVALID_INPUT);
    EXPECT_EQ(roleAdd(10, repeatChar(TITLE_SIZE - 1, 'x'), nullptr), EMS_OK);
}

TEST_F(RoleTest, roleAdd_DuplicateIdOrName)
{
    ASSERT_EQ(roleAdd(10, "Accountant", nullptr), EMS_OK);
    EXPECT_EQ(roleAdd(10, "Clerk", nullptr), ERR_DUPLICATE_ID);
    EXPECT_EQ(roleAdd(11, "ACCOUNTANT", nullptr), ERR_DUPLICATE_ID);
}

TEST_F(RoleTest, roleAdd_LimitReached)
{
    char name[16];
    for (int i = 1; i <= MAX_ROLES; i++) {
        snprintf(name, sizeof(name), "Role %d", i);
        ASSERT_EQ(roleAdd(i, name, nullptr), EMS_OK);
    }
    EXPECT_EQ(roleAdd(5000, "Extra", nullptr), ERR_LIMIT_REACHED);
}

TEST_F(RoleTest, findRole_FoundAndMissing)
{
    ASSERT_EQ(roleAdd(10, "Accountant", nullptr), EMS_OK);
    ASSERT_EQ(roleAdd(11, "HR", nullptr), EMS_OK);
    EXPECT_EQ(findRole(11), 1);
    EXPECT_EQ(findRole(99), -1);
}

TEST_F(RoleTest, findRoleByTitle_CaseInsensitive)
{
    ASSERT_EQ(roleAdd(10, "Accountant", nullptr), EMS_OK);
    EXPECT_EQ(findRoleByTitle("accountant"), 0);
    EXPECT_EQ(findRoleByTitle("Acc"), -1);
}

TEST_F(RoleTest, getRoleTitle_KnownAndUnknown)
{
    ASSERT_EQ(roleAdd(10, "Accountant", nullptr), EMS_OK);
    EXPECT_STREQ(getRoleTitle(10), "Accountant");
    EXPECT_STREQ(getRoleTitle(99), "(none)");
}

TEST_F(RoleTest, getRoleCount_Counts)
{
    EXPECT_EQ(getRoleCount(), 0);
    ASSERT_EQ(roleAdd(10, "Accountant", nullptr), EMS_OK);
    EXPECT_EQ(getRoleCount(), 1);
}

TEST_F(RoleTest, getRoleAt_Bounds)
{
    ASSERT_EQ(roleAdd(10, "Accountant", nullptr), EMS_OK);
    EXPECT_NE(getRoleAt(0), nullptr);
    EXPECT_EQ(getRoleAt(1), nullptr);
    EXPECT_EQ(getRoleAt(-1), nullptr);
}

TEST_F(RoleTest, viewRoles_EmptyMessage)
{
    EXPECT_EQ(viewRoles(), 0);
    EXPECT_CONTAINS(getOutput(), "No roles yet.");
}

TEST_F(RoleTest, viewRoles_ListsWithEmployeeCount)
{
    ASSERT_EQ(test_seed_data(), EMS_OK);
    EXPECT_EQ(viewRoles(), 2);
    const char *out = getOutput();
    EXPECT_CONTAINS(out, "Accountant");
    EXPECT_CONTAINS(out, "Executive");
}

TEST_F(RoleTest, createRole_AddsSavesAndLogs)
{
    feed("10\nAccountant\n");
    EXPECT_EQ(createRole(), EMS_OK);
    EXPECT_CONTAINS(readFile("roles.txt"), "10|Accountant|");
    EXPECT_EQ(validateTimestamp(getRoleAt(0)->createdAt), 1);
    EXPECT_CONTAINS(logText(), "[ROLE] Role 10 (Accountant) defined by admin");
}

TEST_F(RoleTest, createRole_DuplicateShowsE002)
{
    ASSERT_EQ(roleAdd(10, "Accountant", nullptr), EMS_OK);
    feed("10\nClerk\n");
    EXPECT_EQ(createRole(), ERR_DUPLICATE_ID);
    EXPECT_CONTAINS(getOutput(), "E002");
}

TEST_F(RoleTest, createRole_LimitReached)
{
    char name[16];
    for (int i = 1; i <= MAX_ROLES; i++) {
        snprintf(name, sizeof(name), "Role %d", i);
        ASSERT_EQ(roleAdd(i, name, nullptr), EMS_OK);
    }
    feed("5000\nExtra\n");
    EXPECT_EQ(createRole(), ERR_LIMIT_REACHED);
    EXPECT_CONTAINS(getOutput(), "Role limit of 100 reached.");
}

TEST_F(RoleTest, createRole_EndOfInput)
{
    feed("10\n");
    EXPECT_EQ(createRole(), ERR_END_OF_INPUT);
    EXPECT_EQ(getRoleCount(), 0);
}

TEST_F(RoleTest, roleMenu_CreateViewBack)
{
    feed("1\n10\nAccountant\n2\n9\n0\n");
    roleMenu();
    EXPECT_EQ(getRoleCount(), 1);
    const char *out = getOutput();
    EXPECT_CONTAINS(out, "Role defined.");
    EXPECT_CONTAINS(out, "Enter a number from 0 to 2.");
}

TEST_F(RoleTest, roleMenu_EndOfInputInsideCreate)
{
    feed("1\n10\n");
    roleMenu();
    EXPECT_EQ(getRoleCount(), 0);
}

TEST_F(RoleTest, roleAdd_TimestampFromCallerOrNow)
{
    EXPECT_EQ(roleAdd(20, "Accountant", "2025-01-01 09:00:00"), EMS_OK);
    EXPECT_STREQ(getRoleAt(0)->createdAt, "2025-01-01 09:00:00");
    EXPECT_EQ(roleAdd(21, "Executive", "bad time"), ERR_INVALID_INPUT);
    EXPECT_EQ(getRoleCount(), 1);
}

/* ========================================================================= */
/* Employee Module (employee.c)                                              */
/* ========================================================================= */

TEST_F(EmployeeTest, employeeReset_FreesWholeList)
{
    employeeReset();
    EXPECT_EQ(getEmployeeCount(), 0);
    EXPECT_EQ(getEmployeeHead(), nullptr);
}

TEST_F(EmployeeTest, validateEmployeeData_ValidRecord)
{
    Employee e = makeEmployee(200);
    EXPECT_EQ(validateEmployeeData(&e), EMS_OK);
}

TEST_F(EmployeeTest, validateEmployeeData_EachBadField)
{
    Employee e = makeEmployee(200);
    EXPECT_EQ(validateEmployeeData(nullptr), ERR_INVALID_INPUT);
    e.id = 0;
    EXPECT_EQ(validateEmployeeData(&e), ERR_INVALID_INPUT);
    e = makeEmployeeFull(200, "R2D2", 10, 20, 30000.0);
    EXPECT_EQ(validateEmployeeData(&e), ERR_INVALID_INPUT);
    e = makeEmployee(200);
    strcpy(e.phone, "12345");
    EXPECT_EQ(validateEmployeeData(&e), ERR_INVALID_INPUT);
    e = makeEmployee(200);
    strcpy(e.email, "no-at-sign");
    EXPECT_EQ(validateEmployeeData(&e), ERR_INVALID_INPUT);
    e = makeEmployeeFull(200, "Ravi", 10, 20, 0.0);
    EXPECT_EQ(validateEmployeeData(&e), ERR_INVALID_INPUT);
}

TEST_F(EmployeeTest, validateEmployeeData_MissingDepartmentOrRole)
{
    Employee e = makeEmployeeFull(200, "Ravi", 99, 20, 30000.0);
    EXPECT_EQ(validateEmployeeData(&e), ERR_DEPT_ROLE_NOT_FOUND);
    e = makeEmployeeFull(200, "Ravi", 10, 99, 30000.0);
    EXPECT_EQ(validateEmployeeData(&e), ERR_DEPT_ROLE_NOT_FOUND);
}

TEST_F(EmployeeTest, employeeInsert_KeepsListSortedById)
{
    Employee a = makeEmployee(150);
    Employee b = makeEmployee(50);
    ASSERT_EQ(employeeInsert(&a), EMS_OK);
    ASSERT_EQ(employeeInsert(&b), EMS_OK);
    const Employee *n = getEmployeeHead();
    int ids[4];
    for (int i = 0; i < 4; i++) {
        ASSERT_NE(n, nullptr);
        ids[i] = n->id;
        n = n->next;
    }
    EXPECT_EQ(ids[0], 50);
    EXPECT_EQ(ids[1], 101);
    EXPECT_EQ(ids[2], 102);
    EXPECT_EQ(ids[3], 150);
    EXPECT_EQ(getEmployeeCount(), 4);
}

TEST_F(EmployeeTest, employeeInsert_RejectsDuplicateAndInvalid)
{
    Employee dup = makeEmployee(101);
    Employee bad = makeEmployeeFull(300, "Bad", 99, 20, 30000.0);
    EXPECT_EQ(employeeInsert(&dup), ERR_DUPLICATE_ID);
    EXPECT_EQ(employeeInsert(&bad), ERR_DEPT_ROLE_NOT_FOUND);
    EXPECT_EQ(employeeInsert(nullptr), ERR_INVALID_INPUT);
    EXPECT_EQ(getEmployeeCount(), 2);
}

TEST_F(EmployeeTest, findEmployee_FoundAndMissing)
{
    ASSERT_NE(findEmployee(101), nullptr);
    EXPECT_STREQ(findEmployee(102)->name, "Priya Sharma");
    EXPECT_EQ(findEmployee(1), nullptr);
    EXPECT_EQ(findEmployee(999), nullptr);
    employeeReset();
    EXPECT_EQ(findEmployee(101), nullptr);
}

TEST_F(EmployeeTest, employeeUpdate_ChangesFields)
{
    Employee e = makeEmployeeFull(0, "Ravi K", 11, 21, 45000.0);
    strcpy(e.phone, "9000000000");
    EXPECT_EQ(employeeUpdate(101, &e), EMS_OK);
    const Employee *got = findEmployee(101);
    EXPECT_STREQ(got->name, "Ravi K");
    EXPECT_STREQ(got->phone, "9000000000");
    EXPECT_EQ(got->deptId, 11);
    EXPECT_DOUBLE_EQ(got->basicSalary, 45000.0);
    EXPECT_EQ(got->id, 101);
}

TEST_F(EmployeeTest, employeeUpdate_NotFoundNullOrInvalid)
{
    Employee e = makeEmployee(0);
    EXPECT_EQ(employeeUpdate(555, &e), ERR_NOT_FOUND);
    EXPECT_EQ(employeeUpdate(101, nullptr), ERR_INVALID_INPUT);
    strcpy(e.phone, "1");
    EXPECT_EQ(employeeUpdate(101, &e), ERR_INVALID_INPUT);
    EXPECT_STREQ(findEmployee(101)->phone, "9876543210");
}

TEST_F(EmployeeTest, employeeRemove_HeadMiddleTail)
{
    Employee c = makeEmployee(103);
    ASSERT_EQ(employeeInsert(&c), EMS_OK);
    EXPECT_EQ(employeeRemove(102), EMS_OK);
    EXPECT_EQ(employeeRemove(101), EMS_OK);
    EXPECT_EQ(employeeRemove(103), EMS_OK);
    EXPECT_EQ(getEmployeeCount(), 0);
    EXPECT_EQ(employeeRemove(101), ERR_NOT_FOUND);
}

TEST_F(EmployeeTest, employeeRemove_CascadesAttendanceAndPayroll)
{
    Date d = makeDate(3, 1, 2025);
    ASSERT_EQ(attendanceAdd(101, &d, 'P', nullptr), EMS_OK);
    Payroll p;
    ASSERT_EQ(payrollCompute(101, 1, 2025, 0.0, 0.0, &p), EMS_OK);
    ASSERT_EQ(payrollAdd(&p), EMS_OK);
    EXPECT_EQ(employeeRemove(101), EMS_OK);
    EXPECT_EQ(getAttendanceCount(), 0);
    EXPECT_EQ(getPayrollCount(), 0);
}

TEST_F(EmployeeTest, employeeSetDepartment_Cases)
{
    EXPECT_EQ(employeeSetDepartment(101, 11), EMS_OK);
    EXPECT_EQ(findEmployee(101)->deptId, 11);
    EXPECT_EQ(employeeSetDepartment(999, 11), ERR_NOT_FOUND);
    EXPECT_EQ(employeeSetDepartment(101, 99), ERR_DEPT_ROLE_NOT_FOUND);
}

TEST_F(EmployeeTest, employeeSetRole_Cases)
{
    EXPECT_EQ(employeeSetRole(101, 21), EMS_OK);
    EXPECT_EQ(findEmployee(101)->roleId, 21);
    EXPECT_EQ(employeeSetRole(999, 21), ERR_NOT_FOUND);
    EXPECT_EQ(employeeSetRole(101, 99), ERR_DEPT_ROLE_NOT_FOUND);
}

TEST_F(EmployeeTest, countEmployeesInDept_Counts)
{
    EXPECT_EQ(countEmployeesInDept(10), 1);
    EXPECT_EQ(countEmployeesInDept(99), 0);
}

TEST_F(EmployeeTest, countEmployeesInRole_Counts)
{
    EXPECT_EQ(countEmployeesInRole(21), 1);
    EXPECT_EQ(countEmployeesInRole(99), 0);
}

TEST_F(EmployeeTest, getEmployeeHead_ReturnsLowestId)
{
    ASSERT_NE(getEmployeeHead(), nullptr);
    EXPECT_EQ(getEmployeeHead()->id, 101);
}

TEST_F(EmployeeTest, getEmployeeCount_Counts)
{
    EXPECT_EQ(getEmployeeCount(), 2);
}

TEST_F(EmployeeTest, printEmployeeHeader_PrintsColumnsAndNullSafe)
{
    printEmployeeHeader(getOutputStream());
    printEmployeeHeader(nullptr);
    EXPECT_CONTAINS(getOutput(), "Basic Salary");
}

TEST_F(EmployeeTest, printEmployee_ShowsDepartmentAndRoleNames)
{
    printEmployee(getOutputStream(), findEmployee(101));
    printEmployee(getOutputStream(), nullptr);
    printEmployee(nullptr, findEmployee(101));
    const char *out = getOutput();
    EXPECT_CONTAINS(out, "Finance");
    EXPECT_CONTAINS(out, "Accountant");
    EXPECT_CONTAINS(out, "30000.00");
}

TEST_F(EmployeeTest, selectDepartment_RetriesUntilExisting)
{
    int id = 0;
    feed("99\n11\n");
    EXPECT_EQ(selectDepartment(&id), EMS_OK);
    EXPECT_EQ(id, 11);
    EXPECT_CONTAINS(getOutput(), "E006");
}

TEST_F(EmployeeTest, selectDepartment_NoDepartmentsNullAndEof)
{
    int id = 0;
    EXPECT_EQ(selectDepartment(nullptr), ERR_INVALID_INPUT);
    feed("");
    EXPECT_EQ(selectDepartment(&id), ERR_END_OF_INPUT);
    deptReset();
    EXPECT_EQ(selectDepartment(&id), ERR_DEPT_ROLE_NOT_FOUND);
}

TEST_F(EmployeeTest, selectRole_RetriesUntilExisting)
{
    int id = 0;
    feed("5\n21\n");
    EXPECT_EQ(selectRole(&id), EMS_OK);
    EXPECT_EQ(id, 21);
}

TEST_F(EmployeeTest, selectRole_NoRolesAndNull)
{
    int id = 0;
    EXPECT_EQ(selectRole(nullptr), ERR_INVALID_INPUT);
    roleReset();
    EXPECT_EQ(selectRole(&id), ERR_DEPT_ROLE_NOT_FOUND);
}

TEST_F(EmployeeTest, logEmployeeEvent_WritesUserAndId)
{
    logEmployeeEvent("Tested", 101);
    EXPECT_CONTAINS(logText(), "[EMPLOYEE] Tested employee 101 by admin");
}

TEST_F(EmployeeTest, addEmployee_FullValidEntry)
{
    feed("103\nAnil Rao\n9123456780\nanil@corp.com\n10\n20\n28000\n");
    EXPECT_EQ(addEmployee(), EMS_OK);
    ASSERT_NE(findEmployee(103), nullptr);
    EXPECT_CONTAINS(readFile("employees.txt"), "103|Anil Rao|9123456780|anil@corp.com|10|20|28000.00");
    EXPECT_CONTAINS(logText(), "Added employee 103 by admin");
}

TEST_F(EmployeeTest, addEmployee_RetriesBadFields)
{
    feed("103\nAnil 9\nAnil Rao\n123\n9123456780\nbad\nanil@corp.com\n77\n10\n20\n0\n28000\n");
    EXPECT_EQ(addEmployee(), EMS_OK);
    const char *out = getOutput();
    EXPECT_CONTAINS(out, "Use letters");
    EXPECT_CONTAINS(out, "Phone must be exactly 10 digits.");
    EXPECT_CONTAINS(out, "valid email");
    EXPECT_CONTAINS(out, "E006");
}

TEST_F(EmployeeTest, addEmployee_DuplicateId)
{
    feed("101\n");
    EXPECT_EQ(addEmployee(), ERR_DUPLICATE_ID);
    EXPECT_CONTAINS(getOutput(), "E002");
}

TEST_F(EmployeeTest, addEmployee_NoDepartmentsYet)
{
    employeeReset();
    deptReset();
    feed("103\nAnil Rao\n9123456780\nanil@corp.com\n");
    EXPECT_EQ(addEmployee(), ERR_DEPT_ROLE_NOT_FOUND);
    EXPECT_EQ(getEmployeeCount(), 0);
}

TEST_F(EmployeeTest, addEmployee_EndOfInput)
{
    feed("103\nAnil Rao\n");
    EXPECT_EQ(addEmployee(), ERR_END_OF_INPUT);
    EXPECT_EQ(findEmployee(103), nullptr);
}

TEST_F(EmployeeTest, viewEmployee_ListsAll)
{
    EXPECT_EQ(viewEmployee(), 2);
    EXPECT_CONTAINS(getOutput(), "Total employees: 2");
}

TEST_F(EmployeeTest, viewEmployee_EmptyList)
{
    employeeReset();
    EXPECT_EQ(viewEmployee(), 0);
    EXPECT_CONTAINS(getOutput(), "No employees yet.");
}

TEST_F(EmployeeTest, updateEmployee_EachField)
{
    feed("101\n1\nRavi Kumar S\n101\n2\n9000000001\n101\n3\nrks@corp.com\n101\n4\n41000\n");
    EXPECT_EQ(updateEmployee(), EMS_OK);
    EXPECT_EQ(updateEmployee(), EMS_OK);
    EXPECT_EQ(updateEmployee(), EMS_OK);
    EXPECT_EQ(updateEmployee(), EMS_OK);
    const Employee *e = findEmployee(101);
    EXPECT_STREQ(e->name, "Ravi Kumar S");
    EXPECT_STREQ(e->phone, "9000000001");
    EXPECT_STREQ(e->email, "rks@corp.com");
    EXPECT_DOUBLE_EQ(e->basicSalary, 41000.0);
    EXPECT_CONTAINS(readFile("employees.txt"), "41000.00");
}

TEST_F(EmployeeTest, updateEmployee_CancelNotFoundAndEof)
{
    feed("101\n0\n555\n101\n1\n");
    EXPECT_EQ(updateEmployee(), EMS_OK);
    EXPECT_EQ(updateEmployee(), ERR_NOT_FOUND);
    EXPECT_EQ(updateEmployee(), ERR_END_OF_INPUT);
    EXPECT_STREQ(findEmployee(101)->name, "Ravi Kumar");
}

TEST_F(EmployeeTest, deleteEmployee_ConfirmedYes)
{
    feed("102\ny\n");
    EXPECT_EQ(deleteEmployee(), EMS_OK);
    EXPECT_EQ(findEmployee(102), nullptr);
    EXPECT_NOT_CONTAINS(readFile("employees.txt"), "102|");
}

TEST_F(EmployeeTest, deleteEmployee_AnsweredNo)
{
    feed("102\nn\n");
    EXPECT_EQ(deleteEmployee(), EMS_OK);
    EXPECT_NE(findEmployee(102), nullptr);
    EXPECT_CONTAINS(getOutput(), "Delete cancelled.");
}

TEST_F(EmployeeTest, deleteEmployee_NotFound)
{
    feed("999\n");
    EXPECT_EQ(deleteEmployee(), ERR_NOT_FOUND);
}

TEST_F(EmployeeTest, assignDepartment_Changes)
{
    feed("101\n11\n");
    EXPECT_EQ(assignDepartment(), EMS_OK);
    EXPECT_EQ(findEmployee(101)->deptId, 11);
    EXPECT_CONTAINS(getOutput(), "set to HR");
}

TEST_F(EmployeeTest, assignDepartment_EmployeeMissing)
{
    feed("999\n");
    EXPECT_EQ(assignDepartment(), ERR_NOT_FOUND);
}

TEST_F(EmployeeTest, assignRole_Changes)
{
    feed("101\n21\n");
    EXPECT_EQ(assignRole(), EMS_OK);
    EXPECT_EQ(findEmployee(101)->roleId, 21);
}

TEST_F(EmployeeTest, assignRole_EmployeeMissing)
{
    feed("999\n");
    EXPECT_EQ(assignRole(), ERR_NOT_FOUND);
}

TEST_F(EmployeeTest, employeeMenu_AdminRunsEveryOption)
{
    feed("2\n3\n1\n101\n6\n101\n11\n7\n101\n21\n4\n101\n0\n5\n102\ny\n"
         "1\n104\nAnil Rao\n9123456780\nanil@corp.com\n10\n20\n28000\n0\n");
    employeeMenu();
    EXPECT_EQ(findEmployee(102), nullptr);
    EXPECT_NE(findEmployee(104), nullptr);
    EXPECT_EQ(findEmployee(101)->deptId, 11);
    EXPECT_EQ(findEmployee(101)->roleId, 21);
}

TEST_F(EmployeeTest, employeeMenu_OperatorBlockedFromAdminOptions)
{
    setSession("hr_user", ROLE_OPERATOR);
    feed("5\n6\n7\n0\n");
    employeeMenu();
    EXPECT_NE(findEmployee(102), nullptr);
    EXPECT_EQ(countOccurrences(getOutput(), "E010"), 3);
}

TEST_F(EmployeeTest, employeeMenu_EndOfInputInsideAction)
{
    feed("1\n105\n");
    employeeMenu();
    EXPECT_EQ(findEmployee(105), nullptr);
}

TEST_F(EmployeeTest, employeeInsert_StampsCreatedAndUpdatedTime)
{
    Employee e = makeEmployee(150);
    ASSERT_EQ(employeeInsert(&e), EMS_OK);
    EXPECT_EQ(validateTimestamp(findEmployee(150)->createdAt), 1);
    EXPECT_STREQ(findEmployee(150)->updatedAt, findEmployee(150)->createdAt);
}

TEST_F(EmployeeTest, employeeInsert_KeepsTimeFromFileAndRejectsBadTime)
{
    Employee e = makeEmployee(150);
    Employee bad = makeEmployee(151);
    (void)snprintf(e.createdAt, sizeof(e.createdAt), "%s", "2025-01-01 09:00:00");
    (void)snprintf(bad.createdAt, sizeof(bad.createdAt), "%s", "not a time");
    ASSERT_EQ(employeeInsert(&e), EMS_OK);
    EXPECT_STREQ(findEmployee(150)->createdAt, "2025-01-01 09:00:00");
    EXPECT_EQ(employeeInsert(&bad), ERR_INVALID_INPUT);
    EXPECT_EQ(findEmployee(151), nullptr);
}

TEST_F(EmployeeTest, employeeUpdate_ChangesUpdatedTimeOnly)
{
    Employee e = makeEmployee(150);
    (void)snprintf(e.createdAt, sizeof(e.createdAt), "%s", "2025-01-01 09:00:00");
    ASSERT_EQ(employeeInsert(&e), EMS_OK);
    e.basicSalary = 50000.0;
    ASSERT_EQ(employeeUpdate(150, &e), EMS_OK);
    EXPECT_STREQ(findEmployee(150)->createdAt, "2025-01-01 09:00:00");
    EXPECT_NE(strcmp(findEmployee(150)->updatedAt, "2025-01-01 09:00:00"), 0);
    EXPECT_EQ(validateTimestamp(findEmployee(150)->updatedAt), 1);
}

TEST_F(EmployeeTest, employeeSetDepartment_ChangesUpdatedTime)
{
    Employee e = makeEmployee(150);
    (void)snprintf(e.createdAt, sizeof(e.createdAt), "%s", "2025-01-01 09:00:00");
    ASSERT_EQ(employeeInsert(&e), EMS_OK);
    ASSERT_EQ(employeeSetDepartment(150, 11), EMS_OK);
    EXPECT_NE(strcmp(findEmployee(150)->updatedAt, "2025-01-01 09:00:00"), 0);
}

TEST_F(EmployeeTest, employeeSetRole_ChangesUpdatedTime)
{
    Employee e = makeEmployee(150);
    (void)snprintf(e.createdAt, sizeof(e.createdAt), "%s", "2025-01-01 09:00:00");
    ASSERT_EQ(employeeInsert(&e), EMS_OK);
    ASSERT_EQ(employeeSetRole(150, 21), EMS_OK);
    EXPECT_NE(strcmp(findEmployee(150)->updatedAt, "2025-01-01 09:00:00"), 0);
}

TEST_F(EmployeeTest, addEmployee_ShowsCreationTime)
{
    feed("103\nAnil Rao\n9123456780\nanil@corp.com\n10\n20\n28000\n");
    ASSERT_EQ(addEmployee(), EMS_OK);
    EXPECT_CONTAINS(getOutput(), "Employee 103 added at ");
    EXPECT_CONTAINS(getOutput(), findEmployee(103)->createdAt);
}

/* ========================================================================= */
/* Searching Module (searching.c)                                            */
/* ========================================================================= */

TEST_F(SearchingTest, employeeMatches_AllSearchTypes)
{
    const Employee *e = findEmployee(101);
    EXPECT_EQ(employeeMatches(e, SEARCH_BY_ID, 101, nullptr), 1);
    EXPECT_EQ(employeeMatches(e, SEARCH_BY_ID, 102, nullptr), 0);
    EXPECT_EQ(employeeMatches(e, SEARCH_BY_NAME, 0, "kumar"), 1);
    EXPECT_EQ(employeeMatches(e, SEARCH_BY_NAME, 0, "priya"), 0);
    EXPECT_EQ(employeeMatches(e, SEARCH_BY_DEPT, 10, nullptr), 1);
    EXPECT_EQ(employeeMatches(e, SEARCH_BY_ROLE, 21, nullptr), 0);
    EXPECT_EQ(employeeMatches(e, 9, 0, nullptr), 0);
    EXPECT_EQ(employeeMatches(nullptr, SEARCH_BY_ID, 101, nullptr), 0);
}

TEST_F(SearchingTest, employeeSearch_ByIdUsesSortedLookup)
{
    EXPECT_EQ(employeeSearch(SEARCH_BY_ID, 102, nullptr, getOutputStream()), 1);
    EXPECT_CONTAINS(getOutput(), "Priya Sharma");
    EXPECT_EQ(employeeSearch(SEARCH_BY_ID, 103, nullptr, nullptr), 0);
}

TEST_F(SearchingTest, employeeSearch_ByNameDeptRole)
{
    EXPECT_EQ(employeeSearch(SEARCH_BY_NAME, 0, "a", nullptr), 2);
    EXPECT_EQ(employeeSearch(SEARCH_BY_DEPT, 11, nullptr, getOutputStream()), 1);
    EXPECT_EQ(employeeSearch(SEARCH_BY_ROLE, 20, nullptr, nullptr), 1);
    EXPECT_EQ(employeeSearch(SEARCH_BY_ROLE, 99, nullptr, nullptr), 0);
}

TEST_F(SearchingTest, searchEmployee_EachType)
{
    feed("1\n101\n2\npriya\n3\n10\n4\n21\n");
    EXPECT_EQ(searchEmployee(), EMS_OK);
    EXPECT_EQ(searchEmployee(), EMS_OK);
    EXPECT_EQ(searchEmployee(), EMS_OK);
    EXPECT_EQ(searchEmployee(), EMS_OK);
    EXPECT_CONTAINS(getOutput(), "1 match(es).");
}

TEST_F(SearchingTest, searchEmployee_NoMatchGivesE003)
{
    feed("2\nzzz\n");
    EXPECT_EQ(searchEmployee(), ERR_NOT_FOUND);
    EXPECT_CONTAINS(getOutput(), "E003");
}

/* ========================================================================= */
/* Attendance Module (attendance.c)                                          */
/* ========================================================================= */

TEST_F(AttendanceTest, attendanceReset_FreesArray)
{
    mark(101, 1, 3, 2025, 'P');
    attendanceReset();
    EXPECT_EQ(getAttendanceCount(), 0);
    EXPECT_EQ(getAttendanceAt(0), nullptr);
}

TEST_F(AttendanceTest, attendanceGrow_GrowsPastInitialCapacity)
{
    for (int d = 1; d <= 31; d++) {
        mark(101, d, 1, 2025, 'P');
    }
    EXPECT_EQ(getAttendanceCount(), 31);
    EXPECT_EQ(attendanceGrow(), EMS_OK);
    EXPECT_EQ(getAttendanceAt(30)->date.day, 31);
}

TEST_F(AttendanceTest, attendanceAdd_ValidRecord)
{
    Date d = makeDate(5, 3, 2025);
    EXPECT_EQ(attendanceAdd(101, &d, 'A', nullptr), EMS_OK);
    EXPECT_EQ(getAttendanceCount(), 1);
    EXPECT_EQ(getAttendanceAt(0)->status, 'A');
}

TEST_F(AttendanceTest, attendanceAdd_UnknownEmployee)
{
    Date d = makeDate(5, 3, 2025);
    EXPECT_EQ(attendanceAdd(999, &d, 'P', nullptr), ERR_NOT_FOUND);
}

TEST_F(AttendanceTest, attendanceAdd_BadDateStatusOrNull)
{
    Date bad = makeDate(30, 2, 2025);
    Date ok = makeDate(1, 3, 2025);
    EXPECT_EQ(attendanceAdd(101, &bad, 'P', nullptr), ERR_INVALID_ATTENDANCE);
    EXPECT_EQ(attendanceAdd(101, &ok, 'X', nullptr), ERR_INVALID_ATTENDANCE);
    EXPECT_EQ(attendanceAdd(101, nullptr, 'P', nullptr), ERR_INVALID_ATTENDANCE);
}

TEST_F(AttendanceTest, attendanceAdd_FutureDateRejected)
{
    Date future = makeDate(1, 1, MAX_YEAR);
    EXPECT_EQ(attendanceAdd(101, &future, 'P', nullptr), ERR_INVALID_ATTENDANCE);
}

TEST_F(AttendanceTest, attendanceAdd_DuplicateDateRejected)
{
    mark(101, 5, 3, 2025, 'P');
    Date d = makeDate(5, 3, 2025);
    EXPECT_EQ(attendanceAdd(101, &d, 'A', nullptr), ERR_INVALID_ATTENDANCE);
    EXPECT_EQ(attendanceAdd(102, &d, 'A', nullptr), EMS_OK);
}

TEST_F(AttendanceTest, attendanceAdd_LockedAfterPayroll)
{
    Payroll p;
    ASSERT_EQ(payrollCompute(101, 3, 2025, 0.0, 0.0, &p), EMS_OK);
    ASSERT_EQ(payrollAdd(&p), EMS_OK);
    Date d = makeDate(6, 3, 2025);
    EXPECT_EQ(attendanceAdd(101, &d, 'P', nullptr), ERR_INVALID_ATTENDANCE);
}

TEST_F(AttendanceTest, findAttendance_FoundAndMissing)
{
    mark(101, 5, 3, 2025, 'P');
    mark(102, 5, 3, 2025, 'L');
    EXPECT_EQ(findAttendance(102, 5, 3, 2025), 1);
    EXPECT_EQ(findAttendance(101, 6, 3, 2025), -1);
}

TEST_F(AttendanceTest, isMonthLocked_OnlyWhenPayrollExists)
{
    EXPECT_EQ(isMonthLocked(101, 3, 2025), 0);
    Payroll p;
    ASSERT_EQ(payrollCompute(101, 3, 2025, 0.0, 0.0, &p), EMS_OK);
    ASSERT_EQ(payrollAdd(&p), EMS_OK);
    EXPECT_EQ(isMonthLocked(101, 3, 2025), 1);
    EXPECT_EQ(isMonthLocked(101, 4, 2025), 0);
}

TEST_F(AttendanceTest, getMonthlySummary_CountsOneMonthOnly)
{
    mark(101, 1, 3, 2025, 'P');
    mark(101, 2, 3, 2025, 'P');
    mark(101, 3, 3, 2025, 'A');
    mark(101, 4, 3, 2025, 'L');
    mark(101, 1, 4, 2025, 'A');
    mark(102, 1, 3, 2025, 'A');
    int p = -1, a = -1, l = -1;
    getMonthlySummary(101, 3, 2025, &p, &a, &l);
    EXPECT_EQ(p, 2);
    EXPECT_EQ(a, 1);
    EXPECT_EQ(l, 1);
}

TEST_F(AttendanceTest, getMonthlySummary_MonthZeroCountsAll)
{
    mark(101, 1, 3, 2025, 'A');
    mark(101, 1, 4, 2025, 'A');
    int p = 0, a = 0, l = 0;
    getMonthlySummary(101, 0, 0, &p, &a, &l);
    EXPECT_EQ(a, 2);
}

TEST_F(AttendanceTest, getMonthlySummary_NullPointersAreSafe)
{
    mark(101, 1, 3, 2025, 'A');
    getMonthlySummary(101, 3, 2025, nullptr, nullptr, nullptr);
    int a = 0;
    getMonthlySummary(999, 3, 2025, nullptr, &a, nullptr);
    EXPECT_EQ(a, 0);
}

TEST_F(AttendanceTest, attendanceRemoveEmployee_RemovesOnlyThatEmployee)
{
    mark(101, 1, 3, 2025, 'P');
    mark(102, 1, 3, 2025, 'P');
    mark(101, 2, 3, 2025, 'P');
    EXPECT_EQ(attendanceRemoveEmployee(101), 2);
    EXPECT_EQ(getAttendanceCount(), 1);
    EXPECT_EQ(getAttendanceAt(0)->empId, 102);
    EXPECT_EQ(attendanceRemoveEmployee(555), 0);
}

TEST_F(AttendanceTest, getAttendanceCount_Counts)
{
    EXPECT_EQ(getAttendanceCount(), 0);
    mark(101, 1, 3, 2025, 'P');
    EXPECT_EQ(getAttendanceCount(), 1);
}

TEST_F(AttendanceTest, getAttendanceAt_Bounds)
{
    mark(101, 1, 3, 2025, 'P');
    EXPECT_NE(getAttendanceAt(0), nullptr);
    EXPECT_EQ(getAttendanceAt(1), nullptr);
    EXPECT_EQ(getAttendanceAt(-1), nullptr);
}

TEST_F(AttendanceTest, statusText_AllValues)
{
    EXPECT_STREQ(statusText('P'), "Present");
    EXPECT_STREQ(statusText('A'), "Absent");
    EXPECT_STREQ(statusText('L'), "Leave");
    EXPECT_STREQ(statusText('Q'), "Unknown");
}

TEST_F(AttendanceTest, recordAttendance_ValidEntrySavedAndLogged)
{
    feed("101\n5\n3\n2025\na\n");
    EXPECT_EQ(recordAttendance(), EMS_OK);
    EXPECT_CONTAINS(readFile("attendance.txt"), "101|5|3|2025|A|");
    EXPECT_EQ(validateTimestamp(getAttendanceAt(0)->recordedAt), 1);
    EXPECT_CONTAINS(getOutput(), "(entered at ");
    EXPECT_CONTAINS(logText(), "Attendance A for employee 101 on 05-03-2025 by admin");
}

TEST_F(AttendanceTest, recordAttendance_UnknownEmployee)
{
    feed("999\n");
    EXPECT_EQ(recordAttendance(), ERR_NOT_FOUND);
}

TEST_F(AttendanceTest, recordAttendance_DuplicateShowsE004)
{
    mark(101, 5, 3, 2025, 'P');
    feed("101\n5\n3\n2025\nP\n");
    EXPECT_EQ(recordAttendance(), ERR_INVALID_ATTENDANCE);
    EXPECT_CONTAINS(getOutput(), "E004");
}

TEST_F(AttendanceTest, recordAttendance_FutureDateMessage)
{
    feed("101\n1\n1\n2100\nP\n");
    EXPECT_EQ(recordAttendance(), ERR_INVALID_ATTENDANCE);
    EXPECT_CONTAINS(getOutput(), "future date");
}

TEST_F(AttendanceTest, recordAttendance_LockedMonthMessage)
{
    Payroll p;
    ASSERT_EQ(payrollCompute(101, 3, 2025, 0.0, 0.0, &p), EMS_OK);
    ASSERT_EQ(payrollAdd(&p), EMS_OK);
    feed("101\n9\n3\n2025\nP\n");
    EXPECT_EQ(recordAttendance(), ERR_INVALID_ATTENDANCE);
    EXPECT_CONTAINS(getOutput(), "Attendance is locked.");
}

TEST_F(AttendanceTest, recordAttendance_EndOfInput)
{
    feed("101\n5\n");
    EXPECT_EQ(recordAttendance(), ERR_END_OF_INPUT);
}

TEST_F(AttendanceTest, attendanceSummary_PrintsCounts)
{
    mark(101, 1, 3, 2025, 'P');
    mark(101, 2, 3, 2025, 'A');
    feed("101\n3\n2025\n");
    EXPECT_EQ(attendanceSummary(), EMS_OK);
    const char *out = getOutput();
    EXPECT_CONTAINS(out, "Present Days      : 1");
    EXPECT_CONTAINS(out, "Absent Days       : 1");
    EXPECT_CONTAINS(out, "Total Days Marked : 2");
}

TEST_F(AttendanceTest, attendanceSummary_UnknownEmployee)
{
    feed("999\n");
    EXPECT_EQ(attendanceSummary(), ERR_NOT_FOUND);
}

TEST_F(AttendanceTest, attendanceMenu_RecordThenSummary)
{
    feed("1\n101\n4\n3\n2025\nP\n2\n101\n3\n2025\n0\n");
    attendanceMenu();
    EXPECT_EQ(getAttendanceCount(), 1);
    EXPECT_CONTAINS(getOutput(), "Present Days      : 1");
}

TEST_F(AttendanceTest, attendanceMenu_EndOfInput)
{
    feed("1\n101\n");
    attendanceMenu();
    EXPECT_EQ(getAttendanceCount(), 0);
}

TEST_F(AttendanceTest, attendanceAdd_StampsRecordedTime)
{
    Date d = makeDate(5, 3, 2025);
    ASSERT_EQ(attendanceAdd(101, &d, 'P', nullptr), EMS_OK);
    EXPECT_EQ(validateTimestamp(getAttendanceAt(0)->recordedAt), 1);
    d.day = 6;
    ASSERT_EQ(attendanceAdd(101, &d, 'P', "2025-01-01 09:00:00"), EMS_OK);
    EXPECT_STREQ(getAttendanceAt(1)->recordedAt, "2025-01-01 09:00:00");
}

TEST_F(AttendanceTest, attendanceAdd_RejectsBadRecordedTime)
{
    Date d = makeDate(5, 3, 2025);
    EXPECT_EQ(attendanceAdd(101, &d, 'P', "05/03/2025"), ERR_INVALID_ATTENDANCE);
    EXPECT_EQ(getAttendanceCount(), 0);
}

/* ========================================================================= */
/* Payroll Module (payroll.c)                                                */
/* ========================================================================= */

TEST_F(PayrollTest, payrollReset_FreesArray)
{
    addPayroll(101, 3, 2025);
    payrollReset();
    EXPECT_EQ(getPayrollCount(), 0);
}

TEST_F(PayrollTest, payrollGrow_GrowsPastInitialCapacity)
{
    for (int m = 1; m <= 12; m++) {
        addPayroll(101, m, 2023);
        addPayroll(102, m, 2023);
    }
    EXPECT_EQ(getPayrollCount(), 24);
    EXPECT_EQ(payrollGrow(), EMS_OK);
}

TEST_F(PayrollTest, roundMoney_RoundsHalfAwayFromZero)
{
    EXPECT_DOUBLE_EQ(roundMoney(1166.6666), 1166.67);
    EXPECT_DOUBLE_EQ(roundMoney(2.004), 2.0);
    EXPECT_DOUBLE_EQ(roundMoney(-2.006), -2.01);
    EXPECT_DOUBLE_EQ(roundMoney(0.0), 0.0);
}

TEST_F(PayrollTest, calculateGross_BasicPlusAllowance)
{
    EXPECT_DOUBLE_EQ(calculateGross(35000.0, 5000.0), 40000.0);
    EXPECT_DOUBLE_EQ(calculateGross(100.0, 0.0), 100.0);
}

TEST_F(PayrollTest, calculateLop_PerDayTimesAbsent)
{
    EXPECT_DOUBLE_EQ(calculateLop(35000.0, 2), 2333.33);
    EXPECT_DOUBLE_EQ(calculateLop(30000.0, 0), 0.0);
}

TEST_F(PayrollTest, calculateLop_ClampsNegativeAndAboveThirty)
{
    EXPECT_DOUBLE_EQ(calculateLop(30000.0, -3), 0.0);
    EXPECT_DOUBLE_EQ(calculateLop(30000.0, 31), 30000.0);
}

TEST_F(PayrollTest, calculateNet_NormalAndNeverNegative)
{
    EXPECT_DOUBLE_EQ(calculateNet(40000.0, 2000.0, 2333.33), 35666.67);
    EXPECT_DOUBLE_EQ(calculateNet(100.0, 500.0, 0.0), 0.0);
}

TEST_F(PayrollTest, findPayroll_FoundAndMissing)
{
    addPayroll(101, 3, 2025);
    EXPECT_EQ(findPayroll(101, 3, 2025), 0);
    EXPECT_EQ(findPayroll(101, 4, 2025), -1);
    EXPECT_EQ(findPayroll(102, 3, 2025), -1);
}

TEST_F(PayrollTest, payrollCompute_UsesAttendanceForLop)
{
    absent(101, 3);
    absent(101, 4);
    Payroll p;
    ASSERT_EQ(payrollCompute(101, 3, 2025, 5000.0, 2000.0, &p), EMS_OK);
    EXPECT_EQ(p.absentDays, 2);
    EXPECT_DOUBLE_EQ(p.salary.basic, 30000.0);
    EXPECT_DOUBLE_EQ(p.salary.gross, 35000.0);
    EXPECT_DOUBLE_EQ(p.salary.lop, 2000.0);
    EXPECT_DOUBLE_EQ(p.salary.net, 31000.0);
}

TEST_F(PayrollTest, payrollCompute_LeaveIsPaid)
{
    Date d = makeDate(3, 3, 2025);
    ASSERT_EQ(attendanceAdd(101, &d, 'L', nullptr), EMS_OK);
    Payroll p;
    ASSERT_EQ(payrollCompute(101, 3, 2025, 0.0, 0.0, &p), EMS_OK);
    EXPECT_DOUBLE_EQ(p.salary.lop, 0.0);
    EXPECT_DOUBLE_EQ(p.salary.net, 30000.0);
}

TEST_F(PayrollTest, payrollCompute_ErrorCases)
{
    Payroll p;
    EXPECT_EQ(payrollCompute(101, 3, 2025, 0.0, 0.0, nullptr), ERR_INVALID_INPUT);
    EXPECT_EQ(payrollCompute(999, 3, 2025, 0.0, 0.0, &p), ERR_NOT_FOUND);
    EXPECT_EQ(payrollCompute(101, 13, 2025, 0.0, 0.0, &p), ERR_INVALID_INPUT);
    EXPECT_EQ(payrollCompute(101, 0, 2025, 0.0, 0.0, &p), ERR_INVALID_INPUT);
    EXPECT_EQ(payrollCompute(101, 3, 1999, 0.0, 0.0, &p), ERR_INVALID_INPUT);
    EXPECT_EQ(payrollCompute(101, 1, MAX_YEAR, 0.0, 0.0, &p), ERR_INVALID_INPUT);
    EXPECT_EQ(payrollCompute(101, 3, 2025, -1.0, 0.0, &p), ERR_INVALID_INPUT);
    EXPECT_EQ(payrollCompute(101, 3, 2025, 0.0, -1.0, &p), ERR_INVALID_INPUT);
}

TEST_F(PayrollTest, payrollCompute_ExistingMonthGivesE005)
{
    addPayroll(101, 3, 2025);
    Payroll p;
    EXPECT_EQ(payrollCompute(101, 3, 2025, 0.0, 0.0, &p), ERR_PAYROLL_EXISTS);
}

TEST_F(PayrollTest, payrollCompute_DeductionsAboveGross)
{
    Payroll p;
    EXPECT_EQ(payrollCompute(101, 3, 2025, 0.0, 30000.01, &p), ERR_INVALID_INPUT);
    EXPECT_EQ(payrollCompute(101, 3, 2025, 0.0, 30000.0, &p), EMS_OK);
    EXPECT_DOUBLE_EQ(p.salary.net, 0.0);
}

TEST_F(PayrollTest, payrollAdd_ValidDuplicateAndInvalid)
{
    Payroll p;
    ASSERT_EQ(payrollCompute(101, 3, 2025, 0.0, 0.0, &p), EMS_OK);
    EXPECT_EQ(payrollAdd(&p), EMS_OK);
    EXPECT_EQ(payrollAdd(&p), ERR_PAYROLL_EXISTS);
    EXPECT_EQ(payrollAdd(nullptr), ERR_INVALID_INPUT);
    Payroll bad = p;
    bad.month = 4;
    bad.empId = 999;
    EXPECT_EQ(payrollAdd(&bad), ERR_INVALID_INPUT);
    bad = p;
    bad.month = 13;
    EXPECT_EQ(payrollAdd(&bad), ERR_INVALID_INPUT);
    bad = p;
    bad.month = 5;
    bad.salary.net = -1.0;
    EXPECT_EQ(payrollAdd(&bad), ERR_INVALID_INPUT);
}

TEST_F(PayrollTest, payrollRemoveEmployee_RemovesOnlyThatEmployee)
{
    addPayroll(101, 3, 2025);
    addPayroll(102, 3, 2025);
    addPayroll(101, 4, 2025);
    EXPECT_EQ(payrollRemoveEmployee(101), 2);
    EXPECT_EQ(getPayrollCount(), 1);
    EXPECT_EQ(getPayrollAt(0)->empId, 102);
}

TEST_F(PayrollTest, getPayrollCount_Counts)
{
    EXPECT_EQ(getPayrollCount(), 0);
    addPayroll(101, 3, 2025);
    EXPECT_EQ(getPayrollCount(), 1);
}

TEST_F(PayrollTest, getPayrollAt_Bounds)
{
    addPayroll(101, 3, 2025);
    EXPECT_NE(getPayrollAt(0), nullptr);
    EXPECT_EQ(getPayrollAt(1), nullptr);
    EXPECT_EQ(getPayrollAt(-1), nullptr);
}

TEST_F(PayrollTest, printPayslip_ShowsAllAmounts)
{
    absent(101, 3);
    Payroll p;
    ASSERT_EQ(payrollCompute(101, 3, 2025, 5000.0, 2000.0, &p), EMS_OK);
    printPayslip(getOutputStream(), &p);
    printPayslip(nullptr, &p);
    printPayslip(getOutputStream(), nullptr);
    const char *out = getOutput();
    EXPECT_CONTAINS(out, "PAYSLIP - 03 / 2025");
    EXPECT_CONTAINS(out, "Employee          : 101 Ravi Kumar");
    EXPECT_CONTAINS(out, "Absent Days (LOP) : 1 (1000.00)");
    EXPECT_CONTAINS(out, "Net Salary        : 32000.00");
}

TEST_F(PayrollTest, processPayroll_SavedAfterConfirm)
{
    absent(101, 3);
    feed("101\n3\n2025\n5000\n2000\ny\n");
    EXPECT_EQ(processPayroll(), EMS_OK);
    EXPECT_EQ(getPayrollCount(), 1);
    EXPECT_CONTAINS(readFile("payroll.txt"), "101|3|2025|30000.00|5000.00|2000.00|1|1000.00|35000.00|32000.00|");
    EXPECT_EQ(validateTimestamp(getPayrollAt(0)->processedAt), 1);
    EXPECT_CONTAINS(getOutput(), "Payroll saved. Processed on ");
    EXPECT_CONTAINS(logText(), "Payroll 03/2025 processed for employee 101 by admin, net 32000.00");
}

TEST_F(PayrollTest, processPayroll_NotSavedWhenAnsweredNo)
{
    feed("101\n3\n2025\n0\n0\nn\n");
    EXPECT_EQ(processPayroll(), EMS_OK);
    EXPECT_EQ(getPayrollCount(), 0);
    EXPECT_CONTAINS(getOutput(), "Payroll not saved.");
}

TEST_F(PayrollTest, processPayroll_UnknownEmployee)
{
    feed("999\n");
    EXPECT_EQ(processPayroll(), ERR_NOT_FOUND);
}

TEST_F(PayrollTest, processPayroll_FutureMonth)
{
    feed("101\n1\n2100\n");
    EXPECT_EQ(processPayroll(), ERR_INVALID_INPUT);
    EXPECT_CONTAINS(getOutput(), "future month");
}

TEST_F(PayrollTest, processPayroll_AlreadyProcessed)
{
    addPayroll(101, 3, 2025);
    feed("101\n3\n2025\n");
    EXPECT_EQ(processPayroll(), ERR_PAYROLL_EXISTS);
    EXPECT_CONTAINS(getOutput(), "E005");
}

TEST_F(PayrollTest, processPayroll_DeductionsTooHigh)
{
    feed("101\n3\n2025\n0\n40000\n");
    EXPECT_EQ(processPayroll(), ERR_INVALID_INPUT);
    EXPECT_CONTAINS(getOutput(), "more than gross salary");
}

TEST_F(PayrollTest, viewPayslip_FoundAndMissing)
{
    addPayroll(101, 3, 2025);
    feed("101\n3\n2025\n101\n4\n2025\n");
    EXPECT_EQ(viewPayslip(), EMS_OK);
    EXPECT_EQ(viewPayslip(), ERR_NOT_FOUND);
    EXPECT_CONTAINS(getOutput(), "PAYSLIP - 03 / 2025");
}

TEST_F(PayrollTest, payrollMenu_ProcessAndView)
{
    feed("1\n101\n3\n2025\n0\n0\ny\n2\n101\n3\n2025\n0\n");
    payrollMenu();
    EXPECT_EQ(getPayrollCount(), 1);
}

TEST_F(PayrollTest, payrollMenu_EndOfInput)
{
    feed("1\n101\n");
    payrollMenu();
    EXPECT_EQ(getPayrollCount(), 0);
}

TEST_F(PayrollTest, payrollAdd_StampsProcessedTime)
{
    Payroll p;
    ASSERT_EQ(payrollCompute(101, 3, 2025, 0.0, 0.0, &p), EMS_OK);
    EXPECT_STREQ(p.processedAt, "");
    ASSERT_EQ(payrollAdd(&p), EMS_OK);
    EXPECT_EQ(validateTimestamp(getPayrollAt(0)->processedAt), 1);
}

TEST_F(PayrollTest, payrollAdd_RejectsBadProcessedTime)
{
    Payroll p;
    ASSERT_EQ(payrollCompute(101, 3, 2025, 0.0, 0.0, &p), EMS_OK);
    (void)snprintf(p.processedAt, sizeof(p.processedAt), "%s", "soon");
    EXPECT_EQ(payrollAdd(&p), ERR_INVALID_INPUT);
}

TEST_F(PayrollTest, printPayslip_ShowsProcessedTime)
{
    Payroll p;
    ASSERT_EQ(payrollCompute(101, 3, 2025, 0.0, 0.0, &p), EMS_OK);
    printPayslip(getOutputStream(), &p);
    EXPECT_CONTAINS(getOutput(), "Processed On      : (not saved yet)");
    (void)snprintf(p.processedAt, sizeof(p.processedAt), "%s", "2025-01-01 09:00:00");
    printPayslip(getOutputStream(), &p);
    EXPECT_CONTAINS(getOutput(), "Processed On      : 2025-01-01 09:00:00");
}

/* ========================================================================= */
/* Report Module (report.c)                                                  */
/* ========================================================================= */

TEST_F(ReportTest, printReportTitle_HasTitleTimeAndUser)
{
    printReportTitle(getOutputStream(), "TEST REPORT");
    printReportTitle(nullptr, "X");
    printReportTitle(getOutputStream(), nullptr);
    const char *out = getOutput();
    EXPECT_CONTAINS(out, "TEST REPORT");
    EXPECT_CONTAINS(out, "Generated : ");
    EXPECT_CONTAINS(out, "by admin");
}

TEST_F(ReportTest, generateEmployeeReport_AllEmployees)
{
    EXPECT_EQ(generateEmployeeReport(getOutputStream()), 2);
    const char *out = getOutput();
    EXPECT_CONTAINS(out, "EMPLOYEE REPORT");
    EXPECT_CONTAINS(out, "Priya Sharma");
    EXPECT_CONTAINS(out, "Total Employees : 2");
}

TEST_F(ReportTest, generateEmployeeReport_NullStreamAndEmpty)
{
    EXPECT_EQ(generateEmployeeReport(nullptr), 0);
    employeeReset();
    EXPECT_EQ(generateEmployeeReport(getOutputStream()), 0);
}

TEST_F(ReportTest, generateAttendanceReport_CountsPerEmployee)
{
    EXPECT_EQ(generateAttendanceReport(getOutputStream(), 3, 2025), 2);
    const char *out = getOutput();
    EXPECT_CONTAINS(out, "Month / Year : 03 / 2025");
    EXPECT_CONTAINS(out, "|       1 |      1 |     0");
    EXPECT_EQ(generateAttendanceReport(nullptr, 3, 2025), 0);
}

TEST_F(ReportTest, generatePayrollReport_FiltersByMonthAndTotals)
{
    EXPECT_EQ(generatePayrollReport(getOutputStream(), 3, 2025), 1);
    EXPECT_CONTAINS(getOutput(), "Total Net Payout : 32000.00");
    EXPECT_EQ(generatePayrollReport(getOutputStream(), 4, 2025), 0);
    EXPECT_EQ(generatePayrollReport(nullptr, 3, 2025), 0);
}

TEST_F(ReportTest, generateReport_EmployeeType)
{
    EXPECT_EQ(generateReport(REPORT_EMPLOYEE), EMS_OK);
    EXPECT_CONTAINS(logText(), "Report type 1 generated with 2 row(s) by admin");
}

TEST_F(ReportTest, generateReport_AttendanceAsksForMonth)
{
    feed("3\n2025\n");
    EXPECT_EQ(generateReport(REPORT_ATTENDANCE), EMS_OK);
}

TEST_F(ReportTest, generateReport_PayrollAdminOnly)
{
    feed("3\n2025\n");
    EXPECT_EQ(generateReport(REPORT_PAYROLL), EMS_OK);
    setSession("hr_user", ROLE_OPERATOR);
    EXPECT_EQ(generateReport(REPORT_PAYROLL), ERR_ACCESS_DENIED);
    EXPECT_CONTAINS(getOutput(), "E010");
}

TEST_F(ReportTest, generateReport_NoRowsGivesE003)
{
    feed("4\n2025\n");
    EXPECT_EQ(generateReport(REPORT_PAYROLL), ERR_NOT_FOUND);
}

TEST_F(ReportTest, generateReport_InvalidTypeAndEof)
{
    EXPECT_EQ(generateReport(9), ERR_INVALID_INPUT);
    feed("3\n");
    EXPECT_EQ(generateReport(REPORT_ATTENDANCE), ERR_END_OF_INPUT);
}

TEST_F(ReportTest, reportMenu_AllReportsThenBack)
{
    feed("1\n2\n3\n2025\n3\n3\n2025\n0\n");
    reportMenu();
    const char *out = getOutput();
    EXPECT_CONTAINS(out, "EMPLOYEE REPORT");
    EXPECT_CONTAINS(out, "ATTENDANCE SUMMARY REPORT");
    EXPECT_CONTAINS(out, "PAYROLL REPORT");
}

TEST_F(ReportTest, reportMenu_EndOfInput)
{
    feed("2\n3\n");
    reportMenu();
    EXPECT_NOT_CONTAINS(getOutput(), "ATTENDANCE SUMMARY REPORT");
}

TEST_F(ReportTest, generatePayrollReport_ShowsProcessedTime)
{
    EXPECT_EQ(generatePayrollReport(getOutputStream(), 3, 2025), 1);
    EXPECT_CONTAINS(getOutput(), "Processed On");
    EXPECT_CONTAINS(getOutput(), getPayrollAt(0)->processedAt);
}

TEST_F(ReportTest, generateEmployeeReport_ShowsCreatedTime)
{
    EXPECT_EQ(generateEmployeeReport(getOutputStream()), 2);
    EXPECT_CONTAINS(getOutput(), findEmployee(101)->createdAt);
}

/* ========================================================================= */
/* File Manager Module (filemanager.c)                                       */
/* ========================================================================= */

TEST_F(FileManagerTest, setDataDirectory_ValidAndInvalid)
{
    EXPECT_EQ(setDataDirectory("somewhere"), EMS_OK);
    EXPECT_STREQ(getDataDirectory(), "somewhere");
    EXPECT_EQ(setDataDirectory(nullptr), ERR_INVALID_INPUT);
    EXPECT_EQ(setDataDirectory(""), ERR_INVALID_INPUT);
    EXPECT_EQ(setDataDirectory(repeatChar(PATH_SIZE, 'x')), ERR_INVALID_INPUT);
    EXPECT_STREQ(getDataDirectory(), "somewhere");
}

TEST_F(FileManagerTest, getDataDirectory_ReturnsCurrent)
{
    EXPECT_STREQ(getDataDirectory(), testDir());
}

TEST_F(FileManagerTest, buildDataPath_JoinsFolderAndName)
{
    char buf[PATH_SIZE];
    EXPECT_EQ(buildDataPath("x.txt", buf, sizeof(buf)), EMS_OK);
    EXPECT_STREQ(buf, testPath("x.txt"));
}

TEST_F(FileManagerTest, buildDataPath_RejectsNullAndSmallBuffer)
{
    char small[8];
    EXPECT_EQ(buildDataPath(nullptr, small, sizeof(small)), ERR_INVALID_INPUT);
    EXPECT_EQ(buildDataPath("x", nullptr, 10), ERR_INVALID_INPUT);
    EXPECT_EQ(buildDataPath("x", small, 0), ERR_INVALID_INPUT);
    EXPECT_EQ(buildDataPath("x.txt", small, sizeof(small)), ERR_INVALID_INPUT);
}

TEST_F(FileManagerTest, beginSave_CreatesTempFile)
{
    char tmp[PATH_SIZE];
    FILE *fp = beginSave("a.txt", tmp, sizeof(tmp));
    ASSERT_NE(fp, nullptr);
    EXPECT_STREQ(tmp, testPath("a.txt.tmp"));
    fclose(fp);
    EXPECT_TRUE(fileExists("a.txt.tmp"));
}

TEST_F(FileManagerTest, beginSave_FailsForMissingFolderOrNull)
{
    char tmp[PATH_SIZE];
    char small[4];
    EXPECT_EQ(beginSave("a.txt", nullptr, 10), nullptr);
    EXPECT_EQ(beginSave("a.txt", small, sizeof(small)), nullptr);
    ASSERT_EQ(setDataDirectory("/no/such/folder"), EMS_OK);
    EXPECT_EQ(beginSave("a.txt", tmp, sizeof(tmp)), nullptr);
}

TEST_F(FileManagerTest, commitSave_RenamesTempOverReal)
{
    char tmp[PATH_SIZE];
    writeFile("a.txt", "old\n");
    FILE *fp = beginSave("a.txt", tmp, sizeof(tmp));
    ASSERT_NE(fp, nullptr);
    fputs("new\n", fp);
    EXPECT_EQ(commitSave(fp, "a.txt", tmp), EMS_OK);
    EXPECT_STREQ(readFile("a.txt"), "new\n");
    EXPECT_FALSE(fileExists("a.txt.tmp"));
}

TEST_F(FileManagerTest, commitSave_NullFileReportsE007)
{
    EXPECT_EQ(commitSave(nullptr, "a.txt", "x"), ERR_FILE);
    EXPECT_CONTAINS(getOutput(), "E007");
    EXPECT_CONTAINS(logText(), "could not save a.txt");
}

TEST_F(FileManagerTest, commitSave_NullFileNameRemovesTemp)
{
    char tmp[PATH_SIZE];
    FILE *fp = beginSave("b.txt", tmp, sizeof(tmp));
    ASSERT_NE(fp, nullptr);
    EXPECT_EQ(commitSave(fp, nullptr, tmp), ERR_FILE);
    EXPECT_FALSE(fileExists("b.txt.tmp"));
}

TEST_F(FileManagerTest, backupFile_CopiesFile)
{
    writeFile("data.txt", "line1\nline2\n");
    EXPECT_EQ(backupFile("data.txt"), EMS_OK);
    EXPECT_STREQ(readFile("data.txt.bak"), "line1\nline2\n");
}

TEST_F(FileManagerTest, backupFile_MissingFile)
{
    EXPECT_EQ(backupFile("none.txt"), ERR_FILE);
    EXPECT_EQ(backupFile(nullptr), ERR_INVALID_INPUT);
}

TEST_F(FileManagerTest, reportBadLines_WarnsAndBacksUp)
{
    writeFile("x.txt", "bad\n");
    reportBadLines("x.txt", 0);
    EXPECT_FALSE(fileExists("x.txt.bak"));
    reportBadLines("x.txt", 1);
    EXPECT_TRUE(fileExists("x.txt.bak"));
    EXPECT_CONTAINS(getOutput(), "Warning: 1 damaged line(s) skipped in x.txt");
    EXPECT_CONTAINS(logText(), "[WARN] [FILE]");
}

TEST_F(FileManagerTest, openForLoad_MissingFileIsNotAnError)
{
    int status = -1;
    EXPECT_EQ(openForLoad("missing.txt", &status), nullptr);
    EXPECT_EQ(status, EMS_OK);
    EXPECT_CONTAINS(logText(), "missing.txt not found");
}

TEST_F(FileManagerTest, openForLoad_ExistingFile)
{
    int status = -1;
    writeFile("here.txt", "x\n");
    FILE *fp = openForLoad("here.txt", &status);
    ASSERT_NE(fp, nullptr);
    fclose(fp);
    EXPECT_EQ(status, EMS_OK);
}

TEST_F(FileManagerTest, loadDepartments_ValidAndDamaged)
{
    writeFile("departments.txt", "10|Finance|2025-01-01 09:00:00\nxx\n11|HR|2025-01-01 09:00:00\n10|Duplicate|2025-01-01 09:00:00\n12|Sales|not-a-time\n");
    EXPECT_EQ(loadDepartments(), EMS_OK);
    EXPECT_EQ(getDeptCount(), 2);
    EXPECT_STREQ(getDeptAt(0)->createdAt, "2025-01-01 09:00:00");
    EXPECT_TRUE(fileExists("departments.txt.bak"));
}

TEST_F(FileManagerTest, saveDepartments_RoundTrip)
{
    ASSERT_EQ(departmentAdd(10, "Finance", nullptr), EMS_OK);
    EXPECT_EQ(saveDepartments(), EMS_OK);
    deptReset();
    EXPECT_EQ(loadDepartments(), EMS_OK);
    EXPECT_STREQ(getDeptName(10), "Finance");
}

TEST_F(FileManagerTest, loadRoles_ValidAndDamaged)
{
    writeFile("roles.txt", "20|Accountant|2025-01-01 09:00:00\n|\n");
    EXPECT_EQ(loadRoles(), EMS_OK);
    EXPECT_EQ(getRoleCount(), 1);
}

TEST_F(FileManagerTest, saveRoles_RoundTrip)
{
    ASSERT_EQ(roleAdd(20, "Accountant", nullptr), EMS_OK);
    EXPECT_EQ(saveRoles(), EMS_OK);
    EXPECT_CONTAINS(readFile("roles.txt"), "20|Accountant|");
}

TEST_F(FileManagerTest, loadEmployees_SkipsUnknownDepartmentAndBadLines)
{
    ASSERT_EQ(departmentAdd(10, "Finance", nullptr), EMS_OK);
    ASSERT_EQ(roleAdd(20, "Accountant", nullptr), EMS_OK);
    writeFile("employees.txt",
              "101|Ravi Kumar|9876543210|ravi@corp.com|10|20|30000.00|2025-01-01 09:00:00|2025-01-01 09:00:00\n"
              "102|Priya|9876543210|p@corp.com|99|20|20000.00|2025-01-01 09:00:00|2025-01-01 09:00:00\n"
              "103|Old Format|9876543210|o@corp.com|10|20|20000.00\n"
              "junk\n");
    EXPECT_EQ(loadEmployees(), EMS_OK);
    EXPECT_EQ(getEmployeeCount(), 1);
    EXPECT_NE(findEmployee(101), nullptr);
    EXPECT_STREQ(findEmployee(101)->createdAt, "2025-01-01 09:00:00");
}

TEST_F(FileManagerTest, saveEmployees_RoundTrip)
{
    ASSERT_EQ(test_seed_data(), EMS_OK);
    EXPECT_EQ(saveEmployees(), EMS_OK);
    employeeReset();
    EXPECT_EQ(loadEmployees(), EMS_OK);
    EXPECT_EQ(getEmployeeCount(), 2);
    EXPECT_STREQ(findEmployee(102)->email, "ravi@corp.com");
}

TEST_F(FileManagerTest, loadAttendance_SkipsUnknownEmployee)
{
    ASSERT_EQ(test_seed_data(), EMS_OK);
    writeFile("attendance.txt", "101|1|3|2025|P|2025-01-01 09:00:00\n555|1|3|2025|P|2025-01-01 09:00:00\n101|2|3|2025|X|2025-01-01 09:00:00\n"
                                "101|3|3|2025|P|2025-13-01 09:00:00\nbad\n");
    EXPECT_EQ(loadAttendance(), EMS_OK);
    EXPECT_EQ(getAttendanceCount(), 1);
}

TEST_F(FileManagerTest, saveAttendance_RoundTrip)
{
    ASSERT_EQ(test_seed_data(), EMS_OK);
    Date d = makeDate(1, 3, 2025);
    ASSERT_EQ(attendanceAdd(102, &d, 'L', nullptr), EMS_OK);
    EXPECT_EQ(saveAttendance(), EMS_OK);
    EXPECT_CONTAINS(readFile("attendance.txt"), "102|1|3|2025|L|");
}

TEST_F(FileManagerTest, loadPayroll_SkipsDuplicateAndBadLines)
{
    ASSERT_EQ(test_seed_data(), EMS_OK);
    writeFile("payroll.txt",
              "101|3|2025|30000.00|0.00|0.00|0|0.00|30000.00|30000.00|2025-01-01 09:00:00\n"
              "101|3|2025|30000.00|0.00|0.00|0|0.00|30000.00|30000.00|2025-01-01 09:00:00\n"
              "101|4|2025|30000.00|0.00|0.00|0|0.00|30000.00|30000.00|yesterday\n"
              "short line\n");
    EXPECT_EQ(loadPayroll(), EMS_OK);
    EXPECT_EQ(getPayrollCount(), 1);
    EXPECT_STREQ(getPayrollAt(0)->processedAt, "2025-01-01 09:00:00");
}

TEST_F(FileManagerTest, savePayroll_RoundTrip)
{
    ASSERT_EQ(test_seed_data(), EMS_OK);
    Payroll p;
    ASSERT_EQ(payrollCompute(101, 3, 2025, 100.0, 50.0, &p), EMS_OK);
    ASSERT_EQ(payrollAdd(&p), EMS_OK);
    EXPECT_EQ(savePayroll(), EMS_OK);
    payrollReset();
    EXPECT_EQ(loadPayroll(), EMS_OK);
    EXPECT_DOUBLE_EQ(getPayrollAt(0)->salary.net, 30050.0);
}

TEST_F(FileManagerTest, loadAllRecords_FullRoundTrip)
{
    ASSERT_EQ(test_seed_data(), EMS_OK);
    Date d = makeDate(1, 3, 2025);
    ASSERT_EQ(attendanceAdd(101, &d, 'A', nullptr), EMS_OK);
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

TEST_F(FileManagerTest, loadAllRecords_EmptyFolderStartsEmpty)
{
    EXPECT_EQ(loadAllRecords(), EMS_OK);
    EXPECT_EQ(getEmployeeCount(), 0);
}

TEST_F(FileManagerTest, saveAllRecords_FailsWhenFolderMissing)
{
    ASSERT_EQ(setDataDirectory("/no/such/folder"), EMS_OK);
    EXPECT_EQ(saveAllRecords(), ERR_FILE);
}

TEST_F(FileManagerTest, freeAllMemory_EmptiesEverything)
{
    ASSERT_EQ(test_seed_data(), EMS_OK);
    ASSERT_EQ(addUserRecord("admin", "admin123", ROLE_ADMIN), EMS_OK);
    freeAllMemory();
    EXPECT_EQ(getEmployeeCount(), 0);
    EXPECT_EQ(getDeptCount(), 0);
    EXPECT_EQ(getRoleCount(), 0);
    EXPECT_EQ(getUserCount(), 0);
    EXPECT_EQ(getAttendanceCount(), 0);
    EXPECT_EQ(getPayrollCount(), 0);
}

/* ========================================================================= */
/* Application Layer (app.c)                                                 */
/* ========================================================================= */

TEST_F(AppTest, systemInit_LoadsUsersAndOpensLog)
{
    writeAdmin();
    EXPECT_EQ(systemInit(testDir(), testPath("test.log")), EMS_OK);
    EXPECT_EQ(getUserCount(), 2);
    EXPECT_EQ(logIsOpen(), 1);
    EXPECT_CONTAINS(logText(), "System started");
}

TEST_F(AppTest, systemInit_BadLogPathStillRuns)
{
    EXPECT_EQ(systemInit(testDir(), "/no/such/folder/x.log"), EMS_OK);
    EXPECT_CONTAINS(getOutput(), "log file could not be opened");
}

TEST_F(AppTest, systemInit_BadDataFolderName)
{
    EXPECT_EQ(systemInit("", testPath("test.log")), ERR_INVALID_INPUT);
}

TEST_F(AppTest, systemCleanup_FreesAndClosesLog)
{
    ASSERT_EQ(test_seed_data(), EMS_OK);
    systemCleanup();
    EXPECT_EQ(getEmployeeCount(), 0);
    EXPECT_EQ(logIsOpen(), 0);
    EXPECT_CONTAINS(logText(), "System stopped, memory released");
}

TEST_F(AppTest, displayMainMenu_ShowsUserAndOptions)
{
    ASSERT_EQ(test_seed_data(), EMS_OK);
    displayMainMenu();
    const char *out = getOutput();
    EXPECT_CONTAINS(out, "Logged in as : admin (Administrator)");
    EXPECT_CONTAINS(out, "Employees: 2 | Departments: 2 | Roles: 2");
    EXPECT_CONTAINS(out, "7. Save and Exit");
}

TEST_F(AppTest, mainMenu_AdminVisitsEveryMenu)
{
    feed("1\n0\n2\n0\n3\n0\n4\n0\n5\n0\n6\n0\n8\nhr_two\nsecret11\nsecret11\n2\n7\n");
    EXPECT_EQ(mainMenu(), EMS_OK);
    EXPECT_GE(findUser("hr_two"), 0);
}

TEST_F(AppTest, mainMenu_OperatorBlockedFromAdminMenus)
{
    setSession("hr_user", ROLE_OPERATOR);
    feed("2\n3\n5\n8\n7\n");
    EXPECT_EQ(mainMenu(), EMS_OK);
    EXPECT_EQ(countOccurrences(getOutput(), "E010"), 4);
}

TEST_F(AppTest, mainMenu_EndOfInput)
{
    feed("");
    EXPECT_EQ(mainMenu(), ERR_END_OF_INPUT);
}

TEST_F(AppTest, runApplication_FirstRunFullWorkflow)
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

TEST_F(AppTest, runApplication_LoginFailureExits)
{
    writeAdmin();
    feed("admin\nbad1\nadmin\nbad2\nadmin\nbad3\n");
    EXPECT_EQ(runApplication(testDir(), testPath("test.log")), EXIT_FAILURE);
    EXPECT_CONTAINS(logText(), "too many failed attempts");
}

TEST_F(AppTest, runApplication_EndOfInputSavesSafely)
{
    writeAdmin();
    writeFile("departments.txt", "10|Finance|2025-01-01 09:00:00\n");
    feed("admin\nadmin123\n");
    EXPECT_EQ(runApplication(testDir(), testPath("test.log")), EXIT_SUCCESS);
    EXPECT_CONTAINS(getOutput(), "Input closed. Saving and exiting safely.");
    EXPECT_STREQ(readFile("departments.txt"), "10|Finance|2025-01-01 09:00:00\n");
}

TEST_F(AppTest, runApplication_InputClosedDuringFirstRun)
{
    feed("boss\n");
    EXPECT_EQ(runApplication(testDir(), testPath("test.log")), EXIT_FAILURE);
    EXPECT_EQ(getUserCount(), 0);
}

/* ========================================================================= */
/* INTEGRATION TESTS: multi-module workflows                                 */
/* ========================================================================= */

/* Department -> Role -> Employee -> Searching */
TEST_F(IntegrationTest, Integration_HireEmployeeIntoDepartmentWithRole)
{
    ASSERT_EQ(departmentAdd(10, "Engineering", nullptr), EMS_OK);
    ASSERT_EQ(roleAdd(201, "Software Engineer", nullptr), EMS_OK);
    Employee e = makeEmployeeFull(1001, "Alice Smith", 10, 201, 75000.0);
    ASSERT_EQ(employeeInsert(&e), EMS_OK);

    EXPECT_EQ(employeeSearch(SEARCH_BY_DEPT, 10, nullptr, getOutputStream()), 1);
    EXPECT_STREQ(getDeptName(findEmployee(1001)->deptId), "Engineering");
    EXPECT_STREQ(getRoleTitle(findEmployee(1001)->roleId), "Software Engineer");
    EXPECT_EQ(countEmployeesInDept(10), 1);
    EXPECT_EQ(validateTimestamp(findEmployee(1001)->createdAt), 1);
}

/* Employee -> Attendance -> Payroll: absent days become loss of pay */
TEST_F(IntegrationTest, Integration_MonthlyPayrollCalculationFromAttendance)
{
    ASSERT_EQ(departmentAdd(10, "Engineering", nullptr), EMS_OK);
    ASSERT_EQ(roleAdd(201, "Software Engineer", nullptr), EMS_OK);
    Employee e = makeEmployeeFull(1002, "Bob Jones", 10, 201, 60000.0);
    ASSERT_EQ(employeeInsert(&e), EMS_OK);
    for (int day = 3; day <= 24; day++) {
        Date d = makeDate(day, 3, 2025);
        ASSERT_EQ(attendanceAdd(1002, &d, 'P', nullptr), EMS_OK);
    }
    Date a1 = makeDate(25, 3, 2025);
    Date a2 = makeDate(26, 3, 2025);
    Date lv = makeDate(27, 3, 2025);
    ASSERT_EQ(attendanceAdd(1002, &a1, 'A', nullptr), EMS_OK);
    ASSERT_EQ(attendanceAdd(1002, &a2, 'A', nullptr), EMS_OK);
    ASSERT_EQ(attendanceAdd(1002, &lv, 'L', nullptr), EMS_OK);

    Payroll p;
    ASSERT_EQ(payrollCompute(1002, 3, 2025, 4000.0, 1500.0, &p), EMS_OK);
    EXPECT_EQ(p.absentDays, 2);
    EXPECT_DOUBLE_EQ(p.salary.gross, 64000.0);
    EXPECT_DOUBLE_EQ(p.salary.lop, 4000.0);            /* 60000 / 30 x 2 */
    EXPECT_DOUBLE_EQ(p.salary.net, 58500.0);           /* 64000 - 1500 - 4000 */
    ASSERT_EQ(payrollAdd(&p), EMS_OK);
    EXPECT_EQ(validateTimestamp(getPayrollAt(0)->processedAt), 1);
}

/* Payroll -> Attendance: once paid, the month's attendance is frozen */
TEST_F(IntegrationTest, Integration_PayrollLocksAttendanceForThatMonth)
{
    ASSERT_EQ(test_seed_data(), EMS_OK);
    Payroll p;
    ASSERT_EQ(payrollCompute(101, 3, 2025, 0.0, 0.0, &p), EMS_OK);
    ASSERT_EQ(payrollAdd(&p), EMS_OK);
    Date march = makeDate(10, 3, 2025);
    Date april = makeDate(10, 4, 2025);
    EXPECT_EQ(attendanceAdd(101, &march, 'P', nullptr), ERR_INVALID_ATTENDANCE);
    EXPECT_EQ(attendanceAdd(101, &april, 'P', nullptr), EMS_OK);
    EXPECT_EQ(payrollCompute(101, 3, 2025, 0.0, 0.0, &p), ERR_PAYROLL_EXISTS);
}

/* Employee delete cascades into Attendance, Payroll and the saved files */
TEST_F(IntegrationTest, Integration_DeleteEmployeeRemovesHistoryEverywhere)
{
    ASSERT_EQ(test_seed_data(), EMS_OK);
    Date d = makeDate(3, 3, 2025);
    ASSERT_EQ(attendanceAdd(101, &d, 'P', nullptr), EMS_OK);
    ASSERT_EQ(attendanceAdd(102, &d, 'P', nullptr), EMS_OK);
    Payroll p;
    ASSERT_EQ(payrollCompute(101, 3, 2025, 0.0, 0.0, &p), EMS_OK);
    ASSERT_EQ(payrollAdd(&p), EMS_OK);

    feed("101\ny\n");
    ASSERT_EQ(deleteEmployee(), EMS_OK);
    EXPECT_EQ(findEmployee(101), nullptr);
    EXPECT_EQ(getAttendanceCount(), 1);
    EXPECT_EQ(getPayrollCount(), 0);
    EXPECT_NOT_CONTAINS(readFile("employees.txt"), "101|");
    EXPECT_NOT_CONTAINS(readFile("attendance.txt"), "101|");
    EXPECT_CONTAINS(logText(), "Deleted employee 101 by admin");
}

/* File Manager round trip across every module, timestamps preserved */
TEST_F(IntegrationTest, Integration_SaveAndReloadKeepsAllRecordsAndTimestamps)
{
    ASSERT_EQ(test_seed_data(), EMS_OK);
    ASSERT_EQ(addUserRecord("admin", "admin123", ROLE_ADMIN), EMS_OK);
    Date d = makeDate(3, 3, 2025);
    ASSERT_EQ(attendanceAdd(101, &d, 'A', nullptr), EMS_OK);
    Payroll p;
    ASSERT_EQ(payrollCompute(101, 3, 2025, 100.0, 50.0, &p), EMS_OK);
    ASSERT_EQ(payrollAdd(&p), EMS_OK);
    char empCreated[TIMESTAMP_SIZE];
    char paidAt[TIMESTAMP_SIZE];
    std::snprintf(empCreated, sizeof(empCreated), "%s", findEmployee(101)->createdAt);
    std::snprintf(paidAt, sizeof(paidAt), "%s", getPayrollAt(0)->processedAt);
    ASSERT_EQ(saveAllRecords(), EMS_OK);
    ASSERT_EQ(saveUsers(), EMS_OK);

    freeAllMemory();
    ASSERT_EQ(loadUsers(), EMS_OK);
    ASSERT_EQ(loadAllRecords(), EMS_OK);
    EXPECT_EQ(getUserCount(), 1);
    EXPECT_EQ(getEmployeeCount(), 2);
    EXPECT_EQ(getAttendanceCount(), 1);
    EXPECT_EQ(getPayrollCount(), 1);
    EXPECT_STREQ(findEmployee(101)->createdAt, empCreated);
    EXPECT_STREQ(getPayrollAt(0)->processedAt, paidAt);
    EXPECT_EQ(verifyCredentials("admin", "admin123"), ROLE_ADMIN);
}

/* Auth across modules: HR Operator is blocked from every admin menu */
TEST_F(IntegrationTest, Integration_OperatorBlockedFromAdminMenus)
{
    ASSERT_EQ(test_seed_data(), EMS_OK);
    setSession("hr_user", ROLE_OPERATOR);
    feed("2\n3\n5\n8\n6\n3\n0\n1\n5\n0\n7\n");
    EXPECT_EQ(mainMenu(), EMS_OK);
    EXPECT_EQ(countOccurrences(getOutput(), "E010"), 6);
    EXPECT_NE(findEmployee(102), nullptr);
    EXPECT_EQ(countOccurrences(logText(), "[AUTH] E010"), 6);
}

/* Whole program through the menus: first run to exit */
TEST_F(IntegrationTest, Integration_FullSessionAddAssignAttendancePayrollReport)
{
    feed("boss\nadmin123\nadmin123\n"
         "boss\nadmin123\n"
         "2\n1\n10\nFinance\n1\n11\nHR\n0\n"
         "3\n1\n20\nAccountant\n1\n21\nExecutive\n0\n"
         "1\n1\n101\nRavi Kumar\n9876543210\nravi@corp.com\n10\n20\n35000\n"
         "6\n101\n11\n7\n101\n21\n0\n"
         "4\n1\n101\n5\n3\n2025\nA\n0\n"
         "5\n1\n101\n3\n2025\n5000\n2000\ny\n0\n"
         "6\n1\n3\n3\n2025\n0\n"
         "7\n");
    EXPECT_EQ(runApplication(testDir(), testPath("test.log")), EXIT_SUCCESS);
    EXPECT_CONTAINS(readFile("employees.txt"), "101|Ravi Kumar|9876543210|ravi@corp.com|11|21|35000.00|");
    EXPECT_CONTAINS(readFile("payroll.txt"), "101|3|2025|35000.00|5000.00|2000.00|1|1166.67|40000.00|36833.33|");
    EXPECT_CONTAINS(getOutput(), "Total Net Payout : 36833.33");
    EXPECT_CONTAINS(logText(), "Department changed for employee 101 by boss");
    EXPECT_CONTAINS(logText(), "Payroll 03/2025 processed for employee 101 by boss, net 36833.33");
}
