/** @file test_payroll.c  Unit tests for the Payroll module. */
#include "test_framework.h"

/* seed(): 101 basic 30000, 102 basic 24000 */
static void absent(int emp, int d)
{
    Date date = makeDate(d, 3, 2025);
    EXPECT_EQ(attendanceAdd(emp, &date, 'A', NULL), EMS_OK);
}

static void addPayroll(int emp, int m, int y)
{
    Payroll p;
    EXPECT_EQ(payrollCompute(emp, m, y, 1000.0, 500.0, &p), EMS_OK);
    EXPECT_EQ(payrollAdd(&p), EMS_OK);
}


static void payrollReset_FreesArray(void)
{
    addPayroll(101, 3, 2025);
    payrollReset();
    EXPECT_EQ(getPayrollCount(), 0);
}

static void payrollGrow_GrowsPastInitialCapacity(void)
{
    for (int m = 1; m <= 12; m++) {
        addPayroll(101, m, 2023);
        addPayroll(102, m, 2023);
    }
    EXPECT_EQ(getPayrollCount(), 24);
    EXPECT_EQ(payrollGrow(), EMS_OK);
}

static void roundMoney_RoundsHalfAwayFromZero(void)
{
    EXPECT_DBL_EQ(roundMoney(1166.6666), 1166.67);
    EXPECT_DBL_EQ(roundMoney(2.004), 2.0);
    EXPECT_DBL_EQ(roundMoney(-2.006), -2.01);
    EXPECT_DBL_EQ(roundMoney(0.0), 0.0);
}

static void calculateGross_BasicPlusAllowance(void)
{
    EXPECT_DBL_EQ(calculateGross(35000.0, 5000.0), 40000.0);
    EXPECT_DBL_EQ(calculateGross(100.0, 0.0), 100.0);
}

static void calculateLop_PerDayTimesAbsent(void)
{
    EXPECT_DBL_EQ(calculateLop(35000.0, 2), 2333.33);
    EXPECT_DBL_EQ(calculateLop(30000.0, 0), 0.0);
}

static void calculateLop_ClampsNegativeAndAboveThirty(void)
{
    EXPECT_DBL_EQ(calculateLop(30000.0, -3), 0.0);
    EXPECT_DBL_EQ(calculateLop(30000.0, 31), 30000.0);
}

static void calculateNet_NormalAndNeverNegative(void)
{
    EXPECT_DBL_EQ(calculateNet(40000.0, 2000.0, 2333.33), 35666.67);
    EXPECT_DBL_EQ(calculateNet(100.0, 500.0, 0.0), 0.0);
}

static void findPayroll_FoundAndMissing(void)
{
    addPayroll(101, 3, 2025);
    EXPECT_EQ(findPayroll(101, 3, 2025), 0);
    EXPECT_EQ(findPayroll(101, 4, 2025), -1);
    EXPECT_EQ(findPayroll(102, 3, 2025), -1);
}

static void payrollCompute_UsesAttendanceForLop(void)
{
    absent(101, 3);
    absent(101, 4);
    Payroll p;
    ASSERT_EQ(payrollCompute(101, 3, 2025, 5000.0, 2000.0, &p), EMS_OK);
    EXPECT_EQ(p.absentDays, 2);
    EXPECT_DBL_EQ(p.salary.basic, 30000.0);
    EXPECT_DBL_EQ(p.salary.gross, 35000.0);
    EXPECT_DBL_EQ(p.salary.lop, 2000.0);
    EXPECT_DBL_EQ(p.salary.net, 31000.0);
}

static void payrollCompute_LeaveIsPaid(void)
{
    Date d = makeDate(3, 3, 2025);
    ASSERT_EQ(attendanceAdd(101, &d, 'L', NULL), EMS_OK);
    Payroll p;
    ASSERT_EQ(payrollCompute(101, 3, 2025, 0.0, 0.0, &p), EMS_OK);
    EXPECT_DBL_EQ(p.salary.lop, 0.0);
    EXPECT_DBL_EQ(p.salary.net, 30000.0);
}

static void payrollCompute_ErrorCases(void)
{
    Payroll p;
    EXPECT_EQ(payrollCompute(101, 3, 2025, 0.0, 0.0, NULL), ERR_INVALID_INPUT);
    EXPECT_EQ(payrollCompute(999, 3, 2025, 0.0, 0.0, &p), ERR_NOT_FOUND);
    EXPECT_EQ(payrollCompute(101, 13, 2025, 0.0, 0.0, &p), ERR_INVALID_INPUT);
    EXPECT_EQ(payrollCompute(101, 0, 2025, 0.0, 0.0, &p), ERR_INVALID_INPUT);
    EXPECT_EQ(payrollCompute(101, 3, 1999, 0.0, 0.0, &p), ERR_INVALID_INPUT);
    EXPECT_EQ(payrollCompute(101, 1, MAX_YEAR, 0.0, 0.0, &p), ERR_INVALID_INPUT);
    EXPECT_EQ(payrollCompute(101, 3, 2025, -1.0, 0.0, &p), ERR_INVALID_INPUT);
    EXPECT_EQ(payrollCompute(101, 3, 2025, 0.0, -1.0, &p), ERR_INVALID_INPUT);
}

static void payrollCompute_ExistingMonthGivesE005(void)
{
    addPayroll(101, 3, 2025);
    Payroll p;
    EXPECT_EQ(payrollCompute(101, 3, 2025, 0.0, 0.0, &p), ERR_PAYROLL_EXISTS);
}

static void payrollCompute_DeductionsAboveGross(void)
{
    Payroll p;
    EXPECT_EQ(payrollCompute(101, 3, 2025, 0.0, 30000.01, &p), ERR_INVALID_INPUT);
    EXPECT_EQ(payrollCompute(101, 3, 2025, 0.0, 30000.0, &p), EMS_OK);
    EXPECT_DBL_EQ(p.salary.net, 0.0);
}

static void payrollAdd_ValidDuplicateAndInvalid(void)
{
    Payroll p;
    ASSERT_EQ(payrollCompute(101, 3, 2025, 0.0, 0.0, &p), EMS_OK);
    EXPECT_EQ(payrollAdd(&p), EMS_OK);
    EXPECT_EQ(payrollAdd(&p), ERR_PAYROLL_EXISTS);
    EXPECT_EQ(payrollAdd(NULL), ERR_INVALID_INPUT);
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

static void payrollRemoveEmployee_RemovesOnlyThatEmployee(void)
{
    addPayroll(101, 3, 2025);
    addPayroll(102, 3, 2025);
    addPayroll(101, 4, 2025);
    EXPECT_EQ(payrollRemoveEmployee(101), 2);
    EXPECT_EQ(getPayrollCount(), 1);
    EXPECT_EQ(getPayrollAt(0)->empId, 102);
}

static void getPayrollCount_Counts(void)
{
    EXPECT_EQ(getPayrollCount(), 0);
    addPayroll(101, 3, 2025);
    EXPECT_EQ(getPayrollCount(), 1);
}

static void getPayrollAt_Bounds(void)
{
    addPayroll(101, 3, 2025);
    EXPECT_NOT_NULL(getPayrollAt(0));
    EXPECT_NULL(getPayrollAt(1));
    EXPECT_NULL(getPayrollAt(-1));
}

static void printPayslip_ShowsAllAmounts(void)
{
    absent(101, 3);
    Payroll p;
    ASSERT_EQ(payrollCompute(101, 3, 2025, 5000.0, 2000.0, &p), EMS_OK);
    printPayslip(getOutputStream(), &p);
    printPayslip(NULL, &p);
    printPayslip(getOutputStream(), NULL);
    const char *out = getOutput();
    EXPECT_CONTAINS(out, "PAYSLIP - 03 / 2025");
    EXPECT_CONTAINS(out, "Employee          : 101 Ravi Kumar");
    EXPECT_CONTAINS(out, "Absent Days (LOP) : 1 (1000.00)");
    EXPECT_CONTAINS(out, "Net Salary        : 32000.00");
}

static void processPayroll_SavedAfterConfirm(void)
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

static void processPayroll_NotSavedWhenAnsweredNo(void)
{
    feed("101\n3\n2025\n0\n0\nn\n");
    EXPECT_EQ(processPayroll(), EMS_OK);
    EXPECT_EQ(getPayrollCount(), 0);
    EXPECT_CONTAINS(getOutput(), "Payroll not saved.");
}

static void processPayroll_UnknownEmployee(void)
{
    feed("999\n");
    EXPECT_EQ(processPayroll(), ERR_NOT_FOUND);
}

static void processPayroll_FutureMonth(void)
{
    feed("101\n1\n2100\n");
    EXPECT_EQ(processPayroll(), ERR_INVALID_INPUT);
    EXPECT_CONTAINS(getOutput(), "future month");
}

static void processPayroll_AlreadyProcessed(void)
{
    addPayroll(101, 3, 2025);
    feed("101\n3\n2025\n");
    EXPECT_EQ(processPayroll(), ERR_PAYROLL_EXISTS);
    EXPECT_CONTAINS(getOutput(), "E005");
}

static void processPayroll_DeductionsTooHigh(void)
{
    feed("101\n3\n2025\n0\n40000\n");
    EXPECT_EQ(processPayroll(), ERR_INVALID_INPUT);
    EXPECT_CONTAINS(getOutput(), "more than gross salary");
}

static void viewPayslip_FoundAndMissing(void)
{
    addPayroll(101, 3, 2025);
    feed("101\n3\n2025\n101\n4\n2025\n");
    EXPECT_EQ(viewPayslip(), EMS_OK);
    EXPECT_EQ(viewPayslip(), ERR_NOT_FOUND);
    EXPECT_CONTAINS(getOutput(), "PAYSLIP - 03 / 2025");
}

static void payrollMenu_ProcessAndView(void)
{
    feed("1\n101\n3\n2025\n0\n0\ny\n2\n101\n3\n2025\n0\n");
    payrollMenu();
    EXPECT_EQ(getPayrollCount(), 1);
}

static void payrollMenu_EndOfInput(void)
{
    feed("1\n101\n");
    payrollMenu();
    EXPECT_EQ(getPayrollCount(), 0);
}

static void payrollAdd_StampsProcessedTime(void)
{
    Payroll p;
    ASSERT_EQ(payrollCompute(101, 3, 2025, 0.0, 0.0, &p), EMS_OK);
    EXPECT_STREQ(p.processedAt, "");
    ASSERT_EQ(payrollAdd(&p), EMS_OK);
    EXPECT_EQ(validateTimestamp(getPayrollAt(0)->processedAt), 1);
}

static void payrollAdd_RejectsBadProcessedTime(void)
{
    Payroll p;
    ASSERT_EQ(payrollCompute(101, 3, 2025, 0.0, 0.0, &p), EMS_OK);
    (void)snprintf(p.processedAt, sizeof(p.processedAt), "%s", "soon");
    EXPECT_EQ(payrollAdd(&p), ERR_INVALID_INPUT);
}

static void printPayslip_ShowsProcessedTime(void)
{
    Payroll p;
    ASSERT_EQ(payrollCompute(101, 3, 2025, 0.0, 0.0, &p), EMS_OK);
    printPayslip(getOutputStream(), &p);
    EXPECT_CONTAINS(getOutput(), "Processed On      : (not saved yet)");
    (void)snprintf(p.processedAt, sizeof(p.processedAt), "%s", "2025-01-01 09:00:00");
    printPayslip(getOutputStream(), &p);
    EXPECT_CONTAINS(getOutput(), "Processed On      : 2025-01-01 09:00:00");
}

void run_payroll_tests(void)
{
    RUN_TEST_WITH(payrollReset_FreesArray, seed);
    RUN_TEST_WITH(payrollGrow_GrowsPastInitialCapacity, seed);
    RUN_TEST_WITH(roundMoney_RoundsHalfAwayFromZero, seed);
    RUN_TEST_WITH(calculateGross_BasicPlusAllowance, seed);
    RUN_TEST_WITH(calculateLop_PerDayTimesAbsent, seed);
    RUN_TEST_WITH(calculateLop_ClampsNegativeAndAboveThirty, seed);
    RUN_TEST_WITH(calculateNet_NormalAndNeverNegative, seed);
    RUN_TEST_WITH(findPayroll_FoundAndMissing, seed);
    RUN_TEST_WITH(payrollCompute_UsesAttendanceForLop, seed);
    RUN_TEST_WITH(payrollCompute_LeaveIsPaid, seed);
    RUN_TEST_WITH(payrollCompute_ErrorCases, seed);
    RUN_TEST_WITH(payrollCompute_ExistingMonthGivesE005, seed);
    RUN_TEST_WITH(payrollCompute_DeductionsAboveGross, seed);
    RUN_TEST_WITH(payrollAdd_ValidDuplicateAndInvalid, seed);
    RUN_TEST_WITH(payrollRemoveEmployee_RemovesOnlyThatEmployee, seed);
    RUN_TEST_WITH(getPayrollCount_Counts, seed);
    RUN_TEST_WITH(getPayrollAt_Bounds, seed);
    RUN_TEST_WITH(printPayslip_ShowsAllAmounts, seed);
    RUN_TEST_WITH(processPayroll_SavedAfterConfirm, seed);
    RUN_TEST_WITH(processPayroll_NotSavedWhenAnsweredNo, seed);
    RUN_TEST_WITH(processPayroll_UnknownEmployee, seed);
    RUN_TEST_WITH(processPayroll_FutureMonth, seed);
    RUN_TEST_WITH(processPayroll_AlreadyProcessed, seed);
    RUN_TEST_WITH(processPayroll_DeductionsTooHigh, seed);
    RUN_TEST_WITH(viewPayslip_FoundAndMissing, seed);
    RUN_TEST_WITH(payrollMenu_ProcessAndView, seed);
    RUN_TEST_WITH(payrollMenu_EndOfInput, seed);
    RUN_TEST_WITH(payrollAdd_StampsProcessedTime, seed);
    RUN_TEST_WITH(payrollAdd_RejectsBadProcessedTime, seed);
    RUN_TEST_WITH(printPayslip_ShowsProcessedTime, seed);
}
