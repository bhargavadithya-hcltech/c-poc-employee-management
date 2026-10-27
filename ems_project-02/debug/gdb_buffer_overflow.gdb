# FAULT 2: buffer overflow
# Run:  make gdb-overflow
set pagination off
set confirm off
echo \n=== Step 1: stop where the name is imported ===\n
break importName
run
echo \n=== Step 2: phone before the copy, and the size of the name field ===\n
print emp->phone
print sizeof(emp->name)
print (int)strlen(name)
echo \n=== Step 3: watch the phone field, stop when anything writes to it ===\n
watch -l emp->phone[0]
continue
echo \n=== Step 4: who wrote into phone? ===\n
bt
echo \n=== Step 5: phone is now part of the name ===\n
up
print emp->phone
delete
continue
echo \nROOT CAUSE: strcpy() wrote 70 characters into name[50] and ran into phone.\n
echo FIX: validateName() + snprintf(emp->name, sizeof(emp->name), ...) (see #ifdef FIXED).\n
quit
