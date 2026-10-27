/** @file test_validation.c  Unit tests for the validation module. */
#include "test_framework.h"



static void validateText_AcceptsNormalText(void)
{
    EXPECT_EQ(validateText("Finance Team"), 1);
}

static void validateText_RejectsBlankPipeControlAndNull(void)
{
    EXPECT_EQ(validateText(""), 0);
    EXPECT_EQ(validateText("   "), 0);
    EXPECT_EQ(validateText("a|b"), 0);
    EXPECT_EQ(validateText("ab\tc"), 0);
    EXPECT_EQ(validateText(NULL), 0);
}

static void validateName_AcceptsLettersSpacesDotsApostrophes(void)
{
    EXPECT_EQ(validateName("Ravi Kumar"), 1);
    EXPECT_EQ(validateName("A. D'Souza"), 1);
}

static void validateName_RejectsDigitsSymbolsAndLength(void)
{
    EXPECT_EQ(validateName("Ravi2"), 0);
    EXPECT_EQ(validateName("Ravi@"), 0);
    EXPECT_EQ(validateName(""), 0);
    EXPECT_EQ(validateName(NULL), 0);
    EXPECT_EQ(validateName(repeatChar(49, 'a')), 1);
    EXPECT_EQ(validateName(repeatChar(50, 'a')), 0);
}

static void validatePhone_ExactlyTenDigits(void)
{
    EXPECT_EQ(validatePhone("9876543210"), 1);
    EXPECT_EQ(validatePhone("987654321"), 0);
    EXPECT_EQ(validatePhone("98765432101"), 0);
    EXPECT_EQ(validatePhone("98765x3210"), 0);
    EXPECT_EQ(validatePhone(NULL), 0);
}

static void validateEmail_AcceptsNormalAddress(void)
{
    EXPECT_EQ(validateEmail("ravi@corp.com"), 1);
    EXPECT_EQ(validateEmail("a.b@x.co.in"), 1);
}

static void validateEmail_RejectsBadShapes(void)
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
    EXPECT_EQ(validateEmail(NULL), 0);
    char tooLong[64];
    (void)snprintf(tooLong, sizeof(tooLong), "%s@x.com", repeatChar(45, 'a'));
    EXPECT_EQ(validateEmail(tooLong), 0);
}

static void validateUsername_LengthAndCharacters(void)
{
    EXPECT_EQ(validateUsername("admin"), 1);
    EXPECT_EQ(validateUsername("hr_01"), 1);
    EXPECT_EQ(validateUsername("ab"), 0);
    EXPECT_EQ(validateUsername("abcdefghijklmnopqrst"), 0);
    EXPECT_EQ(validateUsername("bad name"), 0);
    EXPECT_EQ(validateUsername(NULL), 0);
}

static void validatePassword_LengthAndCharacters(void)
{
    EXPECT_EQ(validatePassword("secret1"), 1);
    EXPECT_EQ(validatePassword("12345"), 0);
    EXPECT_EQ(validatePassword("has space"), 0);
    EXPECT_EQ(validatePassword("pipe|123"), 0);
    EXPECT_EQ(validatePassword(repeatChar(30, 'x')), 0);
    EXPECT_EQ(validatePassword(repeatChar(29, 'x')), 1);
    EXPECT_EQ(validatePassword(NULL), 0);
}

static void validateId_Boundaries(void)
{
    EXPECT_EQ(validateId(1), 1);
    EXPECT_EQ(validateId(MAX_ID), 1);
    EXPECT_EQ(validateId(0), 0);
    EXPECT_EQ(validateId(MAX_ID + 1), 0);
    EXPECT_EQ(validateId(-5), 0);
}

static void validateSalary_MustBePositive(void)
{
    EXPECT_EQ(validateSalary(0.01), 1);
    EXPECT_EQ(validateSalary(MAX_MONEY), 1);
    EXPECT_EQ(validateSalary(0.0), 0);
    EXPECT_EQ(validateSalary(-1.0), 0);
    EXPECT_EQ(validateSalary(MAX_MONEY + 1.0), 0);
}

static void validateAmount_ZeroAllowed(void)
{
    EXPECT_EQ(validateAmount(0.0), 1);
    EXPECT_EQ(validateAmount(500.0), 1);
    EXPECT_EQ(validateAmount(-0.01), 0);
    EXPECT_EQ(validateAmount(MAX_MONEY + 1.0), 0);
}

static void isLeapYear_Rules(void)
{
    EXPECT_EQ(isLeapYear(2024), 1);
    EXPECT_EQ(isLeapYear(2000), 1);
    EXPECT_EQ(isLeapYear(2100), 0);
    EXPECT_EQ(isLeapYear(2025), 0);
}

static void daysInMonth_AllCases(void)
{
    EXPECT_EQ(daysInMonth(1, 2025), 31);
    EXPECT_EQ(daysInMonth(4, 2025), 30);
    EXPECT_EQ(daysInMonth(2, 2025), 28);
    EXPECT_EQ(daysInMonth(2, 2024), 29);
    EXPECT_EQ(daysInMonth(0, 2025), 0);
    EXPECT_EQ(daysInMonth(13, 2025), 0);
}

static void validateDate_ValidAndInvalid(void)
{
    EXPECT_EQ(validateDate(29, 2, 2024), 1);
    EXPECT_EQ(validateDate(29, 2, 2025), 0);
    EXPECT_EQ(validateDate(31, 4, 2025), 0);
    EXPECT_EQ(validateDate(0, 1, 2025), 0);
    EXPECT_EQ(validateDate(1, 1, MIN_YEAR - 1), 0);
    EXPECT_EQ(validateDate(1, 1, MAX_YEAR + 1), 0);
    EXPECT_EQ(validateDate(1, 1, MIN_YEAR), 1);
}

static void validateStatus_OnlyPAL(void)
{
    EXPECT_EQ(validateStatus('P'), 1);
    EXPECT_EQ(validateStatus('A'), 1);
    EXPECT_EQ(validateStatus('L'), 1);
    EXPECT_EQ(validateStatus('p'), 0);
    EXPECT_EQ(validateStatus('X'), 0);
}

static void containsIgnoreCase_FindsSubstring(void)
{
    EXPECT_EQ(containsIgnoreCase("Ravi Kumar", "KUM"), 1);
    EXPECT_EQ(containsIgnoreCase("Ravi Kumar", ""), 1);
    EXPECT_EQ(containsIgnoreCase("Ravi", "Ravi Kumar"), 0);
    EXPECT_EQ(containsIgnoreCase("Ravi", "xyz"), 0);
    EXPECT_EQ(containsIgnoreCase(NULL, "a"), 0);
    EXPECT_EQ(containsIgnoreCase("a", NULL), 0);
}

static void equalsIgnoreCase_WholeStringOnly(void)
{
    EXPECT_EQ(equalsIgnoreCase("Finance", "fINANCE"), 1);
    EXPECT_EQ(equalsIgnoreCase("Finance", "Fin"), 0);
    EXPECT_EQ(equalsIgnoreCase(NULL, "x"), 0);
}

static void getToday_ReturnsRealDate(void)
{
    Date d;
    getToday(&d);
    EXPECT_EQ(validateDate(d.day, d.month, d.year), 1);
    getToday(NULL);
}

static void isFutureDate_PastTodayFuture(void)
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
    EXPECT_EQ(isFutureDate(NULL), 0);
}

static void isFutureMonth_PastCurrentFuture(void)
{
    Date today;
    getToday(&today);
    EXPECT_EQ(isFutureMonth(today.month, today.year), 0);
    EXPECT_EQ(isFutureMonth(1, 2020), 0);
    EXPECT_EQ(isFutureMonth(today.month + 1, today.year), 1);
    EXPECT_EQ(isFutureMonth(1, MAX_YEAR), 1);
}

static void validateTimestamp_AcceptsRealDateTime(void)
{
    EXPECT_EQ(validateTimestamp("2025-01-01 09:00:00"), 1);
    EXPECT_EQ(validateTimestamp("2024-02-29 23:59:59"), 1);
}

static void validateTimestamp_RejectsBadShapesAndValues(void)
{
    EXPECT_EQ(validateTimestamp(NULL), 0);
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

static void digitsValue_ConvertsDigitRun(void)
{
    EXPECT_EQ(digitsValue("2025-03-07", 0U, 4U), 2025);
    EXPECT_EQ(digitsValue("2025-03-07", 5U, 2U), 3);
    EXPECT_EQ(digitsValue("2025-03-07", 8U, 2U), 7);
}

void run_validation_tests(void)
{
    RUN_TEST(validateText_AcceptsNormalText);
    RUN_TEST(validateText_RejectsBlankPipeControlAndNull);
    RUN_TEST(validateName_AcceptsLettersSpacesDotsApostrophes);
    RUN_TEST(validateName_RejectsDigitsSymbolsAndLength);
    RUN_TEST(validatePhone_ExactlyTenDigits);
    RUN_TEST(validateEmail_AcceptsNormalAddress);
    RUN_TEST(validateEmail_RejectsBadShapes);
    RUN_TEST(validateUsername_LengthAndCharacters);
    RUN_TEST(validatePassword_LengthAndCharacters);
    RUN_TEST(validateId_Boundaries);
    RUN_TEST(validateSalary_MustBePositive);
    RUN_TEST(validateAmount_ZeroAllowed);
    RUN_TEST(isLeapYear_Rules);
    RUN_TEST(daysInMonth_AllCases);
    RUN_TEST(validateDate_ValidAndInvalid);
    RUN_TEST(validateStatus_OnlyPAL);
    RUN_TEST(containsIgnoreCase_FindsSubstring);
    RUN_TEST(equalsIgnoreCase_WholeStringOnly);
    RUN_TEST(getToday_ReturnsRealDate);
    RUN_TEST(isFutureDate_PastTodayFuture);
    RUN_TEST(isFutureMonth_PastCurrentFuture);
    RUN_TEST(validateTimestamp_AcceptsRealDateTime);
    RUN_TEST(validateTimestamp_RejectsBadShapesAndValues);
    RUN_TEST(digitsValue_ConvertsDigitRun);
}
