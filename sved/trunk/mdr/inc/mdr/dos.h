/*
 * Functions interacting with DOS
 *
 * This file is part of the Mateusz' DOS Routines (MDR): http://mdr.osdn.io
 * Published under the terms of the MIT License, as stated below.
 *
 * Copyright (C) 2014-2022 Mateusz Viste
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#ifndef MDR_DOS_H
#define MDR_DOS_H

#include <time.h> /* time_t */

/* returns a far pointer to the current process PSP structure */
void far *mdr_dos_psp(void);

/* returns a far pointer to the environment block of the current process */
char far *mdr_dos_env(void);

/* looks for varname in the DOS environment and fills result with its value if
 * found. returns NULL if not found or if value is too big to fit in result
 * (reslimit exceeded). returns result on success.
 * NOTE: this function performs case-sensitive matches */
char *mdr_dos_getenv(char *result, const char *varname, unsigned short reslimit);

/* fetches directory where the program was loaded from and return its length.
 * path string is never longer than 128 (incl. the null terminator) and it is
 * always terminated with a backslash separator, unless it is an empty string */
unsigned char mdr_dos_exepath(char *path);

/* returns a far pointer to the full path and filename of the running program.
 * returns NULL on error. */
const char far *mdr_dos_selfexe(void);

/* converts a "DOS format" 16-bit packed date into a standard (time_t)
 * unix timestamp. A DOS date is a 16-bit value:
 * YYYYYYYM MMMDDDDD
 *
 * day of month is always within 1-31 range;
 * month is always within 1-12 range;
 * year starts at 1980 and continues for 127 years */
time_t mdr_dos_date2unix(unsigned short d);

/* converts a "DOS format" 16-bit packed time into hours, minutes and seconds
 *
 * A DOS time is a 16-bit value:
 * HHHHHMMM MMMSSSSS
 *
 * HHHHH  = hours, always within 0-23 range
 * MMMMMM = minutes, always within 0-59 range
 * SSSSS  = seconds/2 (always within 0-29 range) */
void mdr_dos_time2hms(unsigned char *h, unsigned char *m, unsigned char *s, unsigned short t);

#endif