# FAULT 3: logic error in loss of pay
# Run:  make gdb-logic
set pagination off
set confirm off
echo \n=== Step 1: stop inside the loss-of-pay calculation ===\n
break wrongLop
run
info args
echo \n=== Step 2: step over the division and inspect it ===\n
next
print perDayFactor
echo WORKING_DAYS is 30:\n
print absentDays / 30
echo \n=== Step 3: the same formula done in double gives the right answer ===\n
print basic / 30 * absentDays
next
print lop
finish
echo \nROOT CAUSE: absentDays / WORKING_DAYS is integer division, 2 / 30 = 0.\n
echo FIX: basic / WORKING_DAYS * absentDays in double, as calculateLop() does (see #ifdef FIXED).\n
quit
