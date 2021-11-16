/* This file is part of the SvarCOM project and is published under the terms
 * of the MIT license.
 *
 * Copyright (C) 2021 Mateusz Viste
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

#ifndef RMODINIT_H
#define RMODINIT_H

#define FLAG_EXEC_AND_QUIT 1
#define FLAG_PERMANENT 2

struct rmod_props {
  char inputbuf[130];         /* input buffer for INT 21, AH=0x0A */
  unsigned short rmodseg;     /* segment where rmod is loaded */
  unsigned long origparent;   /* original parent (far ptr) of the shell */
  unsigned short origenvseg;  /* original environment segment */
  unsigned char flags;        /* command line parameters */
  unsigned char echoflag;     /* ECHO ON / ECHO OFF */
};

#define RMOD_OFFSET_ENVSEG     0x2C   /* stored in rmod's PSP */
#define RMOD_OFFSET_LEXITCODE  0x10A
#define RMOD_OFFSET_COMSPECPTR 0x10C
#define RMOD_OFFSET_BOOTDRIVE  0x10E
#define RMOD_OFFSET_ROUTINE    0x11D

struct rmod_props far *rmod_install(unsigned short envsize);
struct rmod_props far *rmod_find(void);
void rmod_updatecomspecptr(unsigned short rmod_seg, unsigned short env_seg);

#endif
