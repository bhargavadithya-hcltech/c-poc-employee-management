# =====================================================================
# Employee Management System - build, test and quality gates
#
#  Build / run         make app | make run | make debug
#  Testing             make coverage   (GTest unit + integration, coverage table)
#                      make cunit      (CUnit runner, output -> cunit_report.txt)
#  Memory              make valgrind   (interactive program -> valgrind_report.txt)
#                      make valgrind-tests (all GTest cases -> valgrind_tests_report.txt)
#  Secure coding       make misra      (strict GCC diagnostics, must be 0 warnings)
#                      make cppcheck   (-> cppcheck_report.txt, must be empty)
#  Optimization        make optimize   (.s size at -O0..-O3 -> optimization_report.txt)
#  Multithreading      make helgrind-race-buggy | helgrind-race-fixed
#                      make helgrind-deadlock-buggy | helgrind-deadlock-fixed
#                      make helgrind-ems-buggy | helgrind-ems-fixed
#                      make thread-performance
#  Debugging (GDB)     make gdb-null | gdb-overflow | gdb-logic | gdb-doublefree
#                      make gdb-inject-memory | gdb-inject-file
#                      make valgrind-doublefree | make faults-fixed
#  Everything          make all-checks
#  Cleanup             make clean
# =====================================================================

CC             := gcc
CXX            := g++
CFLAGS         := -std=c99 -Iinclude
MISRA_FLAGS    := -std=c99 -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wunused-parameter -fanalyzer -Iinclude
COVERAGE_FLAGS := -O0 -g --coverage -Iinclude
LDFLAGS        := -lgtest -lgtest_main -pthread --coverage
TEST_DEFS      := -D_POSIX_C_SOURCE=200809L
THREAD_FLAGS   := -std=c99 -Wall -Wextra -g -pthread $(TEST_DEFS) -Iinclude
DEBUG_FLAGS    := -std=c99 -Wall -Wextra -g -O0 -Iinclude

# Source definitions
SRCS           := $(wildcard src/*.c)
MODULES        := $(filter-out src/main.c, $(SRCS))
OBJS           := $(notdir $(MODULES:.c=.o)) test_shim.o
TEST_SRCS      := tests/test_shim.c tests/test_ems.cpp
CONCURRENCY    := tests/concurrency

# lcov 2.x needs extra flags for GTest code; lcov 1.x does not accept them
LCOV_MAJOR     := $(shell lcov --version 2>/dev/null | sed -n 's/.*[Vv]ersion \([0-9]*\).*/\1/p')
ifeq ($(LCOV_MAJOR),2)
LCOV_OPTS      := --rc branch_coverage=1 --ignore-errors inconsistent,mismatch,unused
else
LCOV_OPTS      := --rc lcov_branch_coverage=1
endif

.PHONY: all app run debug coverage cunit misra cppcheck valgrind valgrind-tests optimize \
        helgrind-race-buggy helgrind-race-fixed helgrind-deadlock-buggy helgrind-deadlock-fixed \
        helgrind-ems-buggy helgrind-ems-fixed thread-performance \
        gdb-null gdb-overflow gdb-logic gdb-doublefree gdb-inject-memory gdb-inject-file \
        valgrind-doublefree faults-fixed all-checks clean

all: app

# ---------------------------------------------------------------- 1. build
app:
	$(CC) $(CFLAGS) -g $(SRCS) -o ems_app

run: app
	./ems_app

debug:
	$(CC) $(DEBUG_FLAGS) $(SRCS) -o ems_debug

# ---------------------------------------------------- 2. GTest + coverage
#    modules and the shim are compiled as C, the test file as C++
coverage:
	@echo "==> Compiling Unit Tests with Coverage Instrumentation..."
	$(CC) $(CFLAGS) $(TEST_DEFS) $(COVERAGE_FLAGS) -c $(MODULES) tests/test_shim.c
	$(CXX) $(COVERAGE_FLAGS) $(OBJS) tests/test_ems.cpp $(LDFLAGS) -o test_runner
	@echo "==> Running GTest Unit and Integration Tests..."
	./test_runner
	@echo "==> Extracting and Filtering Coverage Data..."
	@lcov --capture --directory . --output-file coverage.info --quiet $(LCOV_OPTS)
	@lcov --remove coverage.info '/usr/*' '*/tests/*' --output-file filtered.info --quiet $(LCOV_OPTS)
	@echo "========================================================================"
	@echo "                       TERMINAL COVERAGE SUMMARY                        "
	@echo "========================================================================"
	@lcov --summary filtered.info $(LCOV_OPTS)
	@echo "========================================================================"
	@echo "Module Function & Branch Details:"
	@gcov -b -n $(notdir $(MODULES:.c=.gcda)) | grep -A 4 "File 'src/"

# ------------------------------------------------------------- 3. CUnit
cunit:
	$(CC) $(CFLAGS) $(TEST_DEFS) -Wall -Wextra -g $(MODULES) tests/test_shim.c tests/cunit/test_cunit.c -lcunit -o cunit_runner
	./cunit_runner | tee cunit_report.txt

# ----------------------------------------------------------- 4. MISRA-C
misra:
	@echo "==> Checking MISRA-C Compliance via GCC Diagnostics..."
	$(CC) $(MISRA_FLAGS) $(SRCS) -o /dev/null
	@echo "==> MISRA-C Check PASSED: 0 warnings, 0 errors."

# ---------------------------------------------------------- 5. CPPCheck
cppcheck:
	@echo "==> Running CPPCheck Static Analysis..."
	cppcheck --enable=all --suppress=missingIncludeSystem --suppress=checkersReport --std=c99 -Iinclude src/ 2> cppcheck_report.txt
	@cat cppcheck_report.txt
	@echo "==> CPPCheck Complete. Output logged to cppcheck_report.txt."

# ---------------------------------------------------------- 6. Valgrind
#    interactive: use the menus, then choose 7 to save and exit
valgrind: app
	@echo "==> Running Valgrind Dynamic Memory Analysis..."
	valgrind --leak-check=full \
	         --show-leak-kinds=all \
	         --track-origins=yes \
	         --log-file=valgrind_report.txt \
	         ./ems_app
	@cat valgrind_report.txt

#    non-interactive: every GTest case under Valgrind
valgrind-tests: coverage
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --error-exitcode=1 \
	         --log-file=valgrind_tests_report.txt ./test_runner > /dev/null
	@tail -n 12 valgrind_tests_report.txt

# ------------------------------------------------------ 7. Optimization
optimize:
	@rm -rf asm && mkdir -p asm/O0 asm/O1 asm/O2 asm/O3
	@for lvl in 0 1 2 3; do \
	    for f in $(SRCS); do $(CC) $(CFLAGS) -O$$lvl -S $$f -o asm/O$$lvl/$$(basename $$f .c).s; done; \
	    $(CC) $(CFLAGS) -O$$lvl $(SRCS) -o asm/ems_O$$lvl; \
	done
	@{ echo "ASSEMBLY (.s) FILE SIZE IN BYTES PER MODULE"; \
	   printf "%-16s %10s %10s %10s %10s\n" "Module" "-O0" "-O1" "-O2" "-O3"; \
	   for f in $(SRCS); do m=$$(basename $$f .c); \
	       printf "%-16s %10s %10s %10s %10s\n" $$m.s $$(wc -c < asm/O0/$$m.s) $$(wc -c < asm/O1/$$m.s) \
	              $$(wc -c < asm/O2/$$m.s) $$(wc -c < asm/O3/$$m.s); done; \
	   printf "%-16s %10s %10s %10s %10s\n" "TOTAL bytes" $$(cat asm/O0/*.s | wc -c) $$(cat asm/O1/*.s | wc -c) \
	          $$(cat asm/O2/*.s | wc -c) $$(cat asm/O3/*.s | wc -c); \
	   printf "%-16s %10s %10s %10s %10s\n" "TOTAL lines" $$(cat asm/O0/*.s | wc -l) $$(cat asm/O1/*.s | wc -l) \
	          $$(cat asm/O2/*.s | wc -l) $$(cat asm/O3/*.s | wc -l); \
	   echo; echo "EXECUTABLE SIZE (size command, bytes)"; size asm/ems_O0 asm/ems_O1 asm/ems_O2 asm/ems_O3; \
	   echo; echo "-O0: no optimization, every variable kept in memory (best for GDB)."; \
	   echo "-O1: removes dead stores and redundant loads -> smallest code."; \
	   echo "-O2: adds scheduling and more inlining -> faster, a little larger."; \
	   echo "-O3: aggressive inlining, loop unrolling, vectorization -> fastest, largest code."; \
	} | tee optimization_report.txt

# --------------------------------------------------- 8. Multithreading
bin_race_buggy: $(CONCURRENCY)/test_thread_race.c
	$(CC) $(THREAD_FLAGS) $< -o $@

bin_race_fixed: $(CONCURRENCY)/test_thread_race.c
	$(CC) $(THREAD_FLAGS) -DUSE_MUTEX $< -o $@

bin_deadlock_buggy: $(CONCURRENCY)/test_deadlock.c
	$(CC) $(THREAD_FLAGS) $< -o $@

bin_deadlock_fixed: $(CONCURRENCY)/test_deadlock.c
	$(CC) $(THREAD_FLAGS) -DFIX_DEADLOCK $< -o $@

bin_ems_threads_buggy: $(CONCURRENCY)/test_ems_threads.c $(MODULES)
	$(CC) $(THREAD_FLAGS) $(MODULES) $< -o $@

bin_ems_threads_fixed: $(CONCURRENCY)/test_ems_threads.c $(MODULES)
	$(CC) $(THREAD_FLAGS) -DUSE_MUTEX $(MODULES) $< -o $@

helgrind-race-buggy: bin_race_buggy
	@echo "==> Running Helgrind on Unsynchronized Counter (Expected to flag Data Race)..."
	-valgrind --tool=helgrind ./bin_race_buggy

helgrind-race-fixed: bin_race_fixed
	@echo "==> Running Helgrind on Mutex-Protected Counter (Expected 0 errors)..."
	valgrind --tool=helgrind ./bin_race_fixed

helgrind-deadlock-buggy: bin_deadlock_buggy
	@echo "==> Running Helgrind on Inverted Lock Order (Expected to flag Lock-Order Cycle)..."
	-valgrind --tool=helgrind ./bin_deadlock_buggy

helgrind-deadlock-fixed: bin_deadlock_fixed
	@echo "==> Running Helgrind on Fixed Lock Hierarchy (Expected 0 errors)..."
	valgrind --tool=helgrind ./bin_deadlock_fixed

helgrind-ems-buggy: bin_ems_threads_buggy
	@echo "==> Helgrind: 4 operators record attendance with NO lock (Expected data races)..."
	-valgrind --tool=helgrind ./bin_ems_threads_buggy

helgrind-ems-fixed: bin_ems_threads_fixed
	@echo "==> Helgrind: 4 operators record attendance WITH a mutex (Expected 0 errors)..."
	valgrind --tool=helgrind ./bin_ems_threads_fixed

thread-performance: $(CONCURRENCY)/test_performance.c
	$(CC) -std=c99 -Wall -Wextra -O2 -pthread $(TEST_DEFS) $< -o bin_performance
	./bin_performance

# ------------------------------------------- 9. GDB and fault injection
bin_fault_null: debug/fault_null_pointer.c $(MODULES)
	$(CC) $(DEBUG_FLAGS) $(MODULES) $< -o $@

bin_fault_overflow: debug/fault_buffer_overflow.c $(MODULES)
	$(CC) $(DEBUG_FLAGS) $(MODULES) $< -o $@

bin_fault_logic: debug/fault_logic_lop.c $(MODULES)
	$(CC) $(DEBUG_FLAGS) $(MODULES) $< -o $@

bin_fault_doublefree: debug/fault_double_free.c $(MODULES)
	$(CC) $(DEBUG_FLAGS) $(MODULES) $< -o $@

gdb-null: bin_fault_null
	-gdb -q -batch -x debug/gdb_null_pointer.gdb ./bin_fault_null

gdb-overflow: bin_fault_overflow
	-gdb -q -batch -x debug/gdb_buffer_overflow.gdb ./bin_fault_overflow

gdb-logic: bin_fault_logic
	-gdb -q -batch -x debug/gdb_logic_lop.gdb ./bin_fault_logic

gdb-doublefree: bin_fault_doublefree
	-gdb -q -batch -x debug/gdb_double_free.gdb ./bin_fault_doublefree

valgrind-doublefree: bin_fault_doublefree
	-valgrind --leak-check=full ./bin_fault_doublefree

#    fault injection into the real program (runs in a scratch folder gdb_run/)
gdb-inject-memory: debug
	@rm -rf gdb_run && mkdir gdb_run
	-cd gdb_run && gdb -q -batch -x ../debug/gdb_inject_memory.gdb ../ems_debug
	@grep "E008" gdb_run/ems_log.txt

gdb-inject-file: debug
	@rm -rf gdb_run && mkdir gdb_run
	-cd gdb_run && gdb -q -batch -x ../debug/gdb_inject_file.gdb ../ems_debug
	@grep "E007" gdb_run/ems_log.txt

#    the same four programs built with -DFIXED: all run cleanly
faults-fixed:
	@for f in null_pointer buffer_overflow logic_lop double_free; do \
	    echo "==> fault_$$f (FIXED)"; \
	    $(CC) $(DEBUG_FLAGS) -DFIXED $(MODULES) debug/fault_$$f.c -o bin_fixed_$$f && ./bin_fixed_$$f; \
	done

# ------------------------------------------------------- 10. everything
all-checks: app misra cppcheck coverage cunit valgrind-tests optimize thread-performance \
            helgrind-race-fixed helgrind-deadlock-fixed helgrind-ems-fixed faults-fixed

# ------------------------------------------------------------ 11. clean
clean:
	rm -rf ems_app ems_debug test_runner cunit_runner *.o *.gcno *.gcda *.info *.gcov asm gdb_run \
	       bin_race_buggy bin_race_fixed bin_deadlock_buggy bin_deadlock_fixed \
	       bin_ems_threads_buggy bin_ems_threads_fixed bin_performance \
	       bin_fault_null bin_fault_overflow bin_fault_logic bin_fault_doublefree bin_fixed_* \
	       cppcheck_report.txt valgrind_report.txt valgrind_tests_report.txt cunit_report.txt \
	       optimization_report.txt
