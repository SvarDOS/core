/* SVARDISK output and small memory helpers.
 * Copyright (C) 2026 Mateusz Viste. MIT license, see svardisk.c.
 */

#include "svardisk.h"

#include "svarlang.h"

#ifdef SV_HOST
#include <stdio.h>
#endif

Vol vol;

void mem_set(void *p, u8 v, u16 n) {
  u8 *d;
  u16 i;
  d = (u8 *)p;
  for (i = 0; i < n; i++) d[i] = v;
}

void mem_copy(void *dst, const void *src, u16 n) {
  u8 *d;
  const u8 *s;
  u16 i;
  d = (u8 *)dst;
  s = (const u8 *)src;
  for (i = 0; i < n; i++) d[i] = s[i];
}

int mem_eq(const void *a, const void *b, u16 n) {
  const u8 *x;
  const u8 *y;
  u16 i;
  x = (const u8 *)a;
  y = (const u8 *)b;
  for (i = 0; i < n; i++) {
    if (x[i] != y[i]) return(0);
  }
  return(1);
}

#ifdef SV_HOST

void out_str(const char *s) {
  fputs(s, stdout);
}

void out_chr(char c) {
  fputc(c, stdout);
}

void out_eol(void) {
  fputs("\r\n", stdout);
  fflush(stdout);
}

void out_msg(u16 major, u16 minor) {
  out_str(svarlang_str(major, minor));
}

#else

static void dos_write(const char *s, u16 n);
#pragma aux dos_write = \
  "mov ah, 40h" \
  "mov bx, 1" \
  "int 21h" \
  parm [dx] [cx] modify [ax bx]

static void dos_putc(u8 c);
#pragma aux dos_putc = \
  "mov ah, 2" \
  "int 21h" \
  parm [dl] modify [ax]

void out_str(const char *s) {
  u16 n;
  n = 0;
  while (s[n]) n++;
  if (n) dos_write(s, n);
}

void out_chr(char c) {
  dos_putc((u8)c);
}

void out_eol(void) {
  dos_write("\r\n", 2);
}

void out_msg(u16 major, u16 minor) {
  out_str(svarlang_str(major, minor));
}

#endif

void out_u32(u32 n) {
  char buf[11];
  int i;
  i = 10;
  buf[10] = 0;
  if (n == 0) {
    out_chr('0');
    return;
  }
  while (n > 0) {
    buf[--i] = (char)('0' + (u8)(n % 10));
    n = n / 10;
  }
  out_str(buf + i);
}

void out_dosname(const u8 *n11) {
  int i;
  for (i = 0; i < 8 && n11[i] != ' '; i++) out_chr((char)n11[i]);
  if (n11[8] != ' ') {
    out_chr('.');
    for (i = 8; i < 11 && n11[i] != ' '; i++) out_chr((char)n11[i]);
  }
}

static int noting;
static int last_shown;
static u16 shown;

void note_begin(void) {
  if (vol.problems < 65535u) vol.problems++;
  noting = 0;
  last_shown = 0;
  if (shown < 24) {
    noting = 1;
    return;
  }
  if (shown == 24) {
    out_msg(4, 18);
    out_eol();
    shown++;
  }
}

void note_msg(u16 major, u16 minor) {
  if (noting) out_msg(major, minor);
}

void note_chr(char c) {
  if (noting) out_chr(c);
}

void note_num(u32 n) {
  if (noting) out_u32(n);
}

void note_nam(const u8 *n11) {
  if (noting) out_dosname(n11);
}

void note_end(void) {
  if (noting) {
    out_eol();
    shown++;
    last_shown = 1;
  }
  noting = 0;
}

void note_line(u16 major, u16 minor) {
  note_begin();
  note_msg(major, minor);
  note_end();
}

int note_shown(void) {
  return(last_shown);
}
