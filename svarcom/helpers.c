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


/* print s string and wait for a single key press from stdin. accepts only
 * key presses defined in the c ASCIIZ string. returns offset of pressed key
 * in string. keys in c MUST BE UPPERCASE! */
unsigned short askchoice(const char *s, const char *c) {
  unsigned short res;
  char key = 0;

  AGAIN:
  output(s);
  output(" ");

  _asm {
    push ax
    push dx

    mov ax, 0x0c01 /* clear input buffer and execute getchar (INT 21h,AH=1) */
    int 0x21
    /* if AL == 0 then this is an extended character */
    test al, al
    jnz GOTCHAR
    mov ah, 0x08   /* read again to flush extended char from input buffer */
    int 0x21
    xor al, al     /* all extended chars are ignored */
    GOTCHAR:       /* received key is in AL now */
    mov [key], al  /* save key */

    /* print a cr/lf */
    mov ah, 0x02
    mov dl, 0x0D
    int 0x21
    mov dl, 0x0A
    int 0x21

    pop dx
    pop ax
  }

  /* ucase() result */
  if ((key >= 'a') && (key <= 'z')) key -= ('a' - 'A');

  /* is there a match? */
  for (res = 0; c[res] != 0; res++) if (c[res] == key) return(res);

  goto AGAIN;
}


/* converts a path to its canonic representation */
void file_truename(const char *src, char *dst) {
  _asm {
    mov ah, 0x60  /* query truename, DS:SI=src, ES:DI=dst */
    push ds
    pop es
    mov si, src
    mov di, dst
    int 0x21
  }
}


/* returns DOS attributes of file, or -1 on error */
int file_getattr(const char *fname) {
  int res = -1;
  _asm {
    mov ax, 0x4300  /* query file attributes, fname at DS:DX */
    mov dx, fname
    int 0x21        /* CX=attributes if CF=0, otherwise AX=errno */
    jc DONE
    mov [res], cx
    DONE:
  }
  return(res);
}
