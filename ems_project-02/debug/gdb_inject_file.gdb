# FAULT INJECTION B: disk write failure in the real program
# Run:  make gdb-inject-file
# GDB makes beginSave() return NULL for employees.txt, as if the disk were
# full or read-only, to prove the program reports E007 and keeps the data in memory.
set pagination off
set confirm off
break beginSave if $_streq(fileName, "employees.txt")
run < ../debug/session_inject.txt
echo \n=== beginSave("employees.txt") reached: inject a NULL file pointer ===\n
bt 3
return (FILE *)0
delete
continue
echo \nRESULT: the program printed E007, logged it, and the employee stayed in memory.\n
quit
