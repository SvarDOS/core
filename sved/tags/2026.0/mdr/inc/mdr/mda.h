/*
 * MDA output that can be used in parallel of some other video calls in a
 * dual-screen setup.
 *
 * This file is part of Mateusz' DOS Routines <http://mateusz.fr/mdr>
 * Published under the terms of the MIT License, as stated below.
 *
 * Copyright (C) 2014-2025 Mateusz Viste
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

/*
 * MDA is always 80x25 monochrome
 *
 * This can be used in two ways:
 * 1. use mdr_mda_write() to position strings on the screen yourself,
 * or
 * 2. mdr_mda_log() to write strings one under another with an automatic
 * screen scroll once the bottom line is reached.
 */

#ifndef MDR_MDA_H
#define MDR_MDA_H

/* clears the MDA screen */
void mdr_mda_cls(void);

/* write a 0-terminated string on MDA's screen at [x,y] position
 * NOTE: [0,0] is the top left corner */
void mdr_mda_write(unsigned char x, unsigned char y, const char *s);

/* write a "log" message to the MDA screen, line after line. scrolls screen
 * automatically when bottom row is reached. */
void mdr_mda_log(const char *s);


#endif
