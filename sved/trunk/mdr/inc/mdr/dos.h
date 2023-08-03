/*
 * Functions interacting with DOS
 *
 * This file is part of the Mateusz' DOS Routines (MDR): http://mdr.osdn.io
 * Published under the terms of the MIT License, as stated below.
 *
 * Copyright (C) 2014-2023 Mateusz Viste
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

/* waits for a keypress and returns it
 * extended keys are returned ORed with 0x100 (example: PGUP is 0x149) */
int mdr_dos_getkey(void);

/* Same as mdr_dos_getkey(), but this call cannot be aborted by CTRL+C */
int mdr_dos_getkey2(void);

/* flush the keyboard buffer */
void mdr_dos_flushkeyb(void);

/* poll stdin status, returns 0 if no character is pending in the keyboard
 * buffer, non-zero otherwise */
int mdr_dos_keypending(void);

/* sets up the CTRL+C handler for the running program to a no-op - in other
 * words, after this call DOS will no longer abort the program on CTRL+C.
 * this is only valid for the duration of the program because DOS will restore
 * the original handler after the program exits.
 *
 * an alternative is mdr_dos_ctrlc_off(), but this does not inhibit the
 * handler, it sets DOS to not react to CTRL+C in the first place, and this
 * setting stays active after the program quits so the program should remember
 * to restore the original setting before quitting. */
void mdr_dos_ctrlc_inhibit(void);

/* sets the DOS BREAK control OFF, ie. instructs DOS not to check for CTRL+C
 * during most input operations. returns the previous state of the break
 * control flag (0=disabled 1=enabled). this changes a global DOS flag that can
 * be checked on command line with the "BREAK" command, so the program should
 * take care to restore the initial setting before quitting. */
unsigned char mdr_dos_ctrlc_disable(void);

/* sets the DOS BREAK control ON. see mdr_dos_ctrlc_disable() for details. */
void mdr_dos_ctrlc_enable(void);

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
