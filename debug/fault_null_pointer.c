/**
 * FAULT 1: NULL pointer dereference (crash, SIGSEGV)
 *
 * Use case : HR prints the payslip header for an employee ID typed by the user.
 * Bug      : findEmployee() returns NULL for an unknown ID (999) and the code
 *            uses the pointer without checking it.
 * Symptom  : "Segmentation fault".
 * GDB      : make gdb-null  ->  run, bt, frame, print emp  (emp = 0x0)
 * Fix      : check for NULL and report E003 (build with -DFIXED).
 */
#include "fault_common.h"

static void printPayslipHeader(int id)
{
    const Employee *emp = findEmployee(id);

#ifdef FIXED
    if (emp == NULL) {
        (void)printf("E003: Record not found (employee %d)\n", id);
        return;
    }
#endif
    (void)printf("Payslip for employee %d: %s\n", emp->id, emp->name);   /* emp may be NULL */
}

int main(void)
{
    faultSetup();
    printPayslipHeader(101);
    printPayslipHeader(999);
    freeAllMemory();
    (void)printf("Finished normally.\n");
    return 0;
}
