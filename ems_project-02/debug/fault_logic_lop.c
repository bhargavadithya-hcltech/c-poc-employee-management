/**
 * FAULT 3: logic error (wrong result, no crash)
 *
 * Use case : monthly payroll, loss of pay for 2 absent days on a basic of 30000.
 * Bug      : integer division. absent / WORKING_DAYS = 2 / 30 = 0, so the
 *            loss of pay is always 0 and every employee is overpaid.
 * Symptom  : "LOP = 0.00", expected 2000.00.
 * GDB      : make gdb-logic  ->  break in wrongLop, step, print perDayFactor (0),
 *            print the correct expression (2000).
 * Fix      : divide as double first: basic / WORKING_DAYS * absent
 *            (the EMS uses calculateLop(); build with -DFIXED).
 */
#include "fault_common.h"

static double wrongLop(double basic, int absentDays)
{
#ifdef FIXED
    return calculateLop(basic, absentDays);
#else
    int perDayFactor = absentDays / WORKING_DAYS;   /* BUG: integer division */
    double lop = basic * (double)perDayFactor;
    return lop;
#endif
}

int main(void)
{
    double lop;

    faultInit();
    lop = wrongLop(30000.0, 2);
    (void)printf("LOP for 2 absent days on 30000.00 = %.2f (expected 2000.00)\n", lop);
    return 0;
}
