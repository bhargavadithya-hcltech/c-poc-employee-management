/** @file test_attendance.c  Unit tests for the Attendance module. */
#include "test_framework.h"

static void mark(int emp, int d, int m, int y, char s)
{
    Date date = makeDate(d, m, y);
    EXPECT_EQ(attendanceAdd(emp, &date, s, NULL), EMS_OK);
}


static void attendanceReset_FreesArray(void)
{
    mark(101, 1, 3, 2025, 'P');
    attendanceReset();
    EXPECT_EQ(getAttendanceCount(), 0);
    EXPECT_NULL(getAttendanceAt(0));
}

static void attendanceGrow_GrowsPastInitialCapacity(void)
{
    for (int d = 1; d <= 31; d++) {
        mark(101, d, 1, 2025, 'P');
    }
    EXPECT_EQ(getAttendanceCount(), 31);
    EXPECT_EQ(attendanceGrow(), EMS_OK);
    EXPECT_EQ(getAttendanceAt(30)->date.day, 31);
}

static void attendanceAdd_ValidRecord(void)
{
    Date d = makeDate(5, 3, 2025);
    EXPECT_EQ(attendanceAdd(101, &d, 'A', NULL), EMS_OK);
    EXPECT_EQ(getAttendanceCount(), 1);
    EXPECT_EQ(getAttendanceAt(0)->status, 'A');
}

static void attendanceAdd_UnknownEmployee(void)
{
    Date d = makeDate(5, 3, 2025);
    EXPECT_EQ(attendanceAdd(999, &d, 'P', NULL), ERR_NOT_FOUND);
}

static void attendanceAdd_BadDateStatusOrNull(void)
{
    Date bad = makeDate(30, 2, 2025);
    Date ok = makeDate(1, 3, 2025);
    EXPECT_EQ(attendanceAdd(101, &bad, 'P', NULL), ERR_INVALID_ATTENDANCE);
    EXPECT_EQ(attendanceAdd(101, &ok, 'X', NULL), ERR_INVALID_ATTENDANCE);
    EXPECT_EQ(attendanceAdd(101, NULL, 'P', NULL), ERR_INVALID_ATTENDANCE);
}

static void attendanceAdd_FutureDateRejected(void)
{
    Date future = makeDate(1, 1, MAX_YEAR);
    EXPECT_EQ(attendanceAdd(101, &future, 'P', NULL), ERR_INVALID_ATTENDANCE);
}

static void attendanceAdd_DuplicateDateRejected(void)
{
    mark(101, 5, 3, 2025, 'P');
    Date d = makeDate(5, 3, 2025);
    EXPECT_EQ(attendanceAdd(101, &d, 'A', NULL), ERR_INVALID_ATTENDANCE);
    EXPECT_EQ(attendanceAdd(102, &d, 'A', NULL), EMS_OK);
}

static void attendanceAdd_LockedAfterPayroll(void)
{
    Payroll p;
    ASSERT_EQ(payrollCompute(101, 3, 2025, 0.0, 0.0, &p), EMS_OK);
    ASSERT_EQ(payrollAdd(&p), EMS_OK);
    Date d = makeDate(6, 3, 2025);
    EXPECT_EQ(attendanceAdd(101, &d, 'P', NULL), ERR_INVALID_ATTENDANCE);
}

static void findAttendance_FoundAndMissing(void)
{
    mark(101, 5, 3, 2025, 'P');
    mark(102, 5, 3, 2025, 'L');
    EXPECT_EQ(findAttendance(102, 5, 3, 2025), 1);
    EXPECT_EQ(findAttendance(101, 6, 3, 2025), -1);
}

static void isMonthLocked_OnlyWhenPayrollExists(void)
{
    EXPECT_EQ(isMonthLocked(101, 3, 2025), 0);
    Payroll p;
    ASSERT_EQ(payrollCompute(101, 3, 2025, 0.0, 0.0, &p), EMS_OK);
    ASSERT_EQ(payrollAdd(&p), EMS_OK);
    EXPECT_EQ(isMonthLocked(101, 3, 2025), 1);
    EXPECT_EQ(isMonthLocked(101, 4, 2025), 0);
}

static void getMonthlySummary_CountsOneMonthOnly(void)
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

static void getMonthlySummary_MonthZeroCountsAll(void)
{
    mark(101, 1, 3, 2025, 'A');
    mark(101, 1, 4, 2025, 'A');
    int p = 0, a = 0, l = 0;
    getMonthlySummary(101, 0, 0, &p, &a, &l);
    EXPECT_EQ(a, 2);
}

static void getMonthlySummary_NullPointersAreSafe(void)
{
    mark(101, 1, 3, 2025, 'A');
    getMonthlySummary(101, 3, 2025, NULL, NULL, NULL);
    int a = 0;
    getMonthlySummary(999, 3, 2025, NULL, &a, NULL);
    EXPECT_EQ(a, 0);
}

static void attendanceRemoveEmployee_RemovesOnlyThatEmployee(void)
{
    mark(101, 1, 3, 2025, 'P');
    mark(102, 1, 3, 2025, 'P');
    mark(101, 2, 3, 2025, 'P');
    EXPECT_EQ(attendanceRemoveEmployee(101), 2);
    EXPECT_EQ(getAttendanceCount(), 1);
    EXPECT_EQ(getAttendanceAt(0)->empId, 102);
    EXPECT_EQ(attendanceRemoveEmployee(555), 0);
}

static void getAttendanceCount_Counts(void)
{
    EXPECT_EQ(getAttendanceCount(), 0);
    mark(101, 1, 3, 2025, 'P');
    EXPECT_EQ(getAttendanceCount(), 1);
}

static void getAttendanceAt_Bounds(void)
{
    mark(101, 1, 3, 2025, 'P');
    EXPECT_NOT_NULL(getAttendanceAt(0));
    EXPECT_NULL(getAttendanceAt(1));
    EXPECT_NULL(getAttendanceAt(-1));
}

static void statusText_AllValues(void)
{
    EXPECT_STREQ(statusText('P'), "Present");
    EXPECT_STREQ(statusText('A'), "Absent");
    EXPECT_STREQ(statusText('L'), "Leave");
    EXPECT_STREQ(statusText('Q'), "Unknown");
}

static void recordAttendance_ValidEntrySavedAndLogged(void)
{
    feed("101\n5\n3\n2025\na\n");
    EXPECT_EQ(recordAttendance(), EMS_OK);
    EXPECT_CONTAINS(readFile("attendance.txt"), "101|5|3|2025|A|");
    EXPECT_EQ(validateTimestamp(getAttendanceAt(0)->recordedAt), 1);
    EXPECT_CONTAINS(getOutput(), "(entered at ");
    EXPECT_CONTAINS(logText(), "Attendance A for employee 101 on 05-03-2025 by admin");
}

static void recordAttendance_UnknownEmployee(void)
{
    feed("999\n");
    EXPECT_EQ(recordAttendance(), ERR_NOT_FOUND);
}

static void recordAttendance_DuplicateShowsE004(void)
{
    mark(101, 5, 3, 2025, 'P');
    feed("101\n5\n3\n2025\nP\n");
    EXPECT_EQ(recordAttendance(), ERR_INVALID_ATTENDANCE);
    EXPECT_CONTAINS(getOutput(), "E004");
}

static void recordAttendance_FutureDateMessage(void)
{
    feed("101\n1\n1\n2100\nP\n");
    EXPECT_EQ(recordAttendance(), ERR_INVALID_ATTENDANCE);
    EXPECT_CONTAINS(getOutput(), "future date");
}

static void recordAttendance_LockedMonthMessage(void)
{
    Payroll p;
    ASSERT_EQ(payrollCompute(101, 3, 2025, 0.0, 0.0, &p), EMS_OK);
    ASSERT_EQ(payrollAdd(&p), EMS_OK);
    feed("101\n9\n3\n2025\nP\n");
    EXPECT_EQ(recordAttendance(), ERR_INVALID_ATTENDANCE);
    EXPECT_CONTAINS(getOutput(), "Attendance is locked.");
}

static void recordAttendance_EndOfInput(void)
{
    feed("101\n5\n");
    EXPECT_EQ(recordAttendance(), ERR_END_OF_INPUT);
}

static void attendanceSummary_PrintsCounts(void)
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

static void attendanceSummary_UnknownEmployee(void)
{
    feed("999\n");
    EXPECT_EQ(attendanceSummary(), ERR_NOT_FOUND);
}

static void attendanceMenu_RecordThenSummary(void)
{
    feed("1\n101\n4\n3\n2025\nP\n2\n101\n3\n2025\n0\n");
    attendanceMenu();
    EXPECT_EQ(getAttendanceCount(), 1);
    EXPECT_CONTAINS(getOutput(), "Present Days      : 1");
}

static void attendanceMenu_EndOfInput(void)
{
    feed("1\n101\n");
    attendanceMenu();
    EXPECT_EQ(getAttendanceCount(), 0);
}

static void attendanceAdd_StampsRecordedTime(void)
{
    Date d = makeDate(5, 3, 2025);
    ASSERT_EQ(attendanceAdd(101, &d, 'P', NULL), EMS_OK);
    EXPECT_EQ(validateTimestamp(getAttendanceAt(0)->recordedAt), 1);
    d.day = 6;
    ASSERT_EQ(attendanceAdd(101, &d, 'P', "2025-01-01 09:00:00"), EMS_OK);
    EXPECT_STREQ(getAttendanceAt(1)->recordedAt, "2025-01-01 09:00:00");
}

static void attendanceAdd_RejectsBadRecordedTime(void)
{
    Date d = makeDate(5, 3, 2025);
    EXPECT_EQ(attendanceAdd(101, &d, 'P', "05/03/2025"), ERR_INVALID_ATTENDANCE);
    EXPECT_EQ(getAttendanceCount(), 0);
}

void run_attendance_tests(void)
{
    RUN_TEST_WITH(attendanceReset_FreesArray, seed);
    RUN_TEST_WITH(attendanceGrow_GrowsPastInitialCapacity, seed);
    RUN_TEST_WITH(attendanceAdd_ValidRecord, seed);
    RUN_TEST_WITH(attendanceAdd_UnknownEmployee, seed);
    RUN_TEST_WITH(attendanceAdd_BadDateStatusOrNull, seed);
    RUN_TEST_WITH(attendanceAdd_FutureDateRejected, seed);
    RUN_TEST_WITH(attendanceAdd_DuplicateDateRejected, seed);
    RUN_TEST_WITH(attendanceAdd_LockedAfterPayroll, seed);
    RUN_TEST_WITH(findAttendance_FoundAndMissing, seed);
    RUN_TEST_WITH(isMonthLocked_OnlyWhenPayrollExists, seed);
    RUN_TEST_WITH(getMonthlySummary_CountsOneMonthOnly, seed);
    RUN_TEST_WITH(getMonthlySummary_MonthZeroCountsAll, seed);
    RUN_TEST_WITH(getMonthlySummary_NullPointersAreSafe, seed);
    RUN_TEST_WITH(attendanceRemoveEmployee_RemovesOnlyThatEmployee, seed);
    RUN_TEST_WITH(getAttendanceCount_Counts, seed);
    RUN_TEST_WITH(getAttendanceAt_Bounds, seed);
    RUN_TEST_WITH(statusText_AllValues, seed);
    RUN_TEST_WITH(recordAttendance_ValidEntrySavedAndLogged, seed);
    RUN_TEST_WITH(recordAttendance_UnknownEmployee, seed);
    RUN_TEST_WITH(recordAttendance_DuplicateShowsE004, seed);
    RUN_TEST_WITH(recordAttendance_FutureDateMessage, seed);
    RUN_TEST_WITH(recordAttendance_LockedMonthMessage, seed);
    RUN_TEST_WITH(recordAttendance_EndOfInput, seed);
    RUN_TEST_WITH(attendanceSummary_PrintsCounts, seed);
    RUN_TEST_WITH(attendanceSummary_UnknownEmployee, seed);
    RUN_TEST_WITH(attendanceMenu_RecordThenSummary, seed);
    RUN_TEST_WITH(attendanceMenu_EndOfInput, seed);
    RUN_TEST_WITH(attendanceAdd_StampsRecordedTime, seed);
    RUN_TEST_WITH(attendanceAdd_RejectsBadRecordedTime, seed);
}
