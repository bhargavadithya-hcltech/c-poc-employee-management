/**
 * @file auth.h
 * @brief Session & Auth Module: users, login and role-based access control.
 *
 * Passwords are never stored. users.txt keeps username|hash|role.
 * The hash is a salted djb2 (username is the salt). It is fine for a
 * learning POC; a real product would use bcrypt or argon2.
 */
#ifndef EMS_AUTH_H
#define EMS_AUTH_H

#include "common.h"

void authReset(void);
/** 32-bit salted djb2 hash of "lower(username):password". */
unsigned long hashPassword(const char *username, const char *password);
/** Returns the index of the user (case-insensitive), or -1. */
int findUser(const char *username);
/** Adds a user with a plain password (hashed here).
 *  Returns EMS_OK, ERR_INVALID_INPUT, ERR_DUPLICATE_ID or ERR_LIMIT_REACHED. */
int addUserRecord(const char *username, const char *password, int role);
/** Adds a user whose hash is already known (used when loading users.txt). */
int addUserHashed(const char *username, unsigned long hash, int role, const char *createdAt);
/** Returns the user's role when the password is right, otherwise ROLE_NONE. */
int verifyCredentials(const char *username, const char *password);
void setSession(const char *username, int role);
void logout(void);
int getCurrentRole(void);
/** Logged-in username, or "-" when nobody is logged in. */
const char *getCurrentUser(void);
/** "Administrator", "HR Operator" or "Unknown". */
const char *roleName(int role);
int getUserCount(void);
const User *getUserAt(int index);
/** 1 when the logged-in user may do an operation that needs requiredRole.
 *  Prints and logs E010 when access is denied. */
int checkAccess(int requiredRole);
/** Reads users.txt from the data folder. Missing file = no users yet. */
int loadUsers(void);
int saveUsers(void);

/* ---- interactive functions ---- */
/** Reads a new password twice until both match and it is valid. */
int readNewPassword(char *password, size_t size);
/** First run only: asks for the first Administrator account. */
int createFirstAdmin(void);
/** Up to MAX_LOGIN_TRIES attempts. Returns EMS_OK, ERR_LOGIN or ERR_END_OF_INPUT. */
int login(void);
/** Admin only (checked by caller): adds an Administrator or HR Operator account. */
int addUser(void);

#endif /* EMS_AUTH_H */
