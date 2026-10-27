/**
 * @file io.c
 * @brief Console input/output helpers with bounded reads.
 */
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "io.h"
#include "validation.h"

static FILE *g_in = NULL;
static FILE *g_out = NULL;

/* cppcheck-suppress unusedFunction ; part of the public API, used by the unit tests */
void setInputStream(FILE *stream)
{
    g_in = stream;
}

/* cppcheck-suppress unusedFunction ; part of the public API, used by the unit tests */
void setOutputStream(FILE *stream)
{
    g_out = stream;
}

FILE *getInputStream(void)
{
    return (g_in != NULL) ? g_in : stdin;
}

FILE *getOutputStream(void)
{
    return (g_out != NULL) ? g_out : stdout;
}

void trimString(char *text)
{
    size_t start = 0U;
    size_t len;

    if (text != NULL) {
        while ((text[start] != '\0') && (isspace((unsigned char)text[start]) != 0)) {
            start++;
        }
        if (start > 0U) {
            (void)memmove(text, &text[start], strlen(&text[start]) + 1U);
        }
        len = strlen(text);
        while ((len > 0U) && (isspace((unsigned char)text[len - 1U]) != 0)) {
            text[len - 1U] = '\0';
            len--;
        }
    }
}

int readLine(const char *prompt, char *buffer, size_t size)
{
    int status = EMS_OK;
    int ch;
    FILE *in = getInputStream();
    FILE *out = getOutputStream();

    if ((buffer == NULL) || (size < 2U)) {
        status = ERR_INVALID_INPUT;
    } else {
        if (prompt != NULL) {
            (void)fprintf(out, "%s", prompt);
            (void)fflush(out);
        }
        if (fgets(buffer, (int)size, in) == NULL) {
            buffer[0] = '\0';
            status = ERR_END_OF_INPUT;
        } else if ((strchr(buffer, '\n') == NULL) && (feof(in) == 0)) {
            /* line longer than the buffer: throw the rest away */
            ch = fgetc(in);
            while ((ch != (int)'\n') && (ch != EOF)) {
                ch = fgetc(in);
            }
            buffer[0] = '\0';
            (void)fprintf(out, "Input is too long.\n");
            status = ERR_INVALID_INPUT;
        } else {
            trimString(buffer);
        }
    }
    return status;
}

int readInt(const char *prompt, int min, int max, int *value)
{
    int status = ERR_INVALID_INPUT;
    char buf[LINE_SIZE];
    char *end = NULL;
    long number;

    if (value == NULL) {
        status = ERR_INVALID_INPUT;
    } else {
        while (status == ERR_INVALID_INPUT) {
            status = readLine(prompt, buf, sizeof(buf));
            if (status == EMS_OK) {
                errno = 0;
                number = strtol(buf, &end, 10);
                if ((buf[0] == '\0') || (end == NULL) || (*end != '\0') || (errno != 0)) {
                    (void)fprintf(getOutputStream(), "Please enter a whole number.\n");
                    status = ERR_INVALID_INPUT;
                } else if ((number < (long)min) || (number > (long)max)) {
                    (void)fprintf(getOutputStream(), "Enter a number from %d to %d.\n", min, max);
                    status = ERR_INVALID_INPUT;
                } else {
                    *value = (int)number;
                }
            }
        }
    }
    return status;
}

int readFloat(const char *prompt, double min, double max, double *value)
{
    int status = ERR_INVALID_INPUT;
    char buf[LINE_SIZE];
    char *end = NULL;
    double number;

    if (value == NULL) {
        status = ERR_INVALID_INPUT;
    } else {
        while (status == ERR_INVALID_INPUT) {
            status = readLine(prompt, buf, sizeof(buf));
            if (status == EMS_OK) {
                errno = 0;
                number = strtod(buf, &end);
                if ((buf[0] == '\0') || (end == NULL) || (*end != '\0') || (errno != 0)) {
                    (void)fprintf(getOutputStream(), "Please enter a valid amount.\n");
                    status = ERR_INVALID_INPUT;
                } else if (!((number >= min) && (number <= max))) {
                    (void)fprintf(getOutputStream(), "Amount must be from %.2f to %.2f.\n", min, max);
                    status = ERR_INVALID_INPUT;
                } else {
                    *value = number;
                }
            }
        }
    }
    return status;
}

int readString(const char *prompt, char *buffer, size_t size, int allowEmpty)
{
    int status = ERR_INVALID_INPUT;
    char buf[LINE_SIZE];

    if ((buffer == NULL) || (size < 2U)) {
        status = ERR_INVALID_INPUT;
    } else {
        while (status == ERR_INVALID_INPUT) {
            status = readLine(prompt, buf, sizeof(buf));
            if (status == EMS_OK) {
                if ((buf[0] == '\0') && (allowEmpty != 0)) {
                    buffer[0] = '\0';
                } else if (validateText(buf) == 0) {
                    (void)fprintf(getOutputStream(), "Cannot be blank or contain the | character.\n");
                    status = ERR_INVALID_INPUT;
                } else if (strlen(buf) >= size) {
                    (void)fprintf(getOutputStream(), "Too long (maximum %u characters).\n",
                                  (unsigned int)(size - 1U));
                    status = ERR_INVALID_INPUT;
                } else {
                    (void)snprintf(buffer, size, "%s", buf);
                }
            }
        }
    }
    return status;
}

int readChar(const char *prompt, const char *allowed, char *value)
{
    int status = ERR_INVALID_INPUT;
    char buf[LINE_SIZE];
    char upper;

    if ((allowed == NULL) || (value == NULL)) {
        status = ERR_INVALID_INPUT;
    } else {
        while (status == ERR_INVALID_INPUT) {
            status = readLine(prompt, buf, sizeof(buf));
            if (status == EMS_OK) {
                upper = (char)toupper((unsigned char)buf[0]);
                if ((strlen(buf) != 1U) || (strchr(allowed, (int)upper) == NULL)) {
                    (void)fprintf(getOutputStream(), "Type one of: %s\n", allowed);
                    status = ERR_INVALID_INPUT;
                } else {
                    *value = upper;
                }
            }
        }
    }
    return status;
}

int readYesNo(const char *prompt, int *answer)
{
    int status = ERR_INVALID_INPUT;
    char c = 'N';

    if (answer != NULL) {
        status = readChar(prompt, "YN", &c);
        if (status == EMS_OK) {
            if (c == 'Y') {
                *answer = 1;
            } else {
                *answer = 0;
            }
        }
    }
    return status;
}

int readDate(Date *date)
{
    int status = ERR_INVALID_INPUT;

    if (date != NULL) {
        while (status == ERR_INVALID_INPUT) {
            status = readInt("Day (1-31): ", 1, 31, &date->day);
            if (status == EMS_OK) {
                status = readInt("Month (1-12): ", 1, 12, &date->month);
            }
            if (status == EMS_OK) {
                status = readInt("Year (2000-2100): ", MIN_YEAR, MAX_YEAR, &date->year);
            }
            if ((status == EMS_OK) && (validateDate(date->day, date->month, date->year) == 0)) {
                (void)fprintf(getOutputStream(), "That date does not exist. Try again.\n");
                status = ERR_INVALID_INPUT;
            }
        }
    }
    return status;
}

int readValidated(const char *prompt, char *buffer, size_t size,
                  int (*validator)(const char *text), const char *errorText)
{
    int status = ERR_INVALID_INPUT;

    if ((buffer == NULL) || (validator == NULL)) {
        status = ERR_INVALID_INPUT;
    } else {
        while (status == ERR_INVALID_INPUT) {
            status = readString(prompt, buffer, size, 0);
            if ((status == EMS_OK) && (validator(buffer) == 0)) {
                (void)fprintf(getOutputStream(), "%s\n", (errorText != NULL) ? errorText : "Invalid value.");
                status = ERR_INVALID_INPUT;
            }
        }
    }
    return status;
}
