/**
 * @file role.h
 * @brief Role Module: fixed array of roles.
 */
#ifndef EMS_ROLE_H
#define EMS_ROLE_H

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Clears all roles (used at start-up, cleanup and in tests). */
void roleReset(void);
/** createdAt = NULL stamps the current time (files pass the saved time).
 *  Adds a role. Returns EMS_OK, ERR_INVALID_INPUT, ERR_DUPLICATE_ID or ERR_LIMIT_REACHED. */
int roleAdd(int id, const char *title, const char *createdAt);
/** Returns the array index of the role, or -1. */
int findRole(int id);
/** Returns the array index of the role with this title (any case), or -1. */
int findRoleByTitle(const char *title);
/** Returns the role title, or "(none)". */
const char *getRoleTitle(int id);
int getRoleCount(void);
/** Returns the role at index, or NULL. */
const Role *getRoleAt(int index);
/** Prints all roles with their employee counts. Returns the number printed. */
int viewRoles(void);
/** Interactive: asks for ID and name, adds and saves. Returns a status code. */
int createRole(void);
/** Interactive role sub-menu (Admin only, checked by caller). */
void roleMenu(void);

#ifdef __cplusplus
}
#endif

#endif /* EMS_ROLE_H */
