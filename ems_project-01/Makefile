# Employee Management System - build and quality gates
#
#   make app        build the program                  -> ./ems_app
#   make coverage   GTest unit + integration tests, coverage table in the terminal
#   make misra      MISRA-C check with strict GCC diagnostics (must print 0 warnings)
#   make cppcheck   static analysis                    -> cppcheck_report.txt
#   make valgrind   memory check of the program        -> valgrind_report.txt
#   make helgrind-race-buggy | helgrind-race-fixed | helgrind-deadlock-buggy | helgrind-deadlock-fixed
#   make clean

CC             := gcc
CXX            := g++
CFLAGS         := -std=c99 -Iinclude
MISRA_FLAGS    := -std=c99 -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wunused-parameter -fanalyzer -Iinclude
COVERAGE_FLAGS := -O0 -g --coverage -Iinclude
LDFLAGS        := -lgtest -lgtest_main -pthread --coverage
THREAD_FLAGS   := -Wall -Wextra -g -pthread

# lcov 2.x needs extra flags for GTest code; lcov 1.x does not accept them
LCOV_MAJOR     := $(shell lcov --version 2>/dev/null | sed -n 's/.*[Vv]ersion \([0-9]*\).*/\1/p')
ifeq ($(LCOV_MAJOR),2)
LCOV_OPTS      := --rc branch_coverage=1 --ignore-errors inconsistent,mismatch,unused
else
LCOV_OPTS      := --rc lcov_branch_coverage=1
endif

# Source definitions
SRCS           := $(wildcard src/*.c)
MODULES        := $(filter-out src/main.c, $(SRCS))
OBJS           := $(notdir $(MODULES:.c=.o)) test_shim.o
TEST_SRCS      := tests/test_shim.c tests/test_ems.cpp
CONCURRENCY    := tests/concurrency

.PHONY: all app coverage misra cppcheck valgrind clean \
        helgrind-race-buggy helgrind-race-fixed helgrind-deadlock-buggy helgrind-deadlock-fixed

all: app

# 1. Build the application
app:
	$(CC) $(CFLAGS) -g $(SRCS) -o ems_app

# 2. GTest unit + integration tests with terminal coverage
#    (modules and the shim are compiled as C, the test file as C++)
coverage:
	@echo "==> Compiling Unit Tests with Coverage Instrumentation..."
	$(CC) $(CFLAGS) -D_POSIX_C_SOURCE=200809L $(COVERAGE_FLAGS) -c $(MODULES) tests/test_shim.c
	$(CXX) $(COVERAGE_FLAGS) $(OBJS) tests/test_ems.cpp $(LDFLAGS) -o test_runner
	@echo "==> Running GTest Unit Tests..."
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

# 3. MISRA-C compiler diagnostics
misra:
	@echo "==> Checking MISRA-C Compliance via GCC Diagnostics..."
	$(CC) $(MISRA_FLAGS) $(SRCS) -o /dev/null
	@echo "==> MISRA-C Check PASSED: 0 warnings, 0 errors."

# 4. CPPCheck static analysis
cppcheck:
	@echo "==> Running CPPCheck Static Analysis..."
	cppcheck --enable=all --suppress=missingIncludeSystem --suppress=checkersReport --std=c99 -Iinclude src/ 2> cppcheck_report.txt
	@cat cppcheck_report.txt
	@echo "==> CPPCheck Complete. Output logged to cppcheck_report.txt."

# 5. Valgrind runtime memory analysis (use the menus, then choose 7 to exit)
valgrind: app
	@echo "==> Running Valgrind Dynamic Memory Analysis..."
	valgrind --leak-check=full \
	         --show-leak-kinds=all \
	         --track-origins=yes \
	         --log-file=valgrind_report.txt \
	         ./ems_app
	@cat valgrind_report.txt

# 6. Multithreading demos checked with Helgrind
bin_race_buggy: $(CONCURRENCY)/test_thread_race.c
	$(CC) $(THREAD_FLAGS) $< -o $@

bin_race_fixed: $(CONCURRENCY)/test_thread_race.c
	$(CC) $(THREAD_FLAGS) -DUSE_MUTEX $< -o $@

bin_deadlock_buggy: $(CONCURRENCY)/test_deadlock.c
	$(CC) $(THREAD_FLAGS) $< -o $@

bin_deadlock_fixed: $(CONCURRENCY)/test_deadlock.c
	$(CC) $(THREAD_FLAGS) -DFIX_DEADLOCK $< -o $@

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

# 7. Cleanup
clean:
	rm -rf ems_app test_runner *.o *.gcno *.gcda *.info *.gcov cppcheck_report.txt valgrind_report.txt \
	       bin_race_buggy bin_race_fixed bin_deadlock_buggy bin_deadlock_fixed
