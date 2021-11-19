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
#include <string.h>

#include "env.h"
#include "helpers.h"

#include "rmodinit.h"


/* returns far pointer to rmod's settings block on success */
struct rmod_props far *rmod_install(unsigned short envsize, unsigned char *rmodcore, unsigned short rmodcore_len) {
  char far *myptr, far *mcb;
  unsigned short far *owner;
  const unsigned short sizeof_rmodandprops_paras = (0x100 + rmodcore_len + sizeof(struct rmod_props) + 15) / 16;
  unsigned short rmodseg = 0xffff;
  unsigned short envseg, origenvseg;
  struct rmod_props far *res;

  /* read my current env segment from PSP and save it */
  envseg = *((unsigned short *)0x2c);
  origenvseg = envseg;

  /* printf("original (PSP) env buffer at %04X\r\n", envseg); */

  /* if envseg is zero, then enforce our own one (MSDOS 5 does not provide a default env) */
  if ((envseg == 0) && (envsize == 0)) envsize = 256;

  /* if custom envsize requested, convert it to number of paragraphs */
  if (envsize != 0) {
    envsize += 15;
    envsize /= 16;
  }

  _asm {
    /* link in the UMB memory chain for enabling high-memory allocation (and save initial status on stack) */
    mov ax, 0x5802  /* GET UMB LINK STATE */
    int 0x21
    xor ah, ah
    push ax         /* save link state on stack */
    mov ax, 0x5803  /* SET UMB LINK STATE */
    mov bx, 1
    int 0x21
    /* get current allocation strategy and save it in DX */
    mov ax, 0x5800
    int 0x21
    push ax
    pop dx
    /* set strategy to 'last fit, try high then low memory' */
    mov ax, 0x5801
    mov bx, 0x0082
    int 0x21
    /* ask for a memory block and save the given segment to rmodseg */
    mov ah, 0x48
    mov bx, sizeof_rmodandprops_paras
    int 0x21
    jc ALLOC_FAIL
    mov rmodseg, ax
    /* ask for a memory block for the environment and save it to envseg (only if custom size requested) */
    mov bx, envsize
    test bx, bx
    jz ALLOC_FAIL
    mov ah, 0x48
    int 0x21
    jc ALLOC_FAIL
    mov envseg, ax

    ALLOC_FAIL:
    /* restore initial allocation strategy */
    mov ax, 0x5801
    mov bx, dx
    int 0x21
    /* restore initial UMB memory link state */
    mov ax, 0x5803
    pop bx       /* pop initial UMB link state from stack */
    int 0x21
  }

  if (rmodseg == 0xffff) {
    outputnl("malloc error");
    return(NULL);
  }

  /* copy rmod to its destination, prefixed with a copy of my own PSP */
  myptr = MK_FP(rmodseg, 0);
  {
    unsigned short i;
    char *mypsp = (void *)0;
    for (i = 0; i < 0x100; i++) myptr[i] = mypsp[i];
  }
  myptr = MK_FP(rmodseg, 0x100);
  _fmemcpy(myptr, rmodcore, rmodcore_len);

  /* mark rmod memory as "self owned" */
  mcb = MK_FP(rmodseg - 1, 0);
  owner = (void far *)(mcb + 1);
  *owner = rmodseg;
  _fmemcpy(mcb + 8, "SVARCOM", 8);

  /* mark env memory as "owned by rmod" */
  mcb = MK_FP(envseg - 1, 0);
  owner = (void far *)(mcb + 1);
  *owner = rmodseg;
  _fmemcpy(mcb + 8, "SVARENV", 8);

  /* if env block is newly allocated, fill it with a few NULLs */
  if (envsize != 0) {
    owner = MK_FP(envseg, 0);
    owner[0] = 0;
    owner[1] = 0;
  }

  /* prepare result (rmod props) */
  res = MK_FP(rmodseg, 0x100 + rmodcore_len);
  _fmemset(res, 0, sizeof(*res));  /* zero out */
  res->rmodseg = rmodseg;          /* rmod segment */
  res->inputbuf[0] = 128;          /* input buffer for INT 0x21, AH=0Ah*/
  res->origenvseg = origenvseg;    /* original environment segment */

  /* write env segment to rmod's PSP */
  owner = MK_FP(rmodseg, RMOD_OFFSET_ENVSEG);
  *owner = envseg;

  /* write boot drive to rmod bootdrive field */
  _asm {
    push ax
    push bx
    push dx
    push ds
    mov ax, 0x3305 /* DOS 4.0+ - GET BOOT DRIVE */
    int 0x21 /* boot drive is in DL now (1=A:, 2=B:, etc) */
    add dl, 'A'-1 /* convert to a proper ASCII letter */
    /* set DS to rmodseg */
    mov ax, rmodseg
    mov ds, ax
    /* write boot drive to rmod bootdrive field */
    mov bx, RMOD_OFFSET_BOOTDRIVE
    mov [bx], dl
    pop ds
    pop dx
    pop bx
    pop ax
  }

  /* save my original parent in rmod's memory */
  res->origparent = *((unsigned long *)0x0a); /* original parent seg:off is at 0x0a of my PSP */

  /* set the int22 handler in my PSP to rmod so DOS jumps to rmod after I
   * terminate and save the original handler in rmod's memory */
  {
    unsigned short *ptr = (void *)0x0a; /* int22 handler is at 0x0A of the PSP */
    ptr[0] = RMOD_OFFSET_ROUTINE;
    ptr[1] = rmodseg;
  }

  return(res);
}


/* look up my parent: if it's rmod then return a ptr to its props struct,
 * otherwise return NULL */
struct rmod_props far *rmod_find(unsigned short rmodcore_len) {
  unsigned short *parent = (void *)0x0C; /* parent's seg in PSP[Ch] ("prev. int22 handler") */
  unsigned short far *ptr;
  const unsigned short sig[] = {0x1983, 0x1985, 0x2017, 0x2019};
  unsigned char *cmdtail = (void *)0x80;
  unsigned char i;
  /* is it rmod? */
  ptr = MK_FP(*parent, 0x100);
  for (i = 0; i < 4; i++) if (ptr[i] != sig[i]) return(NULL);
  /* match successfull (rmod is my parent) - but is it really a respawn?
   * command-line tail should contain a single character '\r' */
  if ((cmdtail[0] != 1) || (cmdtail[1] != '\n')) return(NULL);
  cmdtail[0] = 0;
  cmdtail[1] = '\r';
  /* */
  return(MK_FP(*parent, 0x100 + rmodcore_len));
}


/* update rmod's pointer to comspec */
void rmod_updatecomspecptr(unsigned short rmod_seg, unsigned short env_seg) {
  unsigned short far *comspecptr = MK_FP(rmod_seg, RMOD_OFFSET_COMSPECPTR);
  char far *comspecfp = env_lookup_val(env_seg, "COMSPEC");
  if (comspecfp != NULL) {
    *comspecptr = FP_OFF(comspecfp);
  } else {
    *comspecptr = 0;
  }
}
