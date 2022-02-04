/* This file is part of the svarlang project and is published under the terms
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

#ifndef SVARLANG_H
#define SVARLANG_H

/* loads translations for program PROGNAME, language LANG, in the path NLSPATH.
 * returns 0 on success. */
int svarlang_load(const char *progname, const char *lang, const char *nlspath);

/* same as svarlang_load(), but relies on getenv() to pull LANG and NLSPATH. */
int svarlang_autoload(const char *progname);

/* Returns a pointer to the string "id". Does not require svalang_load() to be
 * executed, but then it will only return the reference language strings.
 * a string id is the concatenation of the CATS-style identifiers, for example
 * string 1,0 becomes 0x0100, string 2.10 is 0x020A, etc.
 * It NEVER returns NULL, if id not found then an empty string is returned */
const char *svarlang_strid(unsigned short id);

/* a convenience definition to fetch strings by their CATS-style pairs instead
 * of the 16-bit id. */
#define svarlang_str(x, y) svarlang_strid((x << 8) | y)

#endif
