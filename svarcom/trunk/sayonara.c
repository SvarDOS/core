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

#include <i86.h>

#include "rmodinit.h"

#include "sayonara.h"


/* rewires my parent pointer, uninstalls rmod let DOS terminate me, UNLESS
 * my parent is unknown */
void sayonara(struct rmod_props far *rmod) {
  unsigned short rmodseg = rmod->rmodseg;
  unsigned long far *orgparent = MK_FP(rmodseg, RMOD_OFFSET_ORIGPARENT);
  unsigned long *myparent = (void *)0x0A;
  unsigned short far *rmodenv_ptr = MK_FP(rmodseg, RMOD_OFFSET_ENVSEG);
  unsigned short rmodenv = *rmodenv_ptr;

  /* detect "I am the origin shell" situations */
  if (*orgparent == 0xffff) return; /* original parent set to 0xffff (DOS-C / FreeDOS) */
  if (rmod->flags & FLAG_PERMANENT) return; /* COMMAND.COM /P */

  /* set my parent back to original value */
  *myparent = *orgparent;

  _asm {
    /* free RMOD's code segment and env segment */
    mov ah, 0x49   /* DOS 2+ -- Free Memory Block */
    mov es, [rmodseg]
    int 0x21

    /* free RMOD's env segment */
    mov ah, 0x49   /* DOS 2+ -- Free Memory Block */
    mov es, [rmodenv]
    int 0x21

    /* gameover */
    mov ax, 0x4C00 /* DOS 2+ -- Terminate with exit code 0 */
    int 0x21
  }
}
