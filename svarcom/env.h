/*
 * routines used to manipulate the environment block
 * Copyright (C) 2021, Mateusz Viste
 */

#ifndef ENV_H
#define ENV_H

/* looks for varname in environment block and returns a far ptr to it if
 * found, NULL otherwise. varname MUST be in upper-case and MUST be terminated
 * by either a = sign or a NULL terminator */
char far *env_lookup(unsigned short env_seg, const char *varname);

/* returns the size, in bytes, of the allocated environment block */
unsigned short env_allocsz(unsigned short env_seg);

/* remove a variable from environment, if present. returns 0 on success, non-zero if variable not found */
int env_dropvar(unsigned short env_seg, const char *varname);

#define ENV_SUCCESS  0
#define ENV_NOTENOM -1
#define ENV_INVSYNT -2

/* Writes a variable to environment block. The variable must in the form
 * "varname=value". If variable is already present in the environment block,
 * then the value will be updated. If the new value is empty, then the
 * existing variable (if any) is removed.
 *
 * This function returns:
 *   ENV_SUCCESS = success
 *   ENV_NOTENOM = not enough available space in memory block
 *   ENV_INVSYNT = invalid syntax
 */
int env_setvar(unsigned short env_seg, const char *v);

#endif
