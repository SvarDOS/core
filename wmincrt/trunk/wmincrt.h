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

/* crt_argc and crt_argv contain the values handed over as argc and argv
   to main(). These are valid for the whole runtime. crt_argv lives at the
   bottom of the stack. May not be used if NOARGV is given on assembly. */
extern int crt_argc;
extern char **crt_argv;

/* Pointer to command line character array.
   Contains the command line arguments separated by one or more zeroes.
   crt_argv is a pointer array into it.

   If NOARGV is given on assembly, this instead contains the unprocessed
   command line tail which is part of the PSP! */
extern char *crt_cmdline;

/* Stack memory allocation routines, useful if some dynamic memory is
   required locally. */
/* crt_stk_alloc reserves memory from the stack (from the bottom) */
void *crt_stk_mem_alloc(unsigned short size);
/* crt_stk_dispose resets stack allocation pointer to bottom,
   freeing all allocated stack memory above. */
void *crt_stk_mem_rewind(void *bottom);

/* Returns to operating system with exitcode. */
void crt_exit(int exitcode);

/* The following is available if WMINCRT is assembled with DEBUG defined: */

/* Checks NULL area for changes, returns 1 if unchanged, 0 otherwise.
   NULL area is 2 bytes for .COM files and 256 bytes for .EXE files, so that
   writes that should have gone to the PSP can be detected.

   The 256 byte .EXE NULL area is filled with INT3 instructions to catch
   jumps and calls to that region.

   Check is performed by CRT on program termination. May also be called
   by the user. */
int crt_nullarea_check(void);


/* the following is available if WMINCRT is assembled with STACKSTAT defined
   and NOSTACKCHECK UNDEFINED! */

/* remembers lowest SP */
extern unsigned short crt_stack_low;

#endif
