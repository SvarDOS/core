/* This file is part of the SvarCOM project and is published under the terms
 * of the MIT license.
 *
 * Copyright (C) 2021-2026 Mateusz Viste
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

#include "helpers.h"

#include "sysparse.h"

/* SYSPARSE-style entry passed to MS-DOS APPEND in ES:DI on AX=AE01h.
 * FreeDOS APPEND never reads ES:DI. The routine is far because APPEND
 * far-calls it, and it must preserve ES and DI: APPEND reuses them for
 * every argument. Results come back in AX, CX, SI, DX and BL. */
static volatile unsigned short mux_in_es = 0;
static volatile unsigned short mux_in_di = 0;
static volatile unsigned short mux_in_si = 0;
static volatile unsigned short mux_in_cx = 0;
static volatile unsigned short mux_out_ax = 0;
static volatile unsigned short mux_out_bx = 0;
static volatile unsigned short mux_out_cx = 0;
static volatile unsigned short mux_out_dx = 0;
static volatile unsigned short mux_out_si = 0;
static char mux_strbuf[128];
static char mux_upbuf[128];

static unsigned char mux_peekb(unsigned short seg, unsigned short off) {
  return(*(unsigned char far *)MK_FP(seg, off));
}

static unsigned short mux_peekw(unsigned short seg, unsigned short off) {
  return(*(unsigned short far *)MK_FP(seg, off));
}

static void mux_pokeb(unsigned short seg, unsigned short off, unsigned char v) {
  *(unsigned char far *)MK_FP(seg, off) = v;
}

static void mux_pokew(unsigned short seg, unsigned short off, unsigned short v) {
  *(unsigned short far *)MK_FP(seg, off) = v;
}

/* filename uppercase table (INT 21h AX=6521h); character table if that is absent */
static void mux_caps_file(char *s) {
  unsigned short err = 0;
  unsigned short n = 0;

  /* CX is the string length. DOS writes that many bytes; a stale CX
   * runs off the end of the buffer and APPEND never returns. */
  while (s[n] != 0) n++;
  _asm {
    push ax
    push cx
    push dx
    mov ax, 0x6521
    mov cx, n
    mov dx, s
    int 0x21
    jnc caps_done
    mov err, ax
    caps_done:
    pop dx
    pop cx
    pop ax
  }
  if (err != 0) nls_strtoup(s);
}

/* uppercase the result the way the control block requested */
static void mux_caps_result(char *s, unsigned short fflag) {
  if (fflag & 0x0001) mux_caps_file(s);       /* $P_CAP_File */
  else if (fflag & 0x0002) nls_strtoup(s);    /* $P_CAP_Char */
}

static void mux_fill_result(unsigned short es, unsigned short ctrl, unsigned char type, unsigned char tag, unsigned short synoff, unsigned short pickoff, unsigned short pickseg) {
  unsigned short rb;

  rb = mux_peekw(es, (unsigned short)(ctrl + 4));
  mux_pokeb(es, rb, type);
  mux_pokeb(es, (unsigned short)(rb + 1), tag);
  mux_pokew(es, (unsigned short)(rb + 2), synoff);
  mux_pokew(es, (unsigned short)(rb + 4), pickoff);
  if (type != 2) mux_pokew(es, (unsigned short)(rb + 6), pickseg); /* list index stores one word */
  mux_out_dx = rb;
}

/* 1 = matched a string choice, 0 = no value list, -1 = not a string list, 2 = not in the list */
static int mux_valuelist(unsigned short es, unsigned short vlist, const char *val, unsigned char *tag, unsigned short *pick) {
  unsigned char nval;
  unsigned char count;
  unsigned short p;

  nval = mux_peekb(es, vlist);
  if (nval == 0) return(0);
  if (nval != 3) return(-1); /* $P_nval_String */

  p = (unsigned short)(vlist + 1);
  count = mux_peekb(es, p); /* ranges */
  p = (unsigned short)(p + 1 + (unsigned short)count * 9);
  count = mux_peekb(es, p); /* numeric choices */
  p = (unsigned short)(p + 1 + (unsigned short)count * 5);
  count = mux_peekb(es, p); /* string choices */
  p = (unsigned short)(p + 2); /* first choice's string pointer; tag is the byte before it */
  while (count > 0) {
    unsigned short strp;
    const char *s;
    unsigned short o;

    strp = mux_peekw(es, p);
    s = val;
    o = strp;
    for (;;) {
      unsigned char a = mux_peekb(es, o);
      unsigned char b = (unsigned char)*s;
      if (a != b) break;
      if (a == 0) {
        *tag = mux_peekb(es, (unsigned short)(p - 1));
        *pick = strp;
        return(1);
      }
      o++;
      s++;
    }
    p = (unsigned short)(p + 3);
    count--;
  }
  return(2);
}

static void mux_parse_c(void) {
  unsigned short es = mux_in_es;
  unsigned short di = mux_in_di;
  unsigned short si = mux_in_si;
  unsigned short cx = mux_in_cx;
  unsigned short dsreg = 0;
  unsigned char far *line;
  unsigned short parmsx;
  unsigned char minp;
  unsigned char maxp;
  unsigned short i;
  unsigned short n;
  int haseq;
  unsigned short fflag;
  unsigned short mflag;
  unsigned short ctrl;
  unsigned short synoff;
  unsigned short vindex;
  unsigned char tag;
  unsigned short pick;
  int vm;
  unsigned short dseg;

  _asm {
    mov dsreg, ds
  }
  line = (unsigned char far *)MK_FP(dsreg, 0);
  mux_out_bx = 0x0d;
  mux_out_dx = 0;
  mux_out_cx = cx;
  mux_out_si = si;

  /* skip spaces, tabs and commas. semicolon is not a delimiter for APPEND */
  for (;;) {
    unsigned char c = line[si];
    if ((c == 0) || (c == 0x0d)) {
      parmsx = mux_peekw(es, di);
      minp = mux_peekb(es, parmsx);
      mux_out_ax = ((unsigned char)cx >= minp) ? 0xffff : 2;
      mux_out_si = si;
      return;
    }
    if ((c != ' ') && (c != '\t') && (c != ',')) break;
    si++;
  }

  /* pack one operand. a slash ends it only once a character has been copied,
   * so /X stays intact and C:\FOO/X splits into the path and the switch */
  n = 0;
  haseq = 0;
  for (;;) {
    unsigned char c = line[si];
    if ((c == 0) || (c == 0x0d)) break;
    if ((c == ' ') || (c == '\t') || (c == ',')) break;
    if ((n > 0) && (c == '/') && !((line[si - 1] >= '0') && (line[si - 1] <= '9') && (line[si + 1] >= '0') && (line[si + 1] <= '9'))) break;
    if (c == '=') haseq = 1;
    if (n < 126) mux_strbuf[n++] = (char)c;
    si++;
  }
  mux_strbuf[n] = 0;
  if ((line[si] == ' ') || (line[si] == '\t')) {
    while ((line[si] == ' ') || (line[si] == '\t')) si++;
  } else if (line[si] == ',') {
    si++;
  }
  mux_out_si = si;
  mux_out_bx = line[si];

  for (i = 0; i <= n; i++) mux_upbuf[i] = mux_strbuf[i];
  nls_strtoup(mux_upbuf);

  parmsx = mux_peekw(es, di);
  maxp = mux_peekb(es, (unsigned short)(parmsx + 1));

  if (mux_upbuf[0] == '/') {
    unsigned short sw = (unsigned short)(parmsx + ((unsigned short)maxp + 1) * 2);
    unsigned char nsw = mux_peekb(es, sw);
    unsigned short sp = (unsigned short)(sw + 1);
    int found = 0;

    ctrl = 0;
    synoff = 0;
    vindex = 0;
    while (nsw > 0) {
      unsigned char nid;
      unsigned short syn;

      ctrl = mux_peekw(es, sp);
      sp = (unsigned short)(sp + 2);
      nid = mux_peekb(es, (unsigned short)(ctrl + 8));
      syn = (unsigned short)(ctrl + 9);
      while (nid > 0) {
        unsigned short ti = 0;
        unsigned short so = syn;
        int match = 0;

        for (;;) {
          unsigned char sc = mux_peekb(es, so);
          unsigned char tc = (unsigned char)mux_upbuf[ti];
          if ((sc == 0) && ((tc == 0) || (tc == ':'))) {
            match = 1;
            vindex = ti;
            break;
          }
          if ((sc == 0) || (tc == 0) || (tc == ':') || (sc != tc)) break;
          so++;
          ti++;
        }
        if (match) {
          found = 1;
          synoff = syn;
          break;
        }
        while (mux_peekb(es, syn) != 0) syn++;
        syn++;
        nid--;
      }
      if (found) break;
      nsw--;
    }
    if (!found) {
      mux_out_ax = 3; /* switch not in the control block */
      return;
    }

    mflag = mux_peekw(es, ctrl);
    fflag = mux_peekw(es, (unsigned short)(ctrl + 2));
    if (mux_upbuf[vindex] == ':') vindex++;
    dseg = dsreg;
    if (mux_upbuf[vindex] == 0) {
      if ((vindex > 0) && (mux_upbuf[vindex - 1] == ':')) mux_out_ax = 9; /* "/X:" */
      else if ((mflag == 0) || (mflag & 0x0001)) mux_out_ax = 0;          /* bare /E or optional /X */
      else mux_out_ax = 2;                                                /* /PATH needs a value */
      mux_fill_result(es, ctrl, 3, 0xff, synoff, (unsigned short)(FP_OFF(mux_strbuf) + vindex), dseg);
      mux_caps_result(mux_strbuf + vindex, fflag);
      return;
    }

    tag = 0xff;
    pick = 0;
    vm = mux_valuelist(es, mux_peekw(es, (unsigned short)(ctrl + 6)), mux_upbuf + vindex, &tag, &pick);
    if (vm == 1) {
      mux_out_ax = 0;
      mux_fill_result(es, ctrl, 2, tag, synoff, pick, 0); /* type 2: list index, item tag is ON/OFF */
      return;
    }
    if (vm == 0) {
      mux_out_ax = 0;
      mux_fill_result(es, ctrl, 3, 0xff, synoff, (unsigned short)(FP_OFF(mux_strbuf) + vindex), dseg);
      mux_caps_result(mux_strbuf + vindex, fflag);
      return;
    }
    mux_out_ax = (vm < 0) ? 9 : 8;
    mux_fill_result(es, ctrl, 3, 0xff, synoff, (unsigned short)(FP_OFF(mux_strbuf) + vindex), dseg);
    return;
  }

  if (haseq) {
    mux_out_ax = 4; /* keyword, and APPEND defines none */
    return;
  }

  if (cx >= maxp) {
    mux_out_ax = 1; /* a second directory */
    return;
  }
  ctrl = mux_peekw(es, (unsigned short)(parmsx + 2 + cx * 2));
  mflag = mux_peekw(es, ctrl);
  fflag = mux_peekw(es, (unsigned short)(ctrl + 2));
  if ((mflag & 0x0002) == 0) cx++; /* $P_Repeat would keep the same ordinal */
  mux_out_cx = cx;

  tag = 0xff;
  pick = 0;
  vm = mux_valuelist(es, mux_peekw(es, (unsigned short)(ctrl + 6)), mux_upbuf, &tag, &pick);
  dseg = dsreg;
  if (vm == 1) {
    mux_out_ax = 0;
    mux_fill_result(es, ctrl, 2, tag, 0, pick, 0);
    return;
  }
  if (vm == 0) {
    mux_out_ax = 0;
    mux_fill_result(es, ctrl, 3, 0xff, 0, FP_OFF(mux_strbuf), dseg);
    mux_caps_result(mux_strbuf, fflag);
    return;
  }
  mux_out_ax = (vm < 0) ? 9 : 8;
  mux_fill_result(es, ctrl, 3, 0xff, 0, FP_OFF(mux_strbuf), dseg);
}

/* far entry: APPEND calls this, then keeps ES:DI for the next operand.
 * aborts: retf leaves AX/BX/CX/DX/SI as the parser results. A compiler
 * epilogue would put the incoming SI back. cld on the way out: APPEND
 * copies the string with movsb and does not clear the direction flag. */
void far mux_parse_entry(void);
#pragma aux mux_parse_entry aborts;
void far mux_parse_entry(void) {
  _asm {
    mov word ptr mux_in_es, es
    mov word ptr mux_in_di, di
    mov word ptr mux_in_si, si
    mov word ptr mux_in_cx, cx
    push es
    push di
    cld
  }
  mux_parse_c();
  _asm {
    pop di
    pop es
    mov ax, word ptr mux_out_ax
    mov bx, word ptr mux_out_bx
    mov cx, word ptr mux_out_cx
    mov dx, word ptr mux_out_dx
    mov si, word ptr mux_out_si
    clc
    cld
    retf
  }
}
