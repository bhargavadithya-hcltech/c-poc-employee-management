/**
 * @file validation.c
 * @brief Pure validation helpers.
 */
#include <ctype.h>
#include <string.h>
#include <time.h>
#include "validation.h"

int validateText(const char *text)
{
    int valid = 0;

    if (text != NULL) {
        size_t i;
        for (i = 0U; text[i] != '\0'; i++) {
            if ((text[i] == '|') || (iscntrl((unsigned char)text[i]) != 0)) {
                valid = -1;
                break;
            }
            if ((valid == 0) && (isspace((unsigned char)text[i]) == 0)) {
                valid = 1;
            }
        }
    }
    return (valid == 1) ? 1 : 0;
}

int validateName(const char *name)
{
    int valid = 0;

    if ((name != NULL) && (validateText(name) == 1)) {
        size_t len;
        len = strlen(name);
        if (len < (size_t)NAME_SIZE) {
            size_t i;
            valid = 1;
            for (i = 0U; i < len; i++) {
                if ((isalpha((unsigned char)name[i]) == 0) && (name[i] != ' ') &&
                    (name[i] != '.') && (name[i] != '\'')) {
                    valid = 0;
                    break;
                }
            }
        }
    }
    return valid;
}

int validatePhone(const char *phone)
{
    int valid = 0;

    if ((phone != NULL) && (strlen(phone) == 10U)) {
        size_t i;
        valid = 1;
        for (i = 0U; i < 10U; i++) {
            if (isdigit((unsigned char)phone[i]) == 0) {
                valid = 0;
                break;
            }
        }
    }
    return valid;
}

int validateEmail(const char *email)
{
    int valid = 0;

    if (email != NULL) {
        size_t len;
        len = strlen(email);
        if ((len >= 5U) && (len < (size_t)EMAIL_SIZE)) {
            size_t i;
            size_t atPos = 0U;
            size_t atCount = 0U;
            size_t lastDot = 0U;
            valid = 1;
            for (i = 0U; i < len; i++) {
                if ((isspace((unsigned char)email[i]) != 0) || (email[i] == '|')) {
                    valid = 0;
                } else if (email[i] == '@') {
                    atCount++;
                    atPos = i;
                } else if (email[i] == '.') {
                    lastDot = i;
                } else {
                    /* ordinary character */
                }
            }
            /* exactly one '@', not first; a '.' after it with text on both sides */
            if ((atCount != 1U) || (atPos == 0U) || (lastDot <= (atPos + 1U)) || (lastDot == (len - 1U))) {
                valid = 0;
            }
        }
    }
    return valid;
}

int validateUsername(const char *username)
{
    int valid = 0;

    if (username != NULL) {
        size_t len;
        len = strlen(username);
        if ((len >= 3U) && (len < (size_t)USERNAME_SIZE)) {
            size_t i;
            valid = 1;
            for (i = 0U; i < len; i++) {
                if ((isalnum((unsigned char)username[i]) == 0) && (username[i] != '_')) {
                    valid = 0;
                    break;
                }
            }
        }
    }
    return valid;
}

int validatePassword(const char *password)
{
    int valid = 0;

    if (password != NULL) {
        size_t len;
        len = strlen(password);
        if ((len >= 6U) && (len < (size_t)PASSWORD_SIZE)) {
            size_t i;
            valid = 1;
            for (i = 0U; i < len; i++) {
                if ((isspace((unsigned char)password[i]) != 0) || (password[i] == '|')) {
                    valid = 0;
                    break;
                }
            }
        }
    }
    return valid;
}

int validateId(int id)
{
    return ((id >= 1) && (id <= MAX_ID)) ? 1 : 0;
}

int validateSalary(double amount)
{
    return ((amount > 0.0) && (amount <= MAX_MONEY)) ? 1 : 0;
}

int validateAmount(double amount)
{
    return ((amount >= 0.0) && (amount <= MAX_MONEY)) ? 1 : 0;
}

int isLeapYear(int year)
{
    int leap = 0;

    if ((year % 400) == 0) {
        leap = 1;
    } else if ((year % 100) == 0) {
        leap = 0;
    } else if ((year % 4) == 0) {
        leap = 1;
    } else {
        leap = 0;
    }
    return leap;
}

int daysInMonth(int month, int year)
{
    static const int days[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    int result = 0;

    if ((month >= 1) && (month <= 12)) {
        result = days[month - 1];
        if ((month == 2) && (isLeapYear(year) == 1)) {
            result = 29;
        }
    }
    return result;
}

int validateDate(int day, int month, int year)
{
    int valid = 0;

    if ((year >= MIN_YEAR) && (year <= MAX_YEAR) && (month >= 1) && (month <= 12) &&
        (day >= 1) && (day <= daysInMonth(month, year))) {
        valid = 1;
    }
    return valid;
}

int validateStatus(char status)
{
    return ((status == 'P') || (status == 'A') || (status == 'L')) ? 1 : 0;
}

int digitsValue(const char *text, size_t start, size_t count)
{
    int value = 0;
    size_t i;

    for (i = start; i < (start + count); i++) {
        value = (value * 10) + ((int)text[i] - (int)'0');
    }
    return value;
}

int validateTimestamp(const char *text)
{
    int valid = 0;
    static const char pattern[] = "dddd-dd-dd dd:dd:dd";

    if ((text != NULL) && (strlen(text) == ((size_t)TIMESTAMP_SIZE - 1U))) {
        size_t i;
        valid = 1;
        for (i = 0U; i < ((size_t)TIMESTAMP_SIZE - 1U); i++) {
            if (pattern[i] == 'd') {
                if (isdigit((unsigned char)text[i]) == 0) {
                    valid = 0;
                }
            } else if (text[i] != pattern[i]) {
                valid = 0;
            } else {
                /* separator matches */
            }
        }
        if ((valid == 1) &&
            ((validateDate(digitsValue(text, 8U, 2U), digitsValue(text, 5U, 2U), digitsValue(text, 0U, 4U)) == 0) ||
             (digitsValue(text, 11U, 2U) > 23) || (digitsValue(text, 14U, 2U) > 59) ||
             (digitsValue(text, 17U, 2U) > 59))) {
            valid = 0;
        }
    }
    return valid;
}

int containsIgnoreCase(const char *text, const char *pattern)
{
    int found = 0;

    if ((text != NULL) && (pattern != NULL)) {
        size_t i;
        size_t n;
        size_t m;
        n = strlen(text);
        m = strlen(pattern);
        for (i = 0U; ((i + m) <= n) && (found == 0); i++) {
            size_t j;
            j = 0U;
            while ((j < m) && (tolower((unsigned char)text[i + j]) == tolower((unsigned char)pattern[j]))) {
                j++;
            }
            if (j == m) {
                found = 1;
            }
        }
    }
    return found;
}

int equalsIgnoreCase(const char *a, const char *b)
{
    int equal = 0;

    if ((a != NULL) && (b != NULL) && (strlen(a) == strlen(b))) {
        equal = containsIgnoreCase(a, b);
    }
    return equal;
}

void getToday(Date *date)
{
    time_t now;
    const struct tm *local;

    if (date != NULL) {
        now = time(NULL);
        local = localtime(&now);
        date->day = 1;
        date->month = 1;
        date->year = MIN_YEAR;
        if (local != NULL) {
            date->day = local->tm_mday;
            date->month = local->tm_mon + 1;
            date->year = local->tm_year + 1900;
        }
    }
}

int isFutureDate(const Date *date)
{
    int future = 0;
    Date today;

    if (date != NULL) {
        getToday(&today);
        if (date->year != today.year) {
            if (date->year > today.year) {
                future = 1;
            } else {
                future = 0;
            }
        } else if (date->month != today.month) {
            if (date->month > today.month) {
                future = 1;
            } else {
                future = 0;
            }
        } else {
            if (date->day > today.day) {
                future = 1;
            } else {
                future = 0;
            }
        }
    }
    return future;
}

int isFutureMonth(int month, int year)
{
    Date today;
    int future;

    getToday(&today);
    if (year != today.year) {
        if (year > today.year) {
            future = 1;
        } else {
            future = 0;
        }
    } else {
        if (month > today.month) {
            future = 1;
        } else {
            future = 0;
        }
    }
    return future;
}
