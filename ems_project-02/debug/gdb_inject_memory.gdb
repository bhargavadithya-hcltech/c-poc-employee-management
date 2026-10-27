# FAULT INJECTION A: memory allocation failure in the real program
# Run:  make gdb-inject-memory
# GDB forces attendanceGrow() to fail as if malloc/realloc returned NULL,
# to prove the program reports E008 and keeps running instead of crashing.
set pagination off
set confirm off
break attendanceGrow
run < ../debug/session_inject.txt
echo \n=== attendanceGrow() reached: inject ERR_MEMORY (8) instead of growing ===\n
bt 3
return 8
continue
echo \nRESULT: the program printed E008, saved its data and exited normally.\n
quit
