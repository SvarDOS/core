/*
 * a variety of helper functions
 * Copyright (C) 2021 Mateusz Viste
 */

#include "helpers.h"

/* case-insensitive comparison of strings, returns non-zero on equality */
int imatch(const char *s1, const char *s2) {
  for (;;) {
    char c1, c2;
    c1 = *s1;
    c2 = *s2;
    if ((c1 >= 'a') && (c1 <= 'z')) c1 -= ('a' - 'A');
    if ((c2 >= 'a') && (c2 <= 'z')) c2 -= ('a' - 'A');
    /* */
    if (c1 != c2) return(0);
    if (c1 == 0) return(1);
    s1++;
    s2++;
  }
}


/* returns zero if s1 starts with s2 */
int strstartswith(const char *s1, const char *s2) {
  while (*s2 != 0) {
    if (*s1 != *s2) return(-1);
    s1++;
    s2++;
  }
  return(0);
}


/* outputs a NULL-terminated string to stdout */
void output_internal(const char *s, unsigned short nl) {
  _asm {
    mov ah, 0x02 /* AH=9 - write character in DL to stdout */
    mov si, s
    cld          /* clear DF so lodsb increments SI */
    NEXTBYTE:
    lodsb /* load byte from DS:SI into AL, SI++ */
    mov dl, al
    or al, 0  /* is al == 0? */
    jz DONE
    int 0x21
    jmp NEXTBYTE
    DONE:
    or nl, 0
    jz FINITO
    /* print out a CR/LF trailer if nl set */
    mov dl, 0x0D /* CR */
    int 0x21
    mov dl, 0x0A /* LF */
    int 0x21
    FINITO:
  }
}


/* find first matching files using a FindFirst DOS call
 * returns 0 on success or a DOS err code on failure */
unsigned short findfirst(struct DTA *dta, const char *pattern, unsigned short attr) {
  unsigned short res = 0;
  _asm {
    /* set DTA location */
    mov ah, 0x1a
    mov dx, dta
    int 0x21
    /* */
    mov ah, 0x4e    /* FindFirst */
    mov dx, pattern
    mov cx, attr
    int 0x21        /* CF set on error + err code in AX, DTA filled with FileInfoRec on success */
    jnc DONE
    mov [res], ax
    DONE:
  }
  return(res);
}


/* find next matching, ie. continues an action intiated by findfirst() */
unsigned short findnext(struct DTA *dta) {
  unsigned short res = 0;
  _asm {
    mov ah, 0x4f    /* FindNext */
    mov dx, dta
    int 0x21        /* CF set on error + err code in AX, DTA filled with FileInfoRec on success */
    jnc DONE
    mov [res], ax
    DONE:
  }
  return(res);
}
