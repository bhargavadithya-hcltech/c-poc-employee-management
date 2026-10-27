/**
 * @file logger.h
 * @brief Timestamped logging and error-code handling (E001 - E010).
 */
#ifndef EMS_LOGGER_H
#define EMS_LOGGER_H

#include <stdio.h>
#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

#define LOG_INFO  0
#define LOG_WARN  1
#define LOG_ERROR 2

/** Opens (appends to) the log file. Returns EMS_OK or ERR_FILE. */
int logInit(const char *path);
/** Closes the log file if open. */
void logClose(void);
/** Returns 1 when a log file is open. */
int logIsOpen(void);
/** Writes "YYYY-MM-DD HH:MM:SS" into buf. Returns EMS_OK or ERR_INVALID_INPUT. */
int getTimestamp(char *buf, size_t size);
/** Fills dest (TIMESTAMP_SIZE bytes) with given when it is a valid timestamp,
 *  or with the current time when given is NULL or empty.
 *  Returns EMS_OK or ERR_INVALID_INPUT. */
int setTimestamp(char *dest, const char *given);
/** Returns "INFO", "WARN" or "ERROR". */
const char *logLevelName(int level);
/** Writes one timestamped line: time [LEVEL] [MODULE] message. */
int logMessage(int level, const char *module, const char *message);
/** Logs an error code with detail text (never passwords or salaries). */
int logError(const char *module, EmsStatus code, const char *detail);
/** Returns "E001".."E010", or "E000" for unknown codes. */
const char *errorCodeText(EmsStatus code);
/** Returns the user-facing message for an error code. */
const char *errorMessage(EmsStatus code);
/** Prints "E00x: message" to the output stream and logs it. */
void showError(const char *module, EmsStatus code);

#ifdef __cplusplus
}
#endif

#endif /* EMS_LOGGER_H */
