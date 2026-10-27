/** @file test_io.c  Unit tests for the console I/O module. */
#include "test_framework.h"



static void setInputStream_NullRestoresStdin(void)
{
    feed("x\n");
    EXPECT_PTR_EQ(getInputStream(), testInput());
    setInputStream(NULL);
    EXPECT_PTR_EQ(getInputStream(), stdin);
}

static void getInputStream_DefaultsToStdin(void)
{
    setInputStream(NULL);
    EXPECT_PTR_EQ(getInputStream(), stdin);
}

static void setOutputStream_NullRestoresStdout(void)
{
    setOutputStream(NULL);
    EXPECT_PTR_EQ(getOutputStream(), stdout);
    setOutputStream(testOutput());
    EXPECT_PTR_EQ(getOutputStream(), testOutput());
}

static void getOutputStream_ReturnsRedirectedStream(void)
{
    EXPECT_PTR_EQ(getOutputStream(), testOutput());
}

static void trimString_RemovesBothEnds(void)
{
    char s[] = "   hello world \t ";
    trimString(s);
    EXPECT_STREQ(s, "hello world");
}

static void trimString_AllSpacesBecomesEmpty(void)
{
    char s[] = "    ";
    trimString(s);
    EXPECT_STREQ(s, "");
}

static void trimString_NullAndEmptyAreSafe(void)
{
    char s[] = "";
    trimString(NULL);
    trimString(s);
    EXPECT_STREQ(s, "");
}

static void readLine_ReadsAndTrimsLine(void)
{
    char buf[32];
    feed("  Ravi  \n");
    EXPECT_EQ(readLine("Name: ", buf, sizeof(buf)), EMS_OK);
    EXPECT_STREQ(buf, "Ravi");
    EXPECT_CONTAINS(getOutput(), "Name: ");
}

static void readLine_EndOfInput(void)
{
    char buf[32];
    feed("");
    EXPECT_EQ(readLine(NULL, buf, sizeof(buf)), ERR_END_OF_INPUT);
    EXPECT_STREQ(buf, "");
}

static void readLine_TooLongLineIsDiscarded(void)
{
    char buf[8];
    feed("abcdefghijklmnop\nok\n");
    EXPECT_EQ(readLine("", buf, sizeof(buf)), ERR_INVALID_INPUT);
    EXPECT_EQ(readLine("", buf, sizeof(buf)), EMS_OK);
    EXPECT_STREQ(buf, "ok");
}

static void readLine_RejectsBadBuffer(void)
{
    char buf[1];
    EXPECT_EQ(readLine("", NULL, 10), ERR_INVALID_INPUT);
    EXPECT_EQ(readLine("", buf, sizeof(buf)), ERR_INVALID_INPUT);
}

static void readLine_LastLineWithoutNewline(void)
{
    char buf[16];
    feed("last");
    EXPECT_EQ(readLine("", buf, sizeof(buf)), EMS_OK);
    EXPECT_STREQ(buf, "last");
}

static void readInt_ValidNumber(void)
{
    int v = 0;
    feed("42\n");
    EXPECT_EQ(readInt("n: ", 1, 100, &v), EMS_OK);
    EXPECT_EQ(v, 42);
}

static void readInt_RetriesOnTextAndRange(void)
{
    int v = 0;
    feed("abc\n12x\n\n500\n7\n");
    EXPECT_EQ(readInt("n: ", 1, 100, &v), EMS_OK);
    EXPECT_EQ(v, 7);
    const char *out = getOutput();
    EXPECT_CONTAINS(out, "Please enter a whole number.");
    EXPECT_CONTAINS(out, "Enter a number from 1 to 100.");
}

static void readInt_AcceptsBoundaries(void)
{
    int v = 0;
    feed("1\n100\n");
    EXPECT_EQ(readInt("", 1, 100, &v), EMS_OK);
    EXPECT_EQ(v, 1);
    EXPECT_EQ(readInt("", 1, 100, &v), EMS_OK);
    EXPECT_EQ(v, 100);
}

static void readInt_RejectsJustOutsideBoundaries(void)
{
    int v = 0;
    feed("0\n101\n");
    EXPECT_EQ(readInt("", 1, 100, &v), ERR_END_OF_INPUT);
}

static void readInt_NullValueAndEof(void)
{
    int v = 0;
    EXPECT_EQ(readInt("", 1, 2, NULL), ERR_INVALID_INPUT);
    feed("");
    EXPECT_EQ(readInt("", 1, 2, &v), ERR_END_OF_INPUT);
}

static void readFloat_ValidAmount(void)
{
    double v = 0;
    feed("1234.50\n");
    EXPECT_EQ(readFloat("", 0.0, MAX_MONEY, &v), EMS_OK);
    EXPECT_DBL_EQ(v, 1234.5);
}

static void readFloat_RetriesOnBadInput(void)
{
    double v = 0;
    feed("ten\n-5\n10.5\n");
    EXPECT_EQ(readFloat("", 0.0, 100.0, &v), EMS_OK);
    EXPECT_DBL_EQ(v, 10.5);
    const char *out = getOutput();
    EXPECT_CONTAINS(out, "Please enter a valid amount.");
    EXPECT_CONTAINS(out, "Amount must be from");
}

static void readFloat_NullValueAndEof(void)
{
    double v = 0;
    EXPECT_EQ(readFloat("", 0, 1, NULL), ERR_INVALID_INPUT);
    feed("");
    EXPECT_EQ(readFloat("", 0, 1, &v), ERR_END_OF_INPUT);
}

static void readString_ValidText(void)
{
    char buf[16];
    feed("Finance\n");
    EXPECT_EQ(readString("", buf, sizeof(buf), 0), EMS_OK);
    EXPECT_STREQ(buf, "Finance");
}

static void readString_BlankAllowed(void)
{
    char buf[16] = "old";
    feed("\n");
    EXPECT_EQ(readString("", buf, sizeof(buf), 1), EMS_OK);
    EXPECT_STREQ(buf, "");
}

static void readString_RejectsBlankPipeAndTooLong(void)
{
    char buf[6];
    feed("\na|b\nabcdefgh\nabc\n");
    EXPECT_EQ(readString("", buf, sizeof(buf), 0), EMS_OK);
    EXPECT_STREQ(buf, "abc");
    const char *out = getOutput();
    EXPECT_CONTAINS(out, "Cannot be blank or contain the | character.");
    EXPECT_CONTAINS(out, "Too long (maximum 5 characters).");
}

static void readString_NullBufferAndEof(void)
{
    char buf[8];
    EXPECT_EQ(readString("", NULL, 8, 0), ERR_INVALID_INPUT);
    feed("");
    EXPECT_EQ(readString("", buf, sizeof(buf), 0), ERR_END_OF_INPUT);
}

static void readChar_ConvertsToUpperCase(void)
{
    char c = ' ';
    feed("a\n");
    EXPECT_EQ(readChar("", "PAL", &c), EMS_OK);
    EXPECT_EQ(c, 'A');
}

static void readChar_RetriesOnInvalid(void)
{
    char c = ' ';
    feed("x\nPA\nl\n");
    EXPECT_EQ(readChar("", "PAL", &c), EMS_OK);
    EXPECT_EQ(c, 'L');
    EXPECT_CONTAINS(getOutput(), "Type one of: PAL");
}

static void readChar_NullArgumentsAndEof(void)
{
    char c;
    EXPECT_EQ(readChar("", NULL, &c), ERR_INVALID_INPUT);
    EXPECT_EQ(readChar("", "PAL", NULL), ERR_INVALID_INPUT);
    feed("");
    EXPECT_EQ(readChar("", "PAL", &c), ERR_END_OF_INPUT);
}

static void readYesNo_YesAndNo(void)
{
    int a = -1;
    feed("y\nN\n");
    EXPECT_EQ(readYesNo("", &a), EMS_OK);
    EXPECT_EQ(a, 1);
    EXPECT_EQ(readYesNo("", &a), EMS_OK);
    EXPECT_EQ(a, 0);
}

static void readYesNo_RetriesAndNull(void)
{
    int a = -1;
    feed("maybe\ny\n");
    EXPECT_EQ(readYesNo("", &a), EMS_OK);
    EXPECT_EQ(a, 1);
    EXPECT_EQ(readYesNo("", NULL), ERR_INVALID_INPUT);
}

static void readDate_ValidDate(void)
{
    Date d;
    feed("29\n2\n2024\n");
    EXPECT_EQ(readDate(&d), EMS_OK);
    EXPECT_EQ(d.day, 29);
    EXPECT_EQ(d.month, 2);
    EXPECT_EQ(d.year, 2024);
}

static void readDate_RejectsImpossibleDate(void)
{
    Date d;
    feed("31\n2\n2025\n28\n2\n2025\n");
    EXPECT_EQ(readDate(&d), EMS_OK);
    EXPECT_EQ(d.day, 28);
    EXPECT_CONTAINS(getOutput(), "That date does not exist.");
}

static void readDate_NullAndEof(void)
{
    Date d;
    EXPECT_EQ(readDate(NULL), ERR_INVALID_INPUT);
    feed("5\n");
    EXPECT_EQ(readDate(&d), ERR_END_OF_INPUT);
}

static void readValidated_RetriesUntilValid(void)
{
    char buf[PHONE_SIZE];
    feed("12345\n9876543210\n");
    EXPECT_EQ(readValidated("", buf, sizeof(buf), validatePhone, "bad phone"), EMS_OK);
    EXPECT_STREQ(buf, "9876543210");
    EXPECT_CONTAINS(getOutput(), "bad phone");
}

static void readValidated_DefaultErrorTextNullAndEof(void)
{
    char buf[16];
    EXPECT_EQ(readValidated("", buf, sizeof(buf), NULL, NULL), ERR_INVALID_INPUT);
    feed("12\n");
    EXPECT_EQ(readValidated("", buf, sizeof(buf), validatePhone, NULL), ERR_END_OF_INPUT);
    EXPECT_CONTAINS(getOutput(), "Invalid value.");
}

void run_io_tests(void)
{
    RUN_TEST(setInputStream_NullRestoresStdin);
    RUN_TEST(getInputStream_DefaultsToStdin);
    RUN_TEST(setOutputStream_NullRestoresStdout);
    RUN_TEST(getOutputStream_ReturnsRedirectedStream);
    RUN_TEST(trimString_RemovesBothEnds);
    RUN_TEST(trimString_AllSpacesBecomesEmpty);
    RUN_TEST(trimString_NullAndEmptyAreSafe);
    RUN_TEST(readLine_ReadsAndTrimsLine);
    RUN_TEST(readLine_EndOfInput);
    RUN_TEST(readLine_TooLongLineIsDiscarded);
    RUN_TEST(readLine_RejectsBadBuffer);
    RUN_TEST(readLine_LastLineWithoutNewline);
    RUN_TEST(readInt_ValidNumber);
    RUN_TEST(readInt_RetriesOnTextAndRange);
    RUN_TEST(readInt_AcceptsBoundaries);
    RUN_TEST(readInt_RejectsJustOutsideBoundaries);
    RUN_TEST(readInt_NullValueAndEof);
    RUN_TEST(readFloat_ValidAmount);
    RUN_TEST(readFloat_RetriesOnBadInput);
    RUN_TEST(readFloat_NullValueAndEof);
    RUN_TEST(readString_ValidText);
    RUN_TEST(readString_BlankAllowed);
    RUN_TEST(readString_RejectsBlankPipeAndTooLong);
    RUN_TEST(readString_NullBufferAndEof);
    RUN_TEST(readChar_ConvertsToUpperCase);
    RUN_TEST(readChar_RetriesOnInvalid);
    RUN_TEST(readChar_NullArgumentsAndEof);
    RUN_TEST(readYesNo_YesAndNo);
    RUN_TEST(readYesNo_RetriesAndNull);
    RUN_TEST(readDate_ValidDate);
    RUN_TEST(readDate_RejectsImpossibleDate);
    RUN_TEST(readDate_NullAndEof);
    RUN_TEST(readValidated_RetriesUntilValid);
    RUN_TEST(readValidated_DefaultErrorTextNullAndEof);
}
