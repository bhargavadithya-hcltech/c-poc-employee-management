/**
 * @file io.h
 * @brief Console input/output helpers. All user input goes through these
 *        bounded fgets()-based readers. The streams can be redirected so
 *        that interactive functions can be unit tested.
 */
#ifndef EMS_IO_H
#define EMS_IO_H

#include <stdio.h>
#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Redirects input (NULL restores stdin). */
void setInputStream(FILE *stream);
/** Redirects output (NULL restores stdout). */
void setOutputStream(FILE *stream);
FILE *getInputStream(void);
FILE *getOutputStream(void);

/** Removes leading and trailing white space in place. */
void trimString(char *text);
/** Prints prompt, reads one line (newline removed, trimmed).
 *  Returns EMS_OK, ERR_INVALID_INPUT (line too long) or ERR_END_OF_INPUT. */
int readLine(const char *prompt, char *buffer, size_t size);
/** Reads a whole number in [min, max], asking again on bad input. */
int readInt(const char *prompt, int min, int max, int *value);
/** Reads a decimal number in [min, max], asking again on bad input. */
int readFloat(const char *prompt, double min, double max, double *value);
/** Reads text without '|' (blank allowed only if allowEmpty). */
int readString(const char *prompt, char *buffer, size_t size, int allowEmpty);
/** Reads one character that must appear in allowed (case-insensitive).
 *  The upper-case character is stored in value. */
int readChar(const char *prompt, const char *allowed, char *value);
/** Reads y/n. answer = 1 for yes, 0 for no. */
int readYesNo(const char *prompt, int *answer);
/** Reads text and repeats until validator(text) returns 1.
 *  errorText is printed after each rejected entry. */
int readValidated(const char *prompt, char *buffer, size_t size,
                  int (*validator)(const char *text), const char *errorText);
/** Reads day, month and year until they form a real calendar date. */
int readDate(Date *date);

#ifdef __cplusplus
}
#endif

#endif /* EMS_IO_H */
