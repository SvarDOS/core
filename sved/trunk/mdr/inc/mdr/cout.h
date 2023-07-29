/*
 * Console output
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

#ifndef MDR_COUT
#define MDR_COUT

/* inits the subsystem, fills arguments with:
 * w = screen width
 * h = screen height
 * Any of these arguments may be passed as NULL
 * Returns a color flag (0=mono, non-zero=color) */
unsigned char mdr_cout_init(unsigned char *w, unsigned char *h);

/* get current attribute value under cursor and returns it */
int mdr_cout_getcurattr(void);

void mdr_cout_close(void);
void mdr_cout_cursor_hide(void);
void mdr_cout_cursor_show(void);

/* gets cursor's position on screen (row, column) and shape */
void mdr_cursor_getinfo(unsigned char *column, unsigned char *row, unsigned short *shape);

void mdr_cout_locate(unsigned char row, unsigned char column);

/* print a single character on screen */
void mdr_cout_char(unsigned char y, unsigned char x, char c, unsigned char attr);

/* print a single character on screen, repeated count times */
void mdr_cout_char_rep(unsigned char y, unsigned char x, char c, unsigned char attr, unsigned char count);

/* print a nul-terminated string on screen, up to maxlen characters
 * returns the number of characters actually displayed */
unsigned char mdr_cout_str(unsigned char y, unsigned char x, const char *s, unsigned char attr, unsigned char maxlen);

/* clears screen, filling it with a single color attribute */
void mdr_cout_cls(unsigned char colattr);

void mdr_cout_getconprops(unsigned char *termwidth, unsigned char *termheight, unsigned char *colorflag);

/* functions below do not need mdr_cout_init() initialization, they may be
 * used to display characters or nul-terminated strings to console right away
 * as they use DOS services */
void mdr_coutraw_char(char c);
void mdr_coutraw_puts(const char *s);

#endif
