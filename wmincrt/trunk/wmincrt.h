/* This file is part of the SvarCOM project and is published under the terms
 * of the MIT license.
 *
 * Copyright (C) 2021-2024 Bernd Boeckmann, Mateusz Viste
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

#ifndef WMINCRT_H
#define WMINCRT_H

#ifndef MK_FP
#  define MK_FP(seg, ofs)(void __far *)(((unsigned long)(seg) << 16) + (ofs))
#endif

#define PSP_PTR(ofs) MK_FP(crt_psp_seg, ofs)

/* segment (frame) number of our PSP */
extern unsigned short crt_psp_seg;

/* pointer to command line character array (COM: PSP:80h, EXE: _DATA:crt_cmdline
   As this comes from the PSP command tail it MIGHT NOT BE terminated by a 0 or CR! */
extern char *crt_cmdline;

void crt_exit(int exitcode);

#endif
