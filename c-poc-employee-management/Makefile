# Employee Management System
#
#   make            build the program        -> ./ems
#   make run        build and run it
#   make test       unit tests               -> test_report.txt
#   make coverage   function/branch coverage -> coverage_report.txt
#   make valgrind   memory check             -> valgrind_report.txt
#   make cppcheck   static analysis + MISRA  -> cppcheck_report.txt, misra_report.txt
#   make clean      remove everything built

CC      = gcc
CFLAGS  = -std=c99 -Wall -Wextra -Wpedantic -g -Iinclude
SRC     = $(wildcard src/*.c)
MODULES = $(filter-out src/main.c, $(SRC))
TESTS   = $(wildcard tests/*.c)
TESTDEF = -D_POSIX_C_SOURCE=200809L

all: ems

ems: $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o ems

run: ems
	./ems

test_runner: $(MODULES) $(TESTS)
	$(CC) $(CFLAGS) $(TESTDEF) $(MODULES) $(TESTS) -o test_runner

test: test_runner
	./test_runner > test_report.txt
	tail -n 5 test_report.txt

coverage:
	$(CC) $(CFLAGS) $(TESTDEF) --coverage $(MODULES) $(TESTS) -o test_coverage
	./test_coverage > /dev/null
	gcov -b -f -n $(patsubst src/%.c, test_coverage-%.gcda, $(MODULES)) > coverage_report.txt
	tail -n 5 coverage_report.txt

valgrind: test_runner
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --error-exitcode=1 \
	         --log-file=valgrind_report.txt ./test_runner > /dev/null
	tail -n 12 valgrind_report.txt

# variableScope is off on purpose: all locals are declared at the top of each function.
cppcheck:
	cppcheck --enable=all --std=c99 -Iinclude --inline-suppr --suppress=missingIncludeSystem \
	         --suppress=checkersReport --suppress=variableScope src 2> cppcheck_report.txt
	cppcheck --addon=misra --enable=style --std=c99 -Iinclude --suppress=missingIncludeSystem \
	         --suppress=checkersReport src 2> misra_report.txt
	cat cppcheck_report.txt
	tail -n 20 misra_report.txt

clean:
	rm -f ems test_runner test_coverage *.gcno *.gcda *.gcov *_report.txt

.PHONY: all run test coverage valgrind cppcheck clean
