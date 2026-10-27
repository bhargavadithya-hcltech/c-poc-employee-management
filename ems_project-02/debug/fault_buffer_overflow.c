/**
 * FAULT 2: buffer overflow (silent memory corruption)
 *
 * Use case : importing employee names from an old system.
 * Bug      : strcpy() copies a 70-character name into name[50] without a
 *            length check, so it runs on into the next field (phone).
 * Symptom  : no crash; the phone number silently becomes garbage.
 * GDB      : make gdb-overflow  ->  watchpoint on emp.phone stops the program
 *            at the exact write that corrupts it; bt shows strcpy <- importName.
 * Fix      : validate the name and copy with snprintf (build with -DFIXED).
 */
#include "fault_common.h"

static int importName(Employee *emp, const char *name)
{
    int status = EMS_OK;

#ifdef FIXED
    if (validateName(name) == 0) {
        (void)printf("E001: name rejected (%u characters, maximum %d)\n", (unsigned int)strlen(name),
                     NAME_SIZE - 1);
        status = ERR_INVALID_INPUT;
    } else {
        (void)snprintf(emp->name, sizeof(emp->name), "%s", name);
    }
#else
    strcpy(emp->name, name);            /* no length check */
#endif
    return status;
}

int main(void)
{
    Employee emp;
    char longName[71];
    int i;

    faultInit();
    (void)memset(&emp, 0, sizeof(emp));
    (void)snprintf(emp.phone, sizeof(emp.phone), "%s", "9876543210");
    for (i = 0; i < 70; i++) {
        longName[i] = (char)('A' + (i % 26));
    }
    longName[70] = '\0';
    (void)importName(&emp, longName);
    (void)printf("Phone after import: %.10s (expected 9876543210)\n", emp.phone);
    return 0;
}
