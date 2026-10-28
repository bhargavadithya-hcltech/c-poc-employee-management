# FAULT 4: double free
# Run:  make gdb-doublefree
set pagination off
set confirm off
echo \n=== Step 1: stop after the employee is deleted ===\n
break employeeRemove
run
finish
echo \n=== Step 2: saved still holds the address of the freed node ===\n
print saved
echo \n=== Step 3: continue until glibc detects the second free ===\n
continue
echo \n=== Step 4: backtrace shows free() called from main ===\n
bt
frame function main
print saved
echo \nROOT CAUSE: employeeRemove() freed the node, then main() freed the same pointer again.\n
echo FIX: the list owns the node; never free it yourself, set saved = NULL (see #ifdef FIXED).\n
kill
quit
