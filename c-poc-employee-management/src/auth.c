/**
 * @file auth.c
 * @brief Session & Auth Module.
 */
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "auth.h"
#include "filemanager.h"
#include "io.h"
#include "logger.h"
#include "validation.h"

#define USERS_FILE "users.txt"
#define HASH_MASK  0xFFFFFFFFUL

static User g_users[MAX_USERS];
static int g_userCount = 0;
static char g_currentUser[USERNAME_SIZE];   /* zero-initialised: nobody logged in */
static int g_currentRole = ROLE_NONE;

void authReset(void)
{
    (void)memset(g_users, 0, sizeof(g_users));
    g_userCount = 0;
    logout();
}

unsigned long hashPassword(const char *username, const char *password)
{
    unsigned long hash = 5381UL;
    size_t i;

    if ((username != NULL) && (password != NULL)) {
        for (i = 0U; username[i] != '\0'; i++) {
            hash = ((hash * 33UL) + (unsigned long)(unsigned char)tolower((unsigned char)username[i])) & HASH_MASK;
        }
        hash = ((hash * 33UL) + (unsigned long)':') & HASH_MASK;
        for (i = 0U; password[i] != '\0'; i++) {
            hash = ((hash * 33UL) + (unsigned long)(unsigned char)password[i]) & HASH_MASK;
        }
    }
    return hash;
}

int findUser(const char *username)
{
    int index = -1;
    int i;

    for (i = 0; i < g_userCount; i++) {
        if (equalsIgnoreCase(g_users[i].username, username) == 1) {
            index = i;
            break;
        }
    }
    return index;
}

int addUserHashed(const char *username, unsigned long hash, int role, const char *createdAt)
{
    int status = EMS_OK;

    if ((validateUsername(username) == 0) || ((role != ROLE_ADMIN) && (role != ROLE_OPERATOR))) {
        status = ERR_INVALID_INPUT;
    } else if (findUser(username) >= 0) {
        status = ERR_DUPLICATE_ID;
    } else if (g_userCount >= MAX_USERS) {
        status = ERR_LIMIT_REACHED;
    } else if (setTimestamp(g_users[g_userCount].createdAt, createdAt) != EMS_OK) {
        status = ERR_INVALID_INPUT;
    } else {
        (void)snprintf(g_users[g_userCount].username, sizeof(g_users[g_userCount].username), "%s", username);
        g_users[g_userCount].passwordHash = hash & HASH_MASK;
        g_users[g_userCount].role = role;
        g_userCount++;
    }
    return status;
}

int addUserRecord(const char *username, const char *password, int role)
{
    int status;

    if (validatePassword(password) == 0) {
        status = ERR_INVALID_INPUT;
    } else {
        status = addUserHashed(username, hashPassword(username, password), role, NULL);
    }
    return status;
}

int verifyCredentials(const char *username, const char *password)
{
    int role = ROLE_NONE;
    int index = findUser(username);

    if ((index >= 0) && (password != NULL) &&
        (g_users[index].passwordHash == hashPassword(g_users[index].username, password))) {
        role = g_users[index].role;
    }
    return role;
}

void setSession(const char *username, int role)
{
    if ((username != NULL) && ((role == ROLE_ADMIN) || (role == ROLE_OPERATOR))) {
        (void)snprintf(g_currentUser, sizeof(g_currentUser), "%s", username);
        g_currentRole = role;
    } else {
        logout();
    }
}

void logout(void)
{
    char msg[LINE_SIZE];

    if (g_currentRole != ROLE_NONE) {
        (void)snprintf(msg, sizeof(msg), "%s logged out", g_currentUser);
        (void)logMessage(LOG_INFO, "AUTH", msg);
    }
    g_currentUser[0] = '\0';
    g_currentRole = ROLE_NONE;
}

int getCurrentRole(void)
{
    return g_currentRole;
}

const char *getCurrentUser(void)
{
    return (g_currentUser[0] != '\0') ? g_currentUser : "-";
}

const char *roleName(int role)
{
    const char *name = "Unknown";

    if (role == ROLE_ADMIN) {
        name = "Administrator";
    } else if (role == ROLE_OPERATOR) {
        name = "HR Operator";
    } else {
        name = "Unknown";
    }
    return name;
}

/* cppcheck-suppress unusedFunction ; part of the public API, used by the unit tests */
int getUserCount(void)
{
    return g_userCount;
}

/* cppcheck-suppress unusedFunction ; part of the public API, used by the unit tests */
const User *getUserAt(int index)
{
    return ((index >= 0) && (index < g_userCount)) ? &g_users[index] : NULL;
}

int checkAccess(int requiredRole)
{
    int allowed = 0;
    char msg[LINE_SIZE];

    if (g_currentRole == ROLE_ADMIN) {
        allowed = 1;
    } else if ((g_currentRole == ROLE_OPERATOR) && (requiredRole == ROLE_OPERATOR)) {
        allowed = 1;
    } else {
        allowed = 0;
    }
    if (allowed == 0) {
        (void)fprintf(getOutputStream(), "%s: %s\n", errorCodeText(ERR_ACCESS_DENIED),
                      errorMessage(ERR_ACCESS_DENIED));
        (void)snprintf(msg, sizeof(msg), "user %s (%s) needs %s", getCurrentUser(), roleName(g_currentRole),
                       roleName(requiredRole));
        (void)logError("AUTH", ERR_ACCESS_DENIED, msg);
    }
    return allowed;
}

int loadUsers(void)
{
    int status = EMS_OK;
    char path[PATH_SIZE];
    char line[LINE_SIZE];
    char name[USERNAME_SIZE];
    char created[TIMESTAMP_SIZE];
    unsigned long hash = 0UL;
    int role = 0;
    int bad = 0;
    FILE *fp;

    status = buildDataPath(USERS_FILE, path, sizeof(path));
    if (status == EMS_OK) {
        fp = fopen(path, "r");
        if (fp == NULL) {
            (void)logMessage(LOG_WARN, "AUTH", "users.txt not found, first-run setup needed");
        } else {
            while (fgets(line, (int)sizeof(line), fp) != NULL) {
                if ((sscanf(line, "%19[^|]|%lu|%d|%19[^|\n]", name, &hash, &role, created) != 4) ||
                    (addUserHashed(name, hash, role, created) != EMS_OK)) {
                    bad++;
                }
            }
            (void)fclose(fp);
            reportBadLines(USERS_FILE, bad);
        }
    }
    return status;
}

int saveUsers(void)
{
    int status;
    char tmp[PATH_SIZE];
    int i;
    FILE *fp = beginSave(USERS_FILE, tmp, sizeof(tmp));

    if (fp != NULL) {
        for (i = 0; i < g_userCount; i++) {
            (void)fprintf(fp, "%s|%lu|%d|%s\n", g_users[i].username, g_users[i].passwordHash, g_users[i].role,
                          g_users[i].createdAt);
        }
    }
    status = commitSave(fp, USERS_FILE, tmp);
    return status;
}

/* ------------------------------------------------- interactive functions */

int readNewPassword(char *password, size_t size)
{
    int status = ERR_INVALID_INPUT;
    char again[PASSWORD_SIZE];

    while (status == ERR_INVALID_INPUT) {
        status = readValidated("New password (6-29 characters, no spaces): ", password, size,
                               validatePassword, "Password must be 6-29 characters with no spaces or |.");
        if (status == EMS_OK) {
            status = readLine("Type it again: ", again, sizeof(again));
        }
        if ((status == EMS_OK) && (strcmp(password, again) != 0)) {
            (void)fprintf(getOutputStream(), "The two passwords do not match.\n");
            status = ERR_INVALID_INPUT;
        }
    }
    (void)memset(again, 0, sizeof(again));
    return status;
}

int createFirstAdmin(void)
{
    int status;
    char name[USERNAME_SIZE];
    char password[PASSWORD_SIZE];

    (void)fprintf(getOutputStream(), "No user accounts yet. Create the first Administrator account.\n");
    status = readValidated("Admin username (3-19 letters, digits or _): ", name, sizeof(name),
                           validateUsername, "Use 3-19 letters, digits or _.");
    if (status == EMS_OK) {
        status = readNewPassword(password, sizeof(password));
    }
    if (status == EMS_OK) {
        status = addUserRecord(name, password, ROLE_ADMIN);
    }
    if (status == EMS_OK) {
        (void)saveUsers();
        (void)logMessage(LOG_INFO, "AUTH", "First administrator account created");
        (void)fprintf(getOutputStream(), "Administrator %s created.\n", name);
    }
    (void)memset(password, 0, sizeof(password));
    return status;
}

int login(void)
{
    int status = EMS_OK;
    int attempt;
    int role = ROLE_NONE;
    char name[USERNAME_SIZE];
    char password[PASSWORD_SIZE];
    char msg[LINE_SIZE];

    password[0] = '\0';
    if (g_userCount == 0) {
        status = createFirstAdmin();
    }
    for (attempt = 1; (attempt <= MAX_LOGIN_TRIES) && (status == EMS_OK) && (role == ROLE_NONE); attempt++) {
        (void)fprintf(getOutputStream(), "\n=== Login (attempt %d of %d) ===\n", attempt, MAX_LOGIN_TRIES);
        status = readLine("Username: ", name, sizeof(name));
        if (status == EMS_OK) {
            status = readLine("Password: ", password, sizeof(password));
        }
        if (status == ERR_INVALID_INPUT) {
            status = EMS_OK;        /* too long: counts as a failed attempt */
            name[0] = '\0';
        }
        if (status == EMS_OK) {
            role = verifyCredentials(name, password);
            if (role == ROLE_NONE) {
                (void)fprintf(getOutputStream(), "%s: %s\n", errorCodeText(ERR_LOGIN), errorMessage(ERR_LOGIN));
                (void)snprintf(msg, sizeof(msg), "failed login for '%s' (attempt %d)", name, attempt);
                (void)logError("AUTH", ERR_LOGIN, msg);
            }
        }
    }
    (void)memset(password, 0, sizeof(password));
    if ((status == EMS_OK) && (role != ROLE_NONE)) {
        setSession(g_users[findUser(name)].username, role);
        (void)snprintf(msg, sizeof(msg), "%s logged in as %s", getCurrentUser(), roleName(role));
        (void)logMessage(LOG_INFO, "AUTH", msg);
        (void)fprintf(getOutputStream(), "Welcome, %s (%s).\n", getCurrentUser(), roleName(role));
    } else if (status == EMS_OK) {
        (void)fprintf(getOutputStream(), "Too many failed attempts. Exiting.\n");
        status = ERR_LOGIN;
    } else {
        /* input closed */
    }
    return status;
}

int addUser(void)
{
    int status;
    int role = ROLE_OPERATOR;
    char name[USERNAME_SIZE];
    char password[PASSWORD_SIZE];
    char msg[LINE_SIZE];

    status = readValidated("New username (3-19 letters, digits or _): ", name, sizeof(name),
                           validateUsername, "Use 3-19 letters, digits or _.");
    if ((status == EMS_OK) && (findUser(name) >= 0)) {
        status = ERR_DUPLICATE_ID;
        showError("AUTH", ERR_DUPLICATE_ID);
    }
    if (status == EMS_OK) {
        status = readNewPassword(password, sizeof(password));
    }
    if (status == EMS_OK) {
        status = readInt("Role (1 = Administrator, 2 = HR Operator): ", ROLE_ADMIN, ROLE_OPERATOR, &role);
    }
    if (status == EMS_OK) {
        status = addUserRecord(name, password, role);
        if (status == EMS_OK) {
            (void)saveUsers();
            (void)snprintf(msg, sizeof(msg), "User %s (%s) created by %s", name, roleName(role), getCurrentUser());
            (void)logMessage(LOG_INFO, "AUTH", msg);
            (void)fprintf(getOutputStream(), "User %s created as %s.\n", name, roleName(role));
        } else {
            showError("AUTH", (EmsStatus)status);
        }
    }
    (void)memset(password, 0, sizeof(password));
    return status;
}
