/** @file test_employee.c  Unit tests for the Employee module. */
#include "test_framework.h"



/* --------------------------------------------------------------- core */

static void employeeReset_FreesWholeList(void)
{
    employeeReset();
    EXPECT_EQ(getEmployeeCount(), 0);
    EXPECT_NULL(getEmployeeHead());
}

static void validateEmployeeData_ValidRecord(void)
{
    Employee e = makeEmployee(200);
    EXPECT_EQ(validateEmployeeData(&e), EMS_OK);
}

static void validateEmployeeData_EachBadField(void)
{
    Employee e = makeEmployee(200);
    EXPECT_EQ(validateEmployeeData(NULL), ERR_INVALID_INPUT);
    e.id = 0;
    EXPECT_EQ(validateEmployeeData(&e), ERR_INVALID_INPUT);
    e = makeEmployeeFull(200, "R2D2", 10, 20, 30000.0);
    EXPECT_EQ(validateEmployeeData(&e), ERR_INVALID_INPUT);
    e = makeEmployee(200);
    strcpy(e.phone, "12345");
    EXPECT_EQ(validateEmployeeData(&e), ERR_INVALID_INPUT);
    e = makeEmployee(200);
    strcpy(e.email, "no-at-sign");
    EXPECT_EQ(validateEmployeeData(&e), ERR_INVALID_INPUT);
    e = makeEmployeeFull(200, "Ravi", 10, 20, 0.0);
    EXPECT_EQ(validateEmployeeData(&e), ERR_INVALID_INPUT);
}

static void validateEmployeeData_MissingDepartmentOrRole(void)
{
    Employee e = makeEmployeeFull(200, "Ravi", 99, 20, 30000.0);
    EXPECT_EQ(validateEmployeeData(&e), ERR_DEPT_ROLE_NOT_FOUND);
    e = makeEmployeeFull(200, "Ravi", 10, 99, 30000.0);
    EXPECT_EQ(validateEmployeeData(&e), ERR_DEPT_ROLE_NOT_FOUND);
}

static void employeeInsert_KeepsListSortedById(void)
{
    Employee a = makeEmployee(150);
    Employee b = makeEmployee(50);
    ASSERT_EQ(employeeInsert(&a), EMS_OK);
    ASSERT_EQ(employeeInsert(&b), EMS_OK);
    const Employee *n = getEmployeeHead();
    int ids[4];
    for (int i = 0; i < 4; i++) {
        ASSERT_NOT_NULL(n);
        ids[i] = n->id;
        n = n->next;
    }
    EXPECT_EQ(ids[0], 50);
    EXPECT_EQ(ids[1], 101);
    EXPECT_EQ(ids[2], 102);
    EXPECT_EQ(ids[3], 150);
    EXPECT_EQ(getEmployeeCount(), 4);
}

static void employeeInsert_RejectsDuplicateAndInvalid(void)
{
    Employee dup = makeEmployee(101);
    Employee bad = makeEmployeeFull(300, "Bad", 99, 20, 30000.0);
    EXPECT_EQ(employeeInsert(&dup), ERR_DUPLICATE_ID);
    EXPECT_EQ(employeeInsert(&bad), ERR_DEPT_ROLE_NOT_FOUND);
    EXPECT_EQ(employeeInsert(NULL), ERR_INVALID_INPUT);
    EXPECT_EQ(getEmployeeCount(), 2);
}

static void findEmployee_FoundAndMissing(void)
{
    ASSERT_NOT_NULL(findEmployee(101));
    EXPECT_STREQ(findEmployee(102)->name, "Priya Sharma");
    EXPECT_NULL(findEmployee(1));
    EXPECT_NULL(findEmployee(999));
    employeeReset();
    EXPECT_NULL(findEmployee(101));
}

static void employeeUpdate_ChangesFields(void)
{
    Employee e = makeEmployeeFull(0, "Ravi K", 11, 21, 45000.0);
    strcpy(e.phone, "9000000000");
    EXPECT_EQ(employeeUpdate(101, &e), EMS_OK);
    const Employee *got = findEmployee(101);
    EXPECT_STREQ(got->name, "Ravi K");
    EXPECT_STREQ(got->phone, "9000000000");
    EXPECT_EQ(got->deptId, 11);
    EXPECT_DBL_EQ(got->basicSalary, 45000.0);
    EXPECT_EQ(got->id, 101);
}

static void employeeUpdate_NotFoundNullOrInvalid(void)
{
    Employee e = makeEmployee(0);
    EXPECT_EQ(employeeUpdate(555, &e), ERR_NOT_FOUND);
    EXPECT_EQ(employeeUpdate(101, NULL), ERR_INVALID_INPUT);
    strcpy(e.phone, "1");
    EXPECT_EQ(employeeUpdate(101, &e), ERR_INVALID_INPUT);
    EXPECT_STREQ(findEmployee(101)->phone, "9876543210");
}

static void employeeRemove_HeadMiddleTail(void)
{
    Employee c = makeEmployee(103);
    ASSERT_EQ(employeeInsert(&c), EMS_OK);
    EXPECT_EQ(employeeRemove(102), EMS_OK);
    EXPECT_EQ(employeeRemove(101), EMS_OK);
    EXPECT_EQ(employeeRemove(103), EMS_OK);
    EXPECT_EQ(getEmployeeCount(), 0);
    EXPECT_EQ(employeeRemove(101), ERR_NOT_FOUND);
}

static void employeeRemove_CascadesAttendanceAndPayroll(void)
{
    Date d = makeDate(3, 1, 2025);
    ASSERT_EQ(attendanceAdd(101, &d, 'P', NULL), EMS_OK);
    Payroll p;
    ASSERT_EQ(payrollCompute(101, 1, 2025, 0.0, 0.0, &p), EMS_OK);
    ASSERT_EQ(payrollAdd(&p), EMS_OK);
    EXPECT_EQ(employeeRemove(101), EMS_OK);
    EXPECT_EQ(getAttendanceCount(), 0);
    EXPECT_EQ(getPayrollCount(), 0);
}

static void employeeSetDepartment_Cases(void)
{
    EXPECT_EQ(employeeSetDepartment(101, 11), EMS_OK);
    EXPECT_EQ(findEmployee(101)->deptId, 11);
    EXPECT_EQ(employeeSetDepartment(999, 11), ERR_NOT_FOUND);
    EXPECT_EQ(employeeSetDepartment(101, 99), ERR_DEPT_ROLE_NOT_FOUND);
}

static void employeeSetRole_Cases(void)
{
    EXPECT_EQ(employeeSetRole(101, 21), EMS_OK);
    EXPECT_EQ(findEmployee(101)->roleId, 21);
    EXPECT_EQ(employeeSetRole(999, 21), ERR_NOT_FOUND);
    EXPECT_EQ(employeeSetRole(101, 99), ERR_DEPT_ROLE_NOT_FOUND);
}

static void employeeMatches_AllSearchTypes(void)
{
    const Employee *e = findEmployee(101);
    EXPECT_EQ(employeeMatches(e, SEARCH_BY_ID, 101, NULL), 1);
    EXPECT_EQ(employeeMatches(e, SEARCH_BY_ID, 102, NULL), 0);
    EXPECT_EQ(employeeMatches(e, SEARCH_BY_NAME, 0, "kumar"), 1);
    EXPECT_EQ(employeeMatches(e, SEARCH_BY_NAME, 0, "priya"), 0);
    EXPECT_EQ(employeeMatches(e, SEARCH_BY_DEPT, 10, NULL), 1);
    EXPECT_EQ(employeeMatches(e, SEARCH_BY_ROLE, 21, NULL), 0);
    EXPECT_EQ(employeeMatches(e, 9, 0, NULL), 0);
    EXPECT_EQ(employeeMatches(NULL, SEARCH_BY_ID, 101, NULL), 0);
}

static void employeeSearch_ByIdUsesSortedLookup(void)
{
    EXPECT_EQ(employeeSearch(SEARCH_BY_ID, 102, NULL, getOutputStream()), 1);
    EXPECT_CONTAINS(getOutput(), "Priya Sharma");
    EXPECT_EQ(employeeSearch(SEARCH_BY_ID, 103, NULL, NULL), 0);
}

static void employeeSearch_ByNameDeptRole(void)
{
    EXPECT_EQ(employeeSearch(SEARCH_BY_NAME, 0, "a", NULL), 2);
    EXPECT_EQ(employeeSearch(SEARCH_BY_DEPT, 11, NULL, getOutputStream()), 1);
    EXPECT_EQ(employeeSearch(SEARCH_BY_ROLE, 20, NULL, NULL), 1);
    EXPECT_EQ(employeeSearch(SEARCH_BY_ROLE, 99, NULL, NULL), 0);
}

static void countEmployeesInDept_Counts(void)
{
    EXPECT_EQ(countEmployeesInDept(10), 1);
    EXPECT_EQ(countEmployeesInDept(99), 0);
}

static void countEmployeesInRole_Counts(void)
{
    EXPECT_EQ(countEmployeesInRole(21), 1);
    EXPECT_EQ(countEmployeesInRole(99), 0);
}

static void getEmployeeHead_ReturnsLowestId(void)
{
    ASSERT_NOT_NULL(getEmployeeHead());
    EXPECT_EQ(getEmployeeHead()->id, 101);
}

static void getEmployeeCount_Counts(void)
{
    EXPECT_EQ(getEmployeeCount(), 2);
}

static void printEmployeeHeader_PrintsColumnsAndNullSafe(void)
{
    printEmployeeHeader(getOutputStream());
    printEmployeeHeader(NULL);
    EXPECT_CONTAINS(getOutput(), "Basic Salary");
}

static void printEmployee_ShowsDepartmentAndRoleNames(void)
{
    printEmployee(getOutputStream(), findEmployee(101));
    printEmployee(getOutputStream(), NULL);
    printEmployee(NULL, findEmployee(101));
    const char *out = getOutput();
    EXPECT_CONTAINS(out, "Finance");
    EXPECT_CONTAINS(out, "Accountant");
    EXPECT_CONTAINS(out, "30000.00");
}

static void selectDepartment_RetriesUntilExisting(void)
{
    int id = 0;
    feed("99\n11\n");
    EXPECT_EQ(selectDepartment(&id), EMS_OK);
    EXPECT_EQ(id, 11);
    EXPECT_CONTAINS(getOutput(), "E006");
}

static void selectDepartment_NoDepartmentsNullAndEof(void)
{
    int id = 0;
    EXPECT_EQ(selectDepartment(NULL), ERR_INVALID_INPUT);
    feed("");
    EXPECT_EQ(selectDepartment(&id), ERR_END_OF_INPUT);
    deptReset();
    EXPECT_EQ(selectDepartment(&id), ERR_DEPT_ROLE_NOT_FOUND);
}

static void selectRole_RetriesUntilExisting(void)
{
    int id = 0;
    feed("5\n21\n");
    EXPECT_EQ(selectRole(&id), EMS_OK);
    EXPECT_EQ(id, 21);
}

static void selectRole_NoRolesAndNull(void)
{
    int id = 0;
    EXPECT_EQ(selectRole(NULL), ERR_INVALID_INPUT);
    roleReset();
    EXPECT_EQ(selectRole(&id), ERR_DEPT_ROLE_NOT_FOUND);
}

static void logEmployeeEvent_WritesUserAndId(void)
{
    logEmployeeEvent("Tested", 101);
    EXPECT_CONTAINS(logText(), "[EMPLOYEE] Tested employee 101 by admin");
}

/* -------------------------------------------------------- interactive */

static void addEmployee_FullValidEntry(void)
{
    feed("103\nAnil Rao\n9123456780\nanil@corp.com\n10\n20\n28000\n");
    EXPECT_EQ(addEmployee(), EMS_OK);
    ASSERT_NOT_NULL(findEmployee(103));
    EXPECT_CONTAINS(readFile("employees.txt"), "103|Anil Rao|9123456780|anil@corp.com|10|20|28000.00");
    EXPECT_CONTAINS(logText(), "Added employee 103 by admin");
}

static void addEmployee_RetriesBadFields(void)
{
    feed("103\nAnil 9\nAnil Rao\n123\n9123456780\nbad\nanil@corp.com\n77\n10\n20\n0\n28000\n");
    EXPECT_EQ(addEmployee(), EMS_OK);
    const char *out = getOutput();
    EXPECT_CONTAINS(out, "Use letters");
    EXPECT_CONTAINS(out, "Phone must be exactly 10 digits.");
    EXPECT_CONTAINS(out, "valid email");
    EXPECT_CONTAINS(out, "E006");
}

static void addEmployee_DuplicateId(void)
{
    feed("101\n");
    EXPECT_EQ(addEmployee(), ERR_DUPLICATE_ID);
    EXPECT_CONTAINS(getOutput(), "E002");
}

static void addEmployee_NoDepartmentsYet(void)
{
    employeeReset();
    deptReset();
    feed("103\nAnil Rao\n9123456780\nanil@corp.com\n");
    EXPECT_EQ(addEmployee(), ERR_DEPT_ROLE_NOT_FOUND);
    EXPECT_EQ(getEmployeeCount(), 0);
}

static void addEmployee_EndOfInput(void)
{
    feed("103\nAnil Rao\n");
    EXPECT_EQ(addEmployee(), ERR_END_OF_INPUT);
    EXPECT_NULL(findEmployee(103));
}

static void viewEmployee_ListsAll(void)
{
    EXPECT_EQ(viewEmployee(), 2);
    EXPECT_CONTAINS(getOutput(), "Total employees: 2");
}

static void viewEmployee_EmptyList(void)
{
    employeeReset();
    EXPECT_EQ(viewEmployee(), 0);
    EXPECT_CONTAINS(getOutput(), "No employees yet.");
}

static void searchEmployee_EachType(void)
{
    feed("1\n101\n2\npriya\n3\n10\n4\n21\n");
    EXPECT_EQ(searchEmployee(), EMS_OK);
    EXPECT_EQ(searchEmployee(), EMS_OK);
    EXPECT_EQ(searchEmployee(), EMS_OK);
    EXPECT_EQ(searchEmployee(), EMS_OK);
    EXPECT_CONTAINS(getOutput(), "1 match(es).");
}

static void searchEmployee_NoMatchGivesE003(void)
{
    feed("2\nzzz\n");
    EXPECT_EQ(searchEmployee(), ERR_NOT_FOUND);
    EXPECT_CONTAINS(getOutput(), "E003");
}

static void updateEmployee_EachField(void)
{
    feed("101\n1\nRavi Kumar S\n101\n2\n9000000001\n101\n3\nrks@corp.com\n101\n4\n41000\n");
    EXPECT_EQ(updateEmployee(), EMS_OK);
    EXPECT_EQ(updateEmployee(), EMS_OK);
    EXPECT_EQ(updateEmployee(), EMS_OK);
    EXPECT_EQ(updateEmployee(), EMS_OK);
    const Employee *e = findEmployee(101);
    EXPECT_STREQ(e->name, "Ravi Kumar S");
    EXPECT_STREQ(e->phone, "9000000001");
    EXPECT_STREQ(e->email, "rks@corp.com");
    EXPECT_DBL_EQ(e->basicSalary, 41000.0);
    EXPECT_CONTAINS(readFile("employees.txt"), "41000.00");
}

static void updateEmployee_CancelNotFoundAndEof(void)
{
    feed("101\n0\n555\n101\n1\n");
    EXPECT_EQ(updateEmployee(), EMS_OK);
    EXPECT_EQ(updateEmployee(), ERR_NOT_FOUND);
    EXPECT_EQ(updateEmployee(), ERR_END_OF_INPUT);
    EXPECT_STREQ(findEmployee(101)->name, "Ravi Kumar");
}

static void deleteEmployee_ConfirmedYes(void)
{
    feed("102\ny\n");
    EXPECT_EQ(deleteEmployee(), EMS_OK);
    EXPECT_NULL(findEmployee(102));
    EXPECT_NOT_CONTAINS(readFile("employees.txt"), "102|");
}

static void deleteEmployee_AnsweredNo(void)
{
    feed("102\nn\n");
    EXPECT_EQ(deleteEmployee(), EMS_OK);
    EXPECT_NOT_NULL(findEmployee(102));
    EXPECT_CONTAINS(getOutput(), "Delete cancelled.");
}

static void deleteEmployee_NotFound(void)
{
    feed("999\n");
    EXPECT_EQ(deleteEmployee(), ERR_NOT_FOUND);
}

static void assignDepartment_Changes(void)
{
    feed("101\n11\n");
    EXPECT_EQ(assignDepartment(), EMS_OK);
    EXPECT_EQ(findEmployee(101)->deptId, 11);
    EXPECT_CONTAINS(getOutput(), "set to HR");
}

static void assignDepartment_EmployeeMissing(void)
{
    feed("999\n");
    EXPECT_EQ(assignDepartment(), ERR_NOT_FOUND);
}

static void assignRole_Changes(void)
{
    feed("101\n21\n");
    EXPECT_EQ(assignRole(), EMS_OK);
    EXPECT_EQ(findEmployee(101)->roleId, 21);
}

static void assignRole_EmployeeMissing(void)
{
    feed("999\n");
    EXPECT_EQ(assignRole(), ERR_NOT_FOUND);
}

static void employeeMenu_AdminRunsEveryOption(void)
{
    feed("2\n3\n1\n101\n6\n101\n11\n7\n101\n21\n4\n101\n0\n5\n102\ny\n"
         "1\n104\nAnil Rao\n9123456780\nanil@corp.com\n10\n20\n28000\n0\n");
    employeeMenu();
    EXPECT_NULL(findEmployee(102));
    EXPECT_NOT_NULL(findEmployee(104));
    EXPECT_EQ(findEmployee(101)->deptId, 11);
    EXPECT_EQ(findEmployee(101)->roleId, 21);
}

static void employeeMenu_OperatorBlockedFromAdminOptions(void)
{
    setSession("hr_user", ROLE_OPERATOR);
    feed("5\n6\n7\n0\n");
    employeeMenu();
    EXPECT_NOT_NULL(findEmployee(102));
    EXPECT_EQ(countOccurrences(getOutput(), "E010"), 3);
}

static void employeeMenu_EndOfInputInsideAction(void)
{
    feed("1\n105\n");
    employeeMenu();
    EXPECT_NULL(findEmployee(105));
}

static void employeeInsert_StampsCreatedAndUpdatedTime(void)
{
    Employee e = makeEmployee(150);
    ASSERT_EQ(employeeInsert(&e), EMS_OK);
    EXPECT_EQ(validateTimestamp(findEmployee(150)->createdAt), 1);
    EXPECT_STREQ(findEmployee(150)->updatedAt, findEmployee(150)->createdAt);
}

static void employeeInsert_KeepsTimeFromFileAndRejectsBadTime(void)
{
    Employee e = makeEmployee(150);
    Employee bad = makeEmployee(151);
    (void)snprintf(e.createdAt, sizeof(e.createdAt), "%s", "2025-01-01 09:00:00");
    (void)snprintf(bad.createdAt, sizeof(bad.createdAt), "%s", "not a time");
    ASSERT_EQ(employeeInsert(&e), EMS_OK);
    EXPECT_STREQ(findEmployee(150)->createdAt, "2025-01-01 09:00:00");
    EXPECT_EQ(employeeInsert(&bad), ERR_INVALID_INPUT);
    EXPECT_NULL(findEmployee(151));
}

static void employeeUpdate_ChangesUpdatedTimeOnly(void)
{
    Employee e = makeEmployee(150);
    (void)snprintf(e.createdAt, sizeof(e.createdAt), "%s", "2025-01-01 09:00:00");
    ASSERT_EQ(employeeInsert(&e), EMS_OK);
    e.basicSalary = 50000.0;
    ASSERT_EQ(employeeUpdate(150, &e), EMS_OK);
    EXPECT_STREQ(findEmployee(150)->createdAt, "2025-01-01 09:00:00");
    EXPECT_NE(strcmp(findEmployee(150)->updatedAt, "2025-01-01 09:00:00"), 0);
    EXPECT_EQ(validateTimestamp(findEmployee(150)->updatedAt), 1);
}

static void employeeSetDepartment_ChangesUpdatedTime(void)
{
    Employee e = makeEmployee(150);
    (void)snprintf(e.createdAt, sizeof(e.createdAt), "%s", "2025-01-01 09:00:00");
    ASSERT_EQ(employeeInsert(&e), EMS_OK);
    ASSERT_EQ(employeeSetDepartment(150, 11), EMS_OK);
    EXPECT_NE(strcmp(findEmployee(150)->updatedAt, "2025-01-01 09:00:00"), 0);
}

static void employeeSetRole_ChangesUpdatedTime(void)
{
    Employee e = makeEmployee(150);
    (void)snprintf(e.createdAt, sizeof(e.createdAt), "%s", "2025-01-01 09:00:00");
    ASSERT_EQ(employeeInsert(&e), EMS_OK);
    ASSERT_EQ(employeeSetRole(150, 21), EMS_OK);
    EXPECT_NE(strcmp(findEmployee(150)->updatedAt, "2025-01-01 09:00:00"), 0);
}

static void addEmployee_ShowsCreationTime(void)
{
    feed("103\nAnil Rao\n9123456780\nanil@corp.com\n10\n20\n28000\n");
    ASSERT_EQ(addEmployee(), EMS_OK);
    EXPECT_CONTAINS(getOutput(), "Employee 103 added at ");
    EXPECT_CONTAINS(getOutput(), findEmployee(103)->createdAt);
}

void run_employee_tests(void)
{
    RUN_TEST_WITH(employeeReset_FreesWholeList, seed);
    RUN_TEST_WITH(validateEmployeeData_ValidRecord, seed);
    RUN_TEST_WITH(validateEmployeeData_EachBadField, seed);
    RUN_TEST_WITH(validateEmployeeData_MissingDepartmentOrRole, seed);
    RUN_TEST_WITH(employeeInsert_KeepsListSortedById, seed);
    RUN_TEST_WITH(employeeInsert_RejectsDuplicateAndInvalid, seed);
    RUN_TEST_WITH(findEmployee_FoundAndMissing, seed);
    RUN_TEST_WITH(employeeUpdate_ChangesFields, seed);
    RUN_TEST_WITH(employeeUpdate_NotFoundNullOrInvalid, seed);
    RUN_TEST_WITH(employeeRemove_HeadMiddleTail, seed);
    RUN_TEST_WITH(employeeRemove_CascadesAttendanceAndPayroll, seed);
    RUN_TEST_WITH(employeeSetDepartment_Cases, seed);
    RUN_TEST_WITH(employeeSetRole_Cases, seed);
    RUN_TEST_WITH(employeeMatches_AllSearchTypes, seed);
    RUN_TEST_WITH(employeeSearch_ByIdUsesSortedLookup, seed);
    RUN_TEST_WITH(employeeSearch_ByNameDeptRole, seed);
    RUN_TEST_WITH(countEmployeesInDept_Counts, seed);
    RUN_TEST_WITH(countEmployeesInRole_Counts, seed);
    RUN_TEST_WITH(getEmployeeHead_ReturnsLowestId, seed);
    RUN_TEST_WITH(getEmployeeCount_Counts, seed);
    RUN_TEST_WITH(printEmployeeHeader_PrintsColumnsAndNullSafe, seed);
    RUN_TEST_WITH(printEmployee_ShowsDepartmentAndRoleNames, seed);
    RUN_TEST_WITH(selectDepartment_RetriesUntilExisting, seed);
    RUN_TEST_WITH(selectDepartment_NoDepartmentsNullAndEof, seed);
    RUN_TEST_WITH(selectRole_RetriesUntilExisting, seed);
    RUN_TEST_WITH(selectRole_NoRolesAndNull, seed);
    RUN_TEST_WITH(logEmployeeEvent_WritesUserAndId, seed);
    RUN_TEST_WITH(addEmployee_FullValidEntry, seed);
    RUN_TEST_WITH(addEmployee_RetriesBadFields, seed);
    RUN_TEST_WITH(addEmployee_DuplicateId, seed);
    RUN_TEST_WITH(addEmployee_NoDepartmentsYet, seed);
    RUN_TEST_WITH(addEmployee_EndOfInput, seed);
    RUN_TEST_WITH(viewEmployee_ListsAll, seed);
    RUN_TEST_WITH(viewEmployee_EmptyList, seed);
    RUN_TEST_WITH(searchEmployee_EachType, seed);
    RUN_TEST_WITH(searchEmployee_NoMatchGivesE003, seed);
    RUN_TEST_WITH(updateEmployee_EachField, seed);
    RUN_TEST_WITH(updateEmployee_CancelNotFoundAndEof, seed);
    RUN_TEST_WITH(deleteEmployee_ConfirmedYes, seed);
    RUN_TEST_WITH(deleteEmployee_AnsweredNo, seed);
    RUN_TEST_WITH(deleteEmployee_NotFound, seed);
    RUN_TEST_WITH(assignDepartment_Changes, seed);
    RUN_TEST_WITH(assignDepartment_EmployeeMissing, seed);
    RUN_TEST_WITH(assignRole_Changes, seed);
    RUN_TEST_WITH(assignRole_EmployeeMissing, seed);
    RUN_TEST_WITH(employeeMenu_AdminRunsEveryOption, seed);
    RUN_TEST_WITH(employeeMenu_OperatorBlockedFromAdminOptions, seed);
    RUN_TEST_WITH(employeeMenu_EndOfInputInsideAction, seed);
    RUN_TEST_WITH(employeeInsert_StampsCreatedAndUpdatedTime, seed);
    RUN_TEST_WITH(employeeInsert_KeepsTimeFromFileAndRejectsBadTime, seed);
    RUN_TEST_WITH(employeeUpdate_ChangesUpdatedTimeOnly, seed);
    RUN_TEST_WITH(employeeSetDepartment_ChangesUpdatedTime, seed);
    RUN_TEST_WITH(employeeSetRole_ChangesUpdatedTime, seed);
    RUN_TEST_WITH(addEmployee_ShowsCreationTime, seed);
}
