/**
 * @file validation.h
 * @brief Pure validation helpers. No input/output is done here.
 *        All functions return 1 for valid / true and 0 otherwise.
 */
#ifndef EMS_VALIDATION_H
#define EMS_VALIDATION_H

#include "common.h"

int validateText(const char *text);
int validateName(const char *name);
int validatePhone(const char *phone);
int validateEmail(const char *email);
int validateUsername(const char *username);
int validatePassword(const char *password);
int validateId(int id);
int validateSalary(double amount);
int validateAmount(double amount);
int isLeapYear(int year);
/** Returns the number of days in the month, or 0 for an invalid month. */
int daysInMonth(int month, int year);
int validateDate(int day, int month, int year);
int validateStatus(char status);
/** Converts count digit characters starting at text[start] to an int. */
int digitsValue(const char *text, size_t start, size_t count);
/** 1 when text has the form "YYYY-MM-DD HH:MM:SS" with a real date. */
int validateTimestamp(const char *text);
int containsIgnoreCase(const char *text, const char *pattern);
int equalsIgnoreCase(const char *a, const char *b);
/** Fills date with the local calendar date. */
void getToday(Date *date);
/** 1 when date is after today. */
int isFutureDate(const Date *date);
/** 1 when month/year is after the current month. */
int isFutureMonth(int month, int year);

#endif /* EMS_VALIDATION_H */
