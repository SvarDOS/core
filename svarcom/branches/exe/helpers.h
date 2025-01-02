/* This file is part of the SvarCOM project and is published under the terms
 * of the MIT license.
 *
 * Copyright (C) 2021-2024 Mateusz Viste
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

#ifndef HELPERS_H
#define HELPERS_H

#include "crt.h"

#ifndef NULL
#define NULL (0)
#endif

/* sets y, m, d to current (DOS) date */
void dos_get_date(unsigned short *y, unsigned char *m, unsigned char *d);

/* sets h, m, s to current (DOS) time */
void dos_get_time(unsigned char *h, unsigned char *m, unsigned char *s);

/* case-insensitive comparison of strings, compares up to maxlen characters.
 * returns non-zero on equality. */
int imatchlim(const char *s1, const char *s2, unsigned short maxlen);

#define imatch(a,b) imatchlim(a,b,0xffff)

/* returns zero if s1 starts with s2 */
int strstartswith(const char *s1, const char *s2);

/* outputs a NULL-terminated string to handle (hSTDOUT or hSTDERR) */
void output_internal(const char *s, unsigned char nl, unsigned char handle);

/* outputs a NULL-terminated NLS string to stdout */
void nls_output_internal(unsigned short id, unsigned char nl, unsigned char handle);

#define hSTDOUT 1
#define hSTDERR 2

#define output(x) output_internal(x, 0, hSTDOUT)
#define outputnl(x) output_internal(x, 1, hSTDOUT)
#define nls_output(x,y) nls_output_internal((x << 8) | y, 0, hSTDOUT)
#define nls_outputnl(x,y) nls_output_internal((x << 8) | y, 1, hSTDOUT)
#define nls_output_err(x,y) nls_output_internal((x << 8) | y, 0, hSTDERR)
#define nls_outputnl_err(x,y) nls_output_internal((x << 8) | y, 1, hSTDERR)

/* output DOS error e to stderr, terminated with a CR/LF */
void nls_outputnl_doserr(unsigned short e);


/* this is also known as the "Country Info Block" or "CountryInfoRec":
 * offset size desc
 *   +0      2   wDateFormat  0=USA (m d y), 1=Europe (d m y), 2=Japan (y m d)
 *   +2      5  szCrncySymb  currency symbol (ASCIIZ)
 *   +7      2  szThouSep    thousands separator (ASCIIZ)
 *   +9      2  szDecSep     decimal separator (ASCIIZ)
 * +0bH      2  szDateSep    date separator (ASCIIZ)
 * +0dH      2  szTimeSep    time separator (ASCIIZ)
 * +0fH      1  bCrncyFlags  currency format flags
 * +10H      1  bCrncyDigits decimals digits in currency
 * +11H      1  bTimeFormat  time format 0=12h 1=24h
 * +12H      4  pfCasemap    Casemap FAR call address
 * +16H      2  szDataSep    data list separator (ASCIIZ)
 * +18H     10  res          reserved zeros
 *          34               total length
 */
_Packed struct nls_patterns {
  unsigned short dateformat;
  char currency[5];
  char thousep[2];
  char decsep[2];
  char datesep[2];
  char timesep[2];
  unsigned char currflags;
  unsigned char currdigits;
  unsigned char timefmt;
  void far *casemapfn;
  char datalistsep[2];
  char reserved[10];
};


#define DOS_ATTR_RO   1
#define DOS_ATTR_HID  2
#define DOS_ATTR_SYS  4
#define DOS_ATTR_VOL  8
#define DOS_ATTR_DIR 16
#define DOS_ATTR_ARC 32

/* find first matching files using a FindFirst DOS call
 * attr contains DOS attributes that files MAY have (ie attr=0 will match only
 * files that have no attributes at all)
 * returns 0 on success or a DOS err code on failure */
unsigned short findfirst(struct DTA *dta, const char *pattern, unsigned short attr);

/* find next matching, ie. continues an action intiated by findfirst() */
unsigned short findnext(struct DTA *dta);

/* print s string and wait for a single key press from stdin. accepts only
 * key presses defined in the c ASCIIZ string. returns offset of pressed key
 * in string. keys in c MUST BE UPPERCASE! */
unsigned short askchoice(const char *s, const char *c);

/* converts a path to its canonic representation, returns 0 on success
 * or DOS err on failure (invalid drive) */
unsigned short file_truename(const char *src, char *dst);

/* returns DOS attributes of file, or -1 on error */
int file_getattr(const char *fname);

/* returns screen's width (in columns) */
unsigned short screen_getwidth(void);

/* returns screen's height (in rows) */
unsigned short screen_getheight(void);

/* displays the "Press any key to continue" msg and waits for a keypress */
void press_any_key(void);

/* validate a drive (A=0, B=1, etc). returns 1 if valid, 0 otherwise */
int isdrivevalid(unsigned char drv);

/* converts a filename into FCB format (FILENAMEEXT) */
void file_fname2fcb(char *dst, const char *src);

/* converts a FCB filename (FILENAMEEXT) into normal format (FILENAME.EXT) */
void file_fcb2fname(char *dst, const char *src);

/* converts an unsigned short to a four-byte ASCIZ hex string ("0ABC") */
void ustoh(char *dst, unsigned short n);

/* converts an ASCIIZ string into an unsigned short. returns 0 on success.
 * on error, result will contain all valid digits that were read until
 * error occurred (0 on overflow or if parsing failed immediately) */
int atous(unsigned short *r, const char *s);

/* convert an unsigned short to ASCIZ, output expanded to minlen chars
 * (prefixed with prefixchar if value too small, else truncated)
 * returns length of produced string */
unsigned short ustoa(char *dst, unsigned short n, unsigned char minlen, char prefixchar);

/* appends a backslash if path is a directory
 * returns the (possibly updated) length of path */
unsigned short path_appendbkslash_if_dir(char *path);

/* get current path drive d (A=1, B=2, etc - 0 is "current drive")
 * returns 0 on success, doserr otherwise */
unsigned short curpathfordrv(char *buff, unsigned char d);

/* like strcpy() but returns the string's length */
unsigned short sv_strcpy(char *dst, const char *s);

/* like sv_strcpy() but operates on far pointers */
unsigned short sv_strcpy_far(char far *dst, const char far *s);

/* like strcat() */
void sv_strcat(char *dst, const char *s);

/* like strcat() but operates on far pointers */
void sv_strcat_far(char far *dst, const char far *s);

/* like strlen() */
unsigned short sv_strlen(const char *s);

/* like len() but operates on far pointers */
unsigned short sv_strlen_far(const char far *s);

/* fills a nls_patterns struct with current NLS patterns, returns 0 on success, DOS errcode otherwise */
unsigned short nls_getpatterns(struct nls_patterns *p);

/* computes a formatted date based on NLS patterns found in p
 * returns length of result */
unsigned short nls_format_date(char *s, unsigned short yr, unsigned char mo, unsigned char dy, const struct nls_patterns *p);

/* computes a formatted time based on NLS patterns found in p, sc are ignored if set 0xff
 * returns length of result */
unsigned short nls_format_time(char *s, unsigned char ho, unsigned char mn, unsigned char sc, const struct nls_patterns *p);

/* computes a formatted integer number based on NLS patterns found in p
 * returns length of result */
unsigned short nls_format_number(char *s, unsigned long num, const struct nls_patterns *p);

/* capitalize an ASCIZ string following country-dependent rules */
void nls_strtoup(char *buff);

/* reload nls ressources from svarcom.lng into langblock */
void nls_langreload(char *buff, struct rmod_props far *rmod);

/* locates executable fname in path and fill res with result. returns 0 on success,
 * -1 on failed match and -2 on failed match + "don't even try with other paths"
 * extptr is filled with a ptr to the extension in fname (NULL if no extension) */
int lookup_cmd(char *res, const char *fname, const char *path, const char **extptr);

/* fills fname with the path and filename to the linkfile related to the
 * executable link "linkname". returns 0 on success. */
int link_computefname(char *fname, const char *linkname, unsigned short env_seg);

/* like memcpy() but guarantees to copy from left to right */
void memcpy_ltr(void *d, const void *s, unsigned short len);

/* like memcpy_ltr() but operates on far pointers */
void memcpy_ltr_far(void far *d, const void far *s, unsigned short len);

/* like memcpy() but guarantees to copy from right to left */
void memcpy_rtl(void *d, const void *s, unsigned short len);

/* like bzero(), but accepts far pointers */
void sv_bzero(void far *dst, unsigned short len);

/* like memset() */
void sv_memset(void *dst, unsigned char c, unsigned short len);

/* replaces characters a by b in s */
void sv_strtr(char *s, char a, char b);

/* inserts string s2 into s1 in place of the first % character */
void sv_insert_str_in_str(char *s1, const char *s2);

/* allocates a block of paras paragraphs, as high as possible
 * returns segment of allocated block on success, 0 on failure */
unsigned short alloc_high_seg(unsigned short paras);

/* free previously allocated memory block at segment segm */
void freeseg(unsigned short segm);

#endif
