/* This file is part of the SvarCOM project and is published under the terms
 * of the MIT license.
 *
 * Copyright (C) 2021-2025 Mateusz Viste
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

#include "env.h"
#include "helpers.h"
#include "wmincrt/wmincrt.h"

#include "rmodinit.h"


/* returns far pointer to rmod's settings block on success */
struct rmod_props far *rmod_install(unsigned short envsize, unsigned char *rmodcore, unsigned short rmodcore_len, unsigned char *cfgflags) {
  char far *myptr, far *mcb;
  unsigned short far *owner;
  const unsigned short sizeof_rmodandprops_paras = (0x100 + rmodcore_len + sizeof(struct rmod_props) + 15) / 16;
  unsigned short rmodseg = 0xffff;
  unsigned short envseg, origenvseg;
  struct rmod_props far *res;

  /* read my current env segment from PSP and save it */
  envseg = *((unsigned short far *)PSP_PTR(0x2c));
  origenvseg = envseg;

  /* printf("original (PSP) env buffer at %04X\r\n", envseg); */

  /* if my environment seg was zeroed, then I am the init process (under DR-DOS and MS-DOS 5 at least) */
  if (envseg == 0) {
    *cfgflags |= FLAG_PERMANENT; /* imply /P so AUTOEXEC.BAT is executed */

    /* make sure to enforce our own env (MSDOS 5 does not provide a default env) */
    if (envsize == 0) envsize = 256;
  }

  /* if custom envsize requested, convert it to number of paragraphs */
  if (envsize != 0) {
    envsize += 15;
    envsize /= 16;
  }

  /* ask for a memory block and save the given segment to rmodseg */
  rmodseg = alloc_high_seg(sizeof_rmodandprops_paras);
  if (rmodseg == 0) {
    outputnl("malloc error");
    return(NULL);
  }

  /* ask for a memory block for the environment and save it to envseg (only if custom size requested) */
  if (envsize != 0) {
    envseg = alloc_high_seg(envsize);
  }

  /* generate a new PSP where RMOD is about to land */
  _asm {
    push dx
    mov ah, 0x26 /* CREATE NEW PROGRAM SEGMENT PREFIX (DOS 1+) */
    mov dx, rmodseg
    int 0x21
    pop dx
  }

  myptr = MK_FP(rmodseg, 0);

  /* patch up RMOD's PSP: Parent's PSP segment @ 0x16-0x17 */
  myptr[0x16] = rmodseg & 0xff; /* RMOD is his own parent */
  myptr[0x17] = rmodseg >> 8;

  /* patch up RMOD's PSP: SS:SP pointer @ 0x2E-0x31  --  I abuse the PSP's
   * command line tail as stack, but I do NOT set the stack at the end of the
   * tail. E. C. Masloch kindly explained why this would be a bad idea:
   *
   * "This is wrong and will potentially overwrite part of your buffers that
   * start past the PSP. This is because the dword [PSP:2Eh] is not used merely
   * to set SS:SP but rather to find the stack frame created by the int 21h
   * call. Therefore the int 21h call that terminates the child process will
   * then pop a number of registers off starting from the address stored in the
   * PSP." <https://github.com/SvarDOS/bugz/issues/38#issuecomment-1817445740>
   */
  myptr[0x2e] = 192; /* middle of the command line tail area so I have 64 bytes */
  myptr[0x2f] = 0;   /* before and 64 bytes in front of me */
  myptr[0x30] = rmodseg & 0xff;
  myptr[0x31] = rmodseg >> 8;

  /* patch up RMOD's PSP: JFT size @ 0x32-0x33 */
  myptr[0x32] = 20; /* default JFT size (max that fits without an extra allocation) */
  myptr[0x33] = 0;

  /* patch up RMOD's PSP: JFT pointer @ 0x34-0x37 */
  myptr[0x34] = 0x18; /* the JFT is in the PSP itself */
  myptr[0x35] = 0;
  myptr[0x36] = rmodseg & 0xff;
  myptr[0x37] = rmodseg >> 8;

  /* patch up RMOD's PSP: pointer to previous PSP @ 0x38-0x3B */
  myptr[0x38] = 0;
  myptr[0x39] = 0;
  myptr[0x3A] = rmodseg & 0xff;
  myptr[0x3B] = rmodseg >> 8;

  /* copy rmod to its destination (right past the PSP I prepared) */
  myptr = MK_FP(rmodseg, 0x100);
  memcpy_ltr_far(myptr, rmodcore, rmodcore_len);

  /* mark rmod memory (MCB) as "self owned" */
  mcb = MK_FP(rmodseg - 1, 0);
  owner = (void far *)(mcb + 1);
  *owner = rmodseg;
  memcpy_ltr_far(mcb + 8, "SVARCOM", 8);

  /* mark env memory (MCB) as "owned by rmod" */
  mcb = MK_FP(envseg - 1, 0);
  owner = (void far *)(mcb + 1);
  *owner = rmodseg;
  memcpy_ltr_far(mcb + 8, "SVARENV", 8);

  /* if env block is newly allocated, then:
   *  if an original env is present then copy it
   *  otherwise fill the new env with a few NULs */
  if (envsize != 0) {
    owner = MK_FP(envseg, 0);
    owner[0] = 0;
    owner[1] = 0;

    /* do we have an original environment? if yes copy it (envsize is a number of paragraphs) */
    if (origenvseg != 0) memcpy_ltr_far(owner, MK_FP(origenvseg, 0), envsize * 16);
  }

  _asm {
    push ax
    push bx
    push dx
    push ds

    /* preset DS with RMOD's seg since I will work on RMOD fields */
    mov ds, rmodseg

    /***********************************************
     * set CTRL+BREAK and CRITERR handlers to rmod *
     ***********************************************/
    mov ax, 0x2523
    mov dx, RMOD_OFFSET_BRKHANDLER
    int 0x21

    mov ax, 0x2524
    mov dx, RMOD_OFFSET_CRITHANDLER
    int 0x21

    /***********************************************
     * write boot drive to RMOD's bootdrive field  *
     ***********************************************/
    mov ax, 0x3305 /* DOS 4.0+ - GET BOOT DRIVE */
    int 0x21       /* boot drive is in DL now (1=A:, 2=B:, etc) */
    add dl, '@'    /* convert to a proper ASCII letter */
    /* write boot drive to rmod bootdrive field */
    mov bx, RMOD_OFFSET_BOOTDRIVE
    mov [bx], dl

    /***********************************************
     * mark command-line history buffer as empty   *
     ***********************************************/
    mov bx, RMOD_OFFSET_INPUTBUF
    mov byte ptr [bx+0], 128     /* max acceptable length */
    mov byte ptr [bx+1], 0       /* len of currently stored history string */
    mov byte ptr [bx+2], 0x0D    /* string terminator */
    mov word ptr [bx+3], 0xFECA  /* signature to detect stack overflow damaging the buffer */

    /***********************************************
     * write env segment to RMOD's PSP             *
     ***********************************************/
    mov bx, RMOD_OFFSET_ENVSEG
    mov ax, envseg
    mov [bx], ax

    pop ds
    pop dx
    pop bx
    pop ax
  }

  /* prepare result (rmod props) */
  res = MK_FP(rmodseg, 0x100 + rmodcore_len);
  sv_bzero(res, sizeof(*res));     /* zero out */
  res->rmodseg = rmodseg;          /* rmod segment */

  /* if /MSG then allocate a lang block */
  if (*cfgflags & FLAG_MSG) {
    /* supplied through DEFLANG.C */
    extern char svarlang_mem[];
    extern unsigned short svarlang_dict[];
    extern const unsigned short svarlang_memsz;
    extern const unsigned short svarlang_string_count;

    /* alloc (high) memory for svarlang strings and dict index */
    res->lng_segmem = alloc_high_seg((svarlang_memsz + 15) / 16);
    res->lng_segdict = alloc_high_seg((svarlang_string_count * 4 + 15) / 16);

    /* back out if failed on any of the allocations */
    if ((res->lng_segdict == 0) || (res->lng_segmem == 0)) {
      if (res->lng_segdict != 0) freeseg(res->lng_segdict);
      if (res->lng_segmem != 0) freeseg(res->lng_segmem);
      res->lng_segdict = 0;
      res->lng_segmem = 0;
    } else { /* success */
      /* mark lng mem seg as owned by RMOD */
      mcb = MK_FP(res->lng_segmem - 1, 0);
      owner = (void far *)(mcb + 1);
      *owner = rmodseg;
      memcpy_ltr_far(mcb + 8, "SVARLNGM", 8);

      /* mark lng dict seg as owned by RMOD */
      mcb = MK_FP(res->lng_segdict - 1, 0);
      owner = (void far *)(mcb + 1);
      *owner = rmodseg;
      memcpy_ltr_far(mcb + 8, "SVARLNGD", 8);
    }
  }

  /* save my original int22h handler and parent in rmod's memory */
  res->origint22 = *((unsigned long far *)PSP_PTR(0x0a)); /* original int22h handler seg:off is at 0x0a of my PSP */
  res->origparent = *((unsigned short far *)PSP_PTR(0x16)); /* PSP segment of my parent is at 0x16 of my PSP */

  /* set the int22 handler in my PSP to rmod so DOS jumps to rmod after I
   * terminate and save the original handler in rmod's memory */
  {
    unsigned short far *ptr = PSP_PTR(0x0a); /* int22 handler is at 0x0A of the PSP */
    ptr[0] = RMOD_OFFSET_ROUTINE;
    ptr[1] = rmodseg;
  }

  /* set my own parent to RMOD (this is not necessary for MS-DOS nor FreeDOS but
   * might be on other DOS implementations) */
  {
    unsigned short far *ptr = PSP_PTR(0x16);
    *ptr = rmodseg;
  }

  return(res);
}


/* look up my parent: if it's rmod then return a ptr to its props struct,
 * otherwise return NULL
 * I look at PSP[Ch] to locate RMOD (ie. the "terminate address") */
struct rmod_props far *rmod_find(unsigned short rmodcore_len) {
  unsigned short far *parent = PSP_PTR(0x0C);
  unsigned short far *ptr;
  const unsigned short sig[] = {0x1983, 0x1985, 0x2017, 0x2019};
  unsigned char far *cmdtail = PSP_PTR(0x80);
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
    /* here I need to translate the comspecfp far pointer into an offset
     * relative to env_seg */
    *comspecptr = FP_OFF(comspecfp) + ((FP_SEG(comspecfp) - env_seg) * 16);
  } else {
    *comspecptr = 0;
  }
}


/* allocates bytes of far memory, flags it as belonging to rmod
 * the new block can be optionally flagged as 'ident' (if not null) and zero
 * out the newly allocated memory.
 * returns a far ptr to the allocated block, or NULL on error */
void far *rmod_fcalloc(unsigned short bytes, unsigned short rmod_seg, char *ident) {
  unsigned short far *owner;
  unsigned short newseg = 0;

  /* ask DOS for a memory block (as high as possible) */
  _asm {
    push bx /* save initial value in BX so I can restore it later */

    /* get current allocation strategy and save it on stack */
    mov ax, 0x5800
    int 0x21
    push ax

    /* set strategy to 'last fit, try high then low memory' */
    mov ax, 0x5801
    mov bx, 0x0082
    int 0x21

    /* ask for a memory block and save the given segment to rmodseg */
    mov ah, 0x48  /* Allocate Memory */
    mov bx, bytes
    add bx, 15    /* convert bytes to paragraphs */
    shr bx, 1     /* bx /= 16 */
    shr bx, 1
    shr bx, 1
    shr bx, 1
    int 0x21

    /* error handling */
    jc FAIL

    /* save newly allocated segment to newseg */
    mov newseg, ax

    FAIL:
    /* restore initial allocation strategy */
    mov ax, 0x5801
    pop bx
    int 0x21

    pop bx /* restore BX to its initial value */
  }

  if (newseg == 0) return(NULL);

  /* mark memory as "owned by rmod" */
  owner = (void far *)(MK_FP(newseg - 1, 1));
  *owner = rmod_seg;

  /* set the MCB description to ident, if provided */
  if (ident) {
    char far *mcbdesc = MK_FP(newseg - 1, 8);
    int i;
    sv_bzero(mcbdesc, 8);
    for (i = 0; (i < 8) && (ident[i] != 0); i++) { /* field's length is limited to 8 bytes max */
      mcbdesc[i] = ident[i];
    }
  }

  /* zero out the memory before handing it out */
  sv_bzero(MK_FP(newseg, 0), bytes);

  return(MK_FP(newseg, 0));
}


/* free memory previously allocated by rmod_fcalloc() */
void rmod_ffree(void far *ptr) {
  unsigned short ptrseg;
  unsigned short myseg = 0;
  unsigned short far *owner;
  if (ptr == NULL) return;
  ptrseg = FP_SEG(ptr);

  /* get my own segment */
  _asm {
    mov myseg, cs
  }

  /* mark memory in MCB as my own, otherwise DOS might refuse to free it */
  owner = MK_FP(ptrseg - 1, 1);
  *owner = myseg;

  /* free the memory block */
  _asm {
    push es
    mov ah, 0x49  /* Free Memory Block */
    mov es, ptrseg
    int 0x21
    pop es
  }
}


/* free the entire linked list of bat ctx nodes (and set its rmod ptr to NULL) */
void rmod_free_bat_llist(struct rmod_props far *rmod) {
  while (rmod->bat != NULL) {
    struct batctx far *victim = rmod->bat;
    rmod->bat = rmod->bat->parent;
    rmod_ffree(victim);
  }
}
