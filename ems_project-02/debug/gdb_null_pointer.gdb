# FAULT 1: NULL pointer dereference
# Run:  make gdb-null
set pagination off
set confirm off
echo \n=== Step 1: run the program until it crashes ===\n
run
echo \n=== Step 2: where did it crash? (backtrace) ===\n
bt
echo \n=== Step 3: look at the variables in the crashing function ===\n
info args
info locals
echo \n=== Step 4: the pointer is NULL, the ID does not exist ===\n
print emp
print findEmployee(999)
print findEmployee(101)->name
echo \nROOT CAUSE: findEmployee(999) returned NULL and emp->id was read without a check.\n
echo FIX: check emp == NULL and report E003 (see #ifdef FIXED in fault_null_pointer.c).\n
kill
quit
