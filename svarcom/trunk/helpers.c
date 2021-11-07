/*
 * a variety of helper functions
 * Copyright (C) 2021 Mateusz Viste
 */

#include <i86.h>    /* MK_FP() */

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


/* converts a path to its canonic representation, returns 0 on success
 * or DOS err on failure (invalid drive) */
unsigned short file_truename(const char *src, char *dst) {
  unsigned short res = 0;
  _asm {
    push es
    mov ah, 0x60  /* query truename, DS:SI=src, ES:DI=dst */
    push ds
    pop es
    mov si, src
    mov di, dst
    int 0x21
    jnc DONE
    mov [res], ax
    DONE:
    pop es
  }
  return(res);
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


/* returns screen's width (in columns) */
unsigned short screen_getwidth(void) {
  /* BIOS 0040:004A = word containing screen width in text columns */
  unsigned short far *scrw = MK_FP(0x40, 0x4a);
  return(*scrw);
}


/* returns screen's height (in rows) */
unsigned short screen_getheight(void) {
  /* BIOS 0040:0084 = byte containing maximum valid row value (EGA ONLY) */
  unsigned char far *scrh = MK_FP(0x40, 0x84);
  if (*scrh == 0) return(25);  /* pre-EGA adapter */
  return(*scrh + 1);
}


/* displays the "Press any key to continue" msg and waits for a keypress */
void press_any_key(void) {
  output("Press any key to continue...");
  _asm {
    mov ah, 0x08  /* no echo console input */
    int 0x21      /* pressed key in AL now (0 for extended keys) */
    test al, al
    jnz DONE
    int 0x21      /* executed ah=8 again to read the rest of extended key */
    DONE:
    /* output CR/LF */
    mov ah, 0x02
    mov dl, 0x0D
    int 0x21
    mov dl, 0x0A
    int 0x21
  }
}


/* validate a drive (A=0, B=1, etc). returns 1 if valid, 0 otherwise */
int isdrivevalid(unsigned char drv) {
  _asm {
    mov ah, 0x19  /* query default (current) disk */
    int 0x21      /* drive in AL (0=A, 1=B, etc) */
    mov ch, al    /* save current drive to ch */
    /* try setting up the drive as current */
    mov ah, 0x0E   /* select default drive */
    mov dl, [drv]  /* 0=A, 1=B, etc */
    int 0x21
    /* this call does not set CF on error, I must check cur drive to look for success */
    mov ah, 0x19  /* query default (current) disk */
    int 0x21      /* drive in AL (0=A, 1=B, etc) */
    mov [drv], 1  /* preset result as success */
    cmp al, dl    /* is eq? */
    je DONE
    mov [drv], 0  /* fail */
    jmp FAILED
    DONE:
    /* set current drive back to what it was initially */
    mov ah, 0x0E
    mov dl, ch
    int 0x21
    FAILED:
  }
  return(drv);
}


/* converts a 8+3 filename into 11-bytes FCB format (MYFILE  EXT) */
void file_fname2fcb(char *dst, const char *src) {
  unsigned short i;

  /* fill dst with 11 spaces and a NULL terminator */
  for (i = 0; i < 12; i++) dst[i] = ' ';
  dst[12] = 0;

  /* copy fname until dot (.) or 8 characters */
  for (i = 0; i < 8; i++) {
    if ((src[i] == '.') || (src[i] == 0)) break;
    dst[i] = src[i];
  }

  /* advance src until extension or end of string */
  src += i;
  for (;;) {
    if (*src == '.') {
      src++; /* next character is extension */
      break;
    }
    if (*src == 0) break;
  }

  /* copy extension to dst (3 chars max) */
  dst += 8;
  for (i = 0; i < 3; i++) {
    if (src[i] == 0) break;
    dst[i] = src[i];
  }
}


/* converts a 11-bytes FCB filename (MYFILE  EXT) into 8+3 format (MYFILE.EXT) */
void file_fcb2fname(char *dst, const char *src) {
  unsigned short i, end = 0;

  for (i = 0; i < 8; i++) {
    dst[i] = src[i];
    if (dst[i] != ' ') end = i + 1;
  }

  /* is there an extension? */
  if (src[8] == ' ') {
    dst[end] = 0;
  } else { /* found extension: copy it until first space */
    dst[end++] = '.';
    for (i = 8; i < 11; i++) {
      if (src[i] == ' ') break;
      dst[end++] = src[i];
    }
    dst[end] = 0;
  }
}


/* converts an ASCIIZ string into an unsigned short. returns 0 on success. */
int atouns(unsigned short *r, const char *s) {
  int err = 0;

  _asm {
    mov si, s
    xor ax, ax  /* general purpose register */
    xor cx, cx  /* contains the result */
    mov bx, 10  /* used as a multiplicative step */

    NEXTBYTE:
    xchg cx, ax /* move result into cx temporarily */
    lodsb  /* AL = DS:[SI++] */
    /* is AL 0? if so we're done */
    test al, al
    jz DONE
    /* validate that AL is in range '0'-'9' */
    sub al, '0'
    jc FAIL   /* neg result */
    cmp al, 9
    jg FAIL
    /* restore result into AX (CX contains the new digit) */
    xchg cx, ax
    /* multiply result by 10 and add cl */
    mul bx    /* DX AX = AX * BX(10) */
    jc FAIL   /* overflow */
    add ax, cx
    /* if CF then overflow occured (overflow part lands in DX) */
    jnc NEXTBYTE

    FAIL:
    inc [err]

    DONE: /* save result (CX) into indirect memory address r */
    mov bx, [r]
    mov [bx], cx
  }
  return(err);
}


/* appends a backslash if path is a directory
 * returns the (possibly updated) length of path */
unsigned short path_appendbkslash_if_dir(char *path) {
  unsigned short len;
  int attr;
  for (len = 0; path[len] != 0; len++);
  if (len == 0) return(0);
  if (path[len - 1] == '\\') return(len);
  /* */
  attr = file_getattr(path);
  if ((attr > 0) && (attr & DOS_ATTR_DIR)) {
    path[len++] = '\\';
    path[len] = 0;
  }
  return(len);
}


/* get current path drive d (A=1, B=2, etc - 0 is "current drive")
 * returns 0 on success, doserr otherwise */
unsigned short curpathfordrv(char *buff, unsigned char d) {
  unsigned short r = 0;

  _asm {
    /* is d == 0? then I need to resolve current drive */
    cmp byte ptr [d], 0
    jne GETCWD
    /* resolve cur drive */
    mov ah, 0x19  /* get current default drive */
    int 0x21      /* al = drive (00h = A:, 01h = B:, etc) */
    inc al        /* convert to 1=A, 2=B, etc */
    mov [d], al

    GETCWD:
    /* prepend buff with drive:\ */
    mov si, buff
    mov dl, [d]
    mov [si], dl
    add byte ptr [si], 'A' - 1
    inc si
    mov [si], ':'
    inc si
    mov [si], '\\'
    inc si

    mov ah, 0x47      /* get current directory of drv DL into DS:SI */
    int 0x21
    jnc DONE
    mov [r], ax       /* copy result from ax */

    DONE:
  }

  return(r);
}
