/**
 * FAULT 4: double free / dangling pointer (crash, SIGABRT)
 *
 * Use case : an employee is deleted, then a later cleanup step frees the
 *            same employee node again through a saved pointer.
 * Bug      : employeeRemove() already freed the node; free(saved) frees it
 *            a second time.
 * Symptom  : "free(): double free detected" and the program aborts.
 * GDB      : make gdb-doublefree  ->  bt shows main -> free; print saved
 *            shows the address that employeeRemove() already freed.
 * Valgrind : make valgrind-doublefree  ->  "Invalid free()".
 * Fix      : the list owns its nodes. Never free a node yourself; set the
 *            saved pointer to NULL after employeeRemove() (build with -DFIXED).
 */
#include <stdlib.h>
#include "fault_common.h"

int main(void)
{
    Employee *saved;

    faultSetup();
    saved = findEmployee(101);
    (void)printf("Deleting employee %d\n", saved->id);
    (void)employeeRemove(101);         /* frees the node */
#ifdef FIXED
    saved = NULL;                      /* pointer is no longer valid */
#else
    free(saved);                       /* BUG: second free of the same node */
#endif
    freeAllMemory();
    (void)printf("Finished normally.\n");
    return 0;
}
