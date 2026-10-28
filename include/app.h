/**
 * @file app.h
 * @brief Application layer: initialization, main menu and cleanup.
 */
#ifndef EMS_APP_H
#define EMS_APP_H

#ifdef __cplusplus
extern "C" {
#endif

#define DEFAULT_DATA_DIR "."
#define DEFAULT_LOG_FILE "ems_log.txt"

/** Explicit initialization: opens the log, sets the data folder and loads users. */
int systemInit(const char *dataDir, const char *logPath);
/** Explicit cleanup: frees all memory and closes the log. */
void systemCleanup(void);
void displayMainMenu(void);
/** Runs the main menu until Save and Exit or end of input. Returns a status code. */
int mainMenu(void);
/** Full program flow: init, login, load, menu, save, cleanup. Returns the process exit code. */
int runApplication(const char *dataDir, const char *logPath);

#ifdef __cplusplus
}
#endif

#endif /* EMS_APP_H */
