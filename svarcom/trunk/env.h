/* This file is part of the SvarCOM project and is published under the terms
 * of the MIT license.
 *
 * Copyright (C) 2021-2022 Mateusz Viste
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/*
 * routines used to manipulate the environment block
 */

#ifndef ENV_H
#define ENV_H

/* looks for varname in environment block and returns a far ptr to it if
 * found, NULL otherwise. varname MUST be in upper-case and MUST be terminated
 * by either a = sign or a NULL terminator */
char far *env_lookup(unsigned short env_seg, const char *varname);

/* almost identical to env_lookup(), but instead of returning a pointer
 * to the 'NAME=value' string, it returns a pointer to value (or NULL if
 * var not found) */
char far *env_lookup_val(unsigned short env_seg, const char *varname);

/* locates the value of env variable varname and copies it to result, up to
 * ressz bytes (incl. the NULL terminator). returns the length of the value on
 * success, 0 if var not found or couldn't fit in ressz). */
unsigned short env_lookup_valcopy(char *res, unsigned short ressz, unsigned short env_seg, const char *varname);

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
