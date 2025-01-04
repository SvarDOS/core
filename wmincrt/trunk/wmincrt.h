/* This file is part of the SvarDOS project and is published under the terms
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

/* FAR pointer into the PSP */
#define PSP_PTR(ofs) MK_FP(crt_psp_seg, ofs)

/* segment (frame) number of our PSP */
extern unsigned short crt_psp_seg;

/* Pointer to command line character array.
   Lives at PSP:80h if .COM is built, otherwise at DGROUP:crt_cmdline.
   As this comes from the PSP command tail it MIGHT NOT BE terminated
   by a 0 or CR! */
extern char *crt_cmdline;

/* Returns to operating system with exitcode. */
void crt_exit(int exitcode);


/* The following is available if WMINCRT is assembled with DEBUG defined: */

/* Checks NULL area for changes, returns 1 if unchanged, 0 otherwise.
   NULL area is 2 bytes for .COM files and 256 bytes for .EXE files, so that
   writes that should have gone to the PSP can be detected.

   The 256 byte .EXE NULL area is filled with INT3 instructions to catch
   jumps and calls via NULL pointer.

   Check is performed by CRT on program termination. May also be called
   by the user. */
int crt_nullarea_check(void);


/* the following is available if WMINCRT is assembled with STACKSTAT defined
   and NOSTACKCHECK UNDEFINED! */

/* remembers lowest SP */
extern unsigned short crt_stack_low;

#endif
