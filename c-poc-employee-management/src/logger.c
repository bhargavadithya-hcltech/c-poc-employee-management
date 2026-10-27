/**
 * @file logger.c
 * @brief Timestamped logging and error-code handling.
 */
#include <string.h>
#include <time.h>
#include "logger.h"
#include "io.h"
#include "validation.h"

static FILE *g_logFile = NULL;

int logInit(const char *path)
{
    int status = EMS_OK;

    logClose();
    if (path == NULL) {
        status = ERR_INVALID_INPUT;
    } else {
        g_logFile = fopen(path, "a");
        if (g_logFile == NULL) {
            status = ERR_FILE;
        }
    }
    return status;
}

void logClose(void)
{
    if (g_logFile != NULL) {
        (void)fclose(g_logFile);
        g_logFile = NULL;
    }
}

/* cppcheck-suppress unusedFunction ; part of the public API, used by the unit tests */
int logIsOpen(void)
{
    return (g_logFile != NULL) ? 1 : 0;
}

int getTimestamp(char *buf, size_t size)
{
    int status = EMS_OK;
    time_t now;
    const struct tm *local;

    if ((buf == NULL) || (size < 20U)) {
        status = ERR_INVALID_INPUT;
    } else {
        now = time(NULL);
        local = localtime(&now);
        if ((local == NULL) || (strftime(buf, size, "%Y-%m-%d %H:%M:%S", local) == 0U)) {
            (void)snprintf(buf, size, "%s", "0000-00-00 00:00:00");
        }
    }
    return status;
}

int setTimestamp(char *dest, const char *given)
{
    int status = EMS_OK;

    if (dest == NULL) {
        status = ERR_INVALID_INPUT;
    } else if ((given == NULL) || (given[0] == '\0')) {
        status = getTimestamp(dest, TIMESTAMP_SIZE);
    } else if (validateTimestamp(given) == 1) {
        (void)snprintf(dest, TIMESTAMP_SIZE, "%s", given);
    } else {
        status = ERR_INVALID_INPUT;
    }
    return status;
}

const char *logLevelName(int level)
{
    const char *name = "INFO";

    if (level == LOG_WARN) {
        name = "WARN";
    } else if (level == LOG_ERROR) {
        name = "ERROR";
    } else {
        name = "INFO";
    }
    return name;
}

int logMessage(int level, const char *module, const char *message)
{
    int status = EMS_OK;
    char stamp[32];

    if ((module == NULL) || (message == NULL)) {
        status = ERR_INVALID_INPUT;
    } else if (g_logFile == NULL) {
        status = ERR_FILE;
    } else {
        (void)getTimestamp(stamp, sizeof(stamp));
        (void)fprintf(g_logFile, "%s [%s] [%s] %s\n", stamp, logLevelName(level), module, message);
        (void)fflush(g_logFile);
    }
    return status;
}

int logError(const char *module, EmsStatus code, const char *detail)
{
    char line[LINE_SIZE];

    (void)snprintf(line, sizeof(line), "%s %s%s%s", errorCodeText(code), errorMessage(code),
                   (detail != NULL) ? " | " : "", (detail != NULL) ? detail : "");
    return logMessage(LOG_ERROR, module, line);
}

const char *errorCodeText(EmsStatus code)
{
    static const char *const codes[] = {
        "E000", "E001", "E002", "E003", "E004", "E005",
        "E006", "E007", "E008", "E009", "E010"
    };
    const char *text = "E000";

    if ((code >= EMS_OK) && (code <= ERR_ACCESS_DENIED)) {
        text = codes[code];
    }
    return text;
}

const char *errorMessage(EmsStatus code)
{
    const char *msg;

    switch (code) {
    case EMS_OK:                  msg = "Success."; break;
    case ERR_INVALID_INPUT:       msg = "Invalid input. Please try again."; break;
    case ERR_DUPLICATE_ID:        msg = "ID already exists."; break;
    case ERR_NOT_FOUND:           msg = "Record not found."; break;
    case ERR_INVALID_ATTENDANCE:  msg = "Invalid or duplicate attendance entry."; break;
    case ERR_PAYROLL_EXISTS:      msg = "Payroll already generated for this month."; break;
    case ERR_DEPT_ROLE_NOT_FOUND: msg = "Department or role not found."; break;
    case ERR_FILE:                msg = "File could not be opened or written."; break;
    case ERR_MEMORY:              msg = "Memory allocation failed."; break;
    case ERR_LOGIN:               msg = "Invalid username or password."; break;
    case ERR_ACCESS_DENIED:       msg = "Access denied for your role."; break;
    case ERR_END_OF_INPUT:        msg = "Input closed."; break;
    case ERR_LIMIT_REACHED:       msg = "Storage limit reached."; break;
    default:                      msg = "Unknown error."; break;
    }
    return msg;
}

void showError(const char *module, EmsStatus code)
{
    (void)fprintf(getOutputStream(), "%s: %s\n", errorCodeText(code), errorMessage(code));
    (void)logError((module != NULL) ? module : "EMS", code, NULL);
}
