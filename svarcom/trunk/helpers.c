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

/*
 * a variety of helper functions
 */

#include <i86.h>    /* MK_FP() */

#include "svarlang.lib\svarlang.h"

#include "env.h"
#include "rmodinit.h"

#include "helpers.h"


void dos_get_date(unsigned short *y, unsigned char *m, unsigned char *d) {
  /* get cur date */
  _asm {
    mov ah, 0x2a  /* DOS 1+ -- Query DOS Date */
    int 0x21      /* CX=year DH=month DL=day */
    mov bx, y
    mov [bx], cx
    mov bx, m
    mov [bx], dh
    mov bx, d
    mov [bx], dl
  }
}


void dos_get_time(unsigned char *h, unsigned char *m, unsigned char *s) {
  _asm {
    mov ah, 0x2c  /* DOS 1+ -- Query DOS Time */
    int 0x21      /* CH=hour CL=minutes DH=seconds DL=1/100sec */
    mov bx, h
    mov [bx], ch
    mov bx, m
    mov [bx], cl
    mov bx, s
    mov [bx], dh
  }
}


/* like strlen() */
unsigned short sv_strlen(const char *s) {
  unsigned short len = 0;
  while (*s != 0) {
    s++;
    len++;
  }
  return(len);
}


/* case-insensitive comparison of strings, compares up to maxlen characters.
 * returns non-zero on equality. */
int imatchlim(const char *s1, const char *s2, unsigned short maxlen) {
  while (maxlen--) {
    char c1, c2;
    c1 = *s1;
    c2 = *s2;
    if ((c1 >= 'a') && (c1 <= 'z')) c1 -= ('a' - 'A');
    if ((c2 >= 'a') && (c2 <= 'z')) c2 -= ('a' - 'A');
    /* */
    if (c1 != c2) return(0);
    if (c1 == 0) break;
    s1++;
    s2++;
  }
  return(1);
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


/* outputs a NULL-terminated string to handle (1=stdout 2=stderr) */
void output_internal(const char *s, unsigned char nl, unsigned char handle) {
  const static unsigned char *crlf = "\r\n";
  _asm {
    push ds
    pop es         /* make sure es=ds (scasb uses es) */
    /* get length of s into CX */
    mov ax, 0x4000 /* ah=DOS "write to file" and AL=0 for NULL matching */
    mov dx, s      /* set dx to string (required for later) */
    mov di, dx     /* set di to string (for NULL matching) */
    mov cx, 0xffff /* preset cx to 65535 (-1) */
    cld            /* clear DF so scasb increments DI */
    repne scasb    /* cmp al, es:[di], inc di, dec cx until match found */
    /* CX contains (65535 - strlen(s)) now */
    not cx         /* reverse all bits so I get (strlen(s) + 1) */
    dec cx         /* this is CX length */
    jz WRITEDONE   /* do nothing for empty strings */

    /* output by writing to stdout */
    /* mov ah, 0x40 */  /* DOS 2+ -- write to file via handle */
    xor bh, bh
    mov bl, handle /* set handle (1=stdout 2=stderr) */
    /* mov cx, xxx */ /* write CX bytes */
    /* mov dx, s   */ /* DS:DX is the source of bytes to "write" */
    int 0x21
    WRITEDONE:

    /* print out a CR/LF trailer if nl set */
    test byte ptr [nl], 0xff
    jz FINITO
    /* bx still contains handle */
    mov ah, 0x40 /* "write to file" */
    mov cx, 2
    mov dx, crlf
    int 0x21
    FINITO:
  }
}


void nls_output_internal(unsigned short id, unsigned char nl, unsigned char handle) {
  const char *NOTFOUND = "NLS_STRING_NOT_FOUND";
  const char *ptr = svarlang_strid(id);
  if ((ptr == NULL) || (ptr[0]) == 0) ptr = NOTFOUND;
  output_internal(ptr, nl, handle);
}


/* output DOS error e to stdout, if stdout is redirected then *additionally*
 * also to stderr */
void nls_outputnl_doserr(unsigned short e) {
  static char errstr[16];
  const char *ptr = NULL;
  unsigned char redirflag = 0;
  /* find string in nls block */
  if (e < 0xff) ptr = svarlang_strid(0xff00 | e);
  /* if not found, use a fallback */
  if ((ptr == NULL) || (ptr[0] == 0)) {
    sv_strcpy(errstr, "DOS ERR ");
    ustoa(errstr + sv_strlen(errstr), e, 0, '0');
    ptr = errstr;
  }

  /* display to stdout */
  output_internal(ptr, 1, hSTDOUT);

  /* is stdout redirected? */
  _asm {
    push bx
    push dx

    mov ax, 0x4400   /* query device flags */
    mov bx, 1        /* stdout */
    int 0x21
    /* CF set on error and AX filled with DOS error,
     * returns flags in DX on succes:
     *  bit 7 reset if handle points to a file, set if handle points to a device  */
    jc FAIL
    mov redirflag, dl
    and redirflag, 128

    FAIL:
    pop dx
    pop bx
  }

  if (redirflag == 0) output_internal(ptr, 1, hSTDERR);
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
    /* set DTA location */
    mov ah, 0x1a
    mov dx, dta
    int 0x21
    /* FindNext */
    mov ah, 0x4f
    int 0x21        /* CF set on error + err code in AX, DTA filled with FileInfoRec on success */
    jnc DONE
    mov [res], ax
    DONE:
  }
  return(res);
}


static unsigned char _dos_getkey_noecho(void);
#pragma aux _dos_getkey_noecho = \
"mov ax, 0x0c08" /* clear input buffer and execute getchar (INT 21h,AH=8) */  \
"int 0x21"                                                                    \
"test al, al"    /* if AL == 0 then this is an extended character */          \
"jnz GOTCHAR"                                                                 \
"mov ah, 0x08"   /* read again to flush extended char from input buffer */    \
"int 0x21"                                                                    \
"xor al, al"     /* all extended chars are ignored */                         \
"GOTCHAR:"       /* received key is in AL now */                              \
modify [ah]                                                                   \
value [al]


/* print s string and wait for a single key press from stdin. accepts only
 * key presses defined in the c ASCIIZ string. returns offset of pressed key
 * in string. keys in c MUST BE UPPERCASE! ENTER chooses the FIRST choice */
unsigned short askchoice(const char *s, const char *c) {
  unsigned short res;
  char cstr[2] = {0,0};
  char key = 0;

  output(s);
  output(" ");
  output("(");
  for (res = 0; c[res] != 0; res++) {
    if (res != 0) output("/");
    cstr[0] = c[res];
    output(cstr);
  }
  output(") ");

  AGAIN:
  key = _dos_getkey_noecho();
  if (key == '\r') key = c[0]; /* ENTER is synonym for the first key */

  /* ucase() result */
  if ((key >= 'a') && (key <= 'z')) key -= ('a' - 'A');

  /* is there a match? */
  for (res = 0; c[res] != 0; res++) {
    if (c[res] == key) {
      cstr[0] = key;
      output(cstr);
      output("\r\n");
      return(res);
    }
  }

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
  nls_output(15, 1); /* Press any key to continue... */
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
  for (i = 0; i < 11; i++) dst[i] = ' ';
  dst[11] = 0;

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


/* convert an unsigned short to ASCIZ, output expanded to minlen chars
 * (prefixed with prefixchar if value too small, else truncated)
 * returns length of produced string */
unsigned short ustoa(char *dst, unsigned short n, unsigned char minlen, char prefixchar) {
  unsigned short r;
  unsigned char i, len;
  unsigned char nonzerocharat = 5;

  for (i = 4; i != 0xff; i--) {
    r = n % 10;
    n /= 10;
    dst[i] = '0' + r;
    if (r != 0) nonzerocharat = i;
  }

  /* change prefix to prefixchar (eg. "00120" -> "  120") */
  for (i = 0; i < 4; i++) {
    if (dst[i] != '0') break;
    dst[i] = prefixchar;
  }

  len = 5 - nonzerocharat;

  /* apply minlen, if set */
  if ((minlen > len) && (minlen < 6)) {
    len = minlen;
    nonzerocharat = 5 - minlen;
  }

  if (nonzerocharat != 0) {
    memcpy_ltr(dst, dst + nonzerocharat, len);
  }

  dst[len] = 0;

  return(len);
}


/* converts an unsigned short to a four-byte ASCIZ hex string ("0ABC") */
void ustoh(char *dst, unsigned short n) {
  const char *h = "0123456789ABCDEF";
  dst[2] = h[(n >> 4) & 15];
  dst[3] = h[n & 15];
  n >>= 8;
  dst[0] = h[n >> 4];
  dst[1] = h[n & 15];
  dst[4] = 0;
}


/* converts an ASCIIZ string into an unsigned short. returns 0 on success.
 * on error, result will contain all valid digits that were read until
 * error occurred (0 on overflow or if parsing failed immediately) */
int atous(unsigned short *r, const char *s) {
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
    jc FAIL   /* invalid character detected */
    cmp al, 9
    jg FAIL   /* invalid character detected */
    /* restore result into AX (CX contains the new digit) */
    xchg cx, ax
    /* multiply result by 10 and add cl */
    mul bx    /* DX AX = AX * BX(10) */
    jc OVERFLOW  /* overflow */
    add ax, cx
    /* if CF is set then overflow occurred (overflow part lands in DX) */
    jnc NEXTBYTE

    OVERFLOW:
    xor cx, cx  /* make sure result is zeroed in case overflow occured */

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


/* like strcpy() but returns the string's length */
unsigned short sv_strcpy(char *dst, const char *s) {
  unsigned short len = 0;
  for (;;) {
    *dst = *s;
    if (*s == 0) return(len);
    dst++;
    s++;
    len++;
  }
}


/* like sv_strcpy() but operates on far pointers */
unsigned short sv_strcpy_far(char far *dst, const char far *s) {
  unsigned short len = 0;
  for (;;) {
    *dst = *s;
    if (*s == 0) return(len);
    dst++;
    s++;
    len++;
  }
}


/* like strcat() */
void sv_strcat(char *dst, const char *s) {
  /* advance dst to end of string */
  while (*dst != 0) dst++;
  /* append string */
  sv_strcpy(dst, s);
}


/* like strcat() but operates on far pointers */
void sv_strcat_far(char far *dst, const char far *s) {
  /* advance dst to end of string */
  while (*dst != 0) dst++;
  /* append string */
  sv_strcpy_far(dst, s);
}


/* fills a nls_patterns struct with current NLS patterns, returns 0 on success, DOS errcode otherwise */
unsigned short nls_getpatterns(struct nls_patterns *p) {
  unsigned short r = 0;

  _asm {
    mov ax, 0x3800  /* DOS 2+ -- Get Country Info for current country */
    mov dx, p       /* DS:DX points to the CountryInfoRec buffer */
    int 0x21
    jnc DONE
    mov [r], ax     /* copy DOS err code to r */
    DONE:
  }

  return(r);
}


/* computes a formatted date based on NLS patterns found in p
 * returns length of result */
unsigned short nls_format_date(char *s, unsigned short yr, unsigned char mo, unsigned char dy, const struct nls_patterns *p) {
  unsigned short items[3];
  unsigned short slen;

  /* preset date/month/year in proper order depending on date format */
  switch (p->dateformat) {
    case 0:  /* USA style: m d y */
      items[0] = mo;
      items[1] = dy;
      items[2] = yr;
      break;
    case 1:  /* EU style: d m y */
      items[0] = dy;
      items[1] = mo;
      items[2] = yr;
      break;
    case 2:  /* Japan-style: y m d */
    default:
      items[0] = yr;
      items[1] = mo;
      items[2] = dy;
      break;
  }
  /* compute the string */

  slen = ustoa(s, items[0], 2, '0');
  slen += sv_strcpy(s + slen, p->datesep);
  slen += ustoa(s + slen, items[1], 2, '0');
  slen += sv_strcpy(s + slen, p->datesep);
  slen += ustoa(s + slen, items[2], 2, '0');

  return(slen);
}


/* computes a formatted time based on NLS patterns found in p, sc are ignored if set 0xff
 * returns length of result */
unsigned short nls_format_time(char *s, unsigned char ho, unsigned char mn, unsigned char sc, const struct nls_patterns *p) {
  char ampm = 0;
  unsigned short res;

  if (p->timefmt == 0) {
    if (ho == 12) {
      ampm = 'p';
    } else if (ho > 12) {
      ho -= 12;
      ampm = 'p';
    } else { /* ho < 12 */
      if (ho == 0) ho = 12;
      ampm = 'a';
    }
    res = ustoa(s, ho, 2, ' '); /* %2u */
  } else {
    res = ustoa(s, ho, 2, '0'); /* %02u */
  }

  /* append separator and minutes */
  res += sv_strcpy(s + res, p->timesep);
  res += ustoa(s + res, mn, 2, '0'); /* %02u */

  /* if seconds provided, append them, too */
  if (sc != 0xff) {
    res += sv_strcpy(s + res, p->timesep);
    res += ustoa(s + res, sc, 2, '0'); /* %02u */
  }

  /* finally append the AM/PM char */
  if (ampm != 0) s[res++] = ampm;
  s[res] = 0;

  return(res);
}


/* divides an unsigned long by 10 using 16bit division */
static unsigned short divulong10(unsigned long *n) {
  unsigned short a, b;
  unsigned short rem;
  const long adj[10] = {-1, 6553, 13107, 19660, 26214, 32767, 39321, 45875, 52428, 58982};

  b = *n;
  *n >>= 16;
  a = *n;

  rem = ((a % 10) << 4) + (b % 10);
  rem %= 10;

  /* AB / 10 = (((A / 10) + ((A % 10) / 10)) << 16) + (B / 10) */

  *n = (a / 10);
  *n <<= 16;
  *n += b / 10;

  /* I have no clue about the math behind this, found out the cyclic relation
   * using brute force */
  *n += adj[a % 10];
  if ((b % 10) >= (((a % 10) * 4) % 10)) *n += 1;

  return(rem);
}


/* computes a formatted integer number based on NLS patterns found in p
 * returns length of result */
unsigned short nls_format_number(char *s, unsigned long num, const struct nls_patterns *p) {
  unsigned short sl = 0, i;
  unsigned char thcount = 0;

  /* write the value (reverse) with thousand separators (if any defined) */
  do {
    unsigned short rem;
    if ((thcount == 3) && (p->thousep[0] != 0)) {
      s[sl++] = p->thousep[0];
      thcount = 0;
    }
    rem = divulong10(&num);
    s[sl++] = '0' + rem;
    thcount++;
  } while (num > 0);

  /* terminate the string */
  s[sl] = 0;

  /* reverse the string now (has been built in reverse) */
  for (i = sl / 2 + (sl & 1); i < sl; i++) {
    thcount = s[i];
    s[i] = s[sl - (i + 1)];   /* abc'de  if i=3 then ' <-> c */
    s[sl - (i + 1)] = thcount;
  }

  return(sl);
}


/* capitalize an ASCIZ string following country-dependent rules */
void nls_strtoup(char *buff) {
  unsigned short errcode = 0;
  /* requires DOS 4+ */
  _asm {
    push ax
    push dx

    mov ax, 0x6522 /* country-dependent capitalize string (DOS 4+) */
    mov dx, buff   /* DS:DX -> string to capitalize */
    int 0x21
    jnc DONE

    mov errcode, ax /* set errcode on failure */
    DONE:

    pop dx
    pop ax
  }

  /* do a naive upcase if DOS has no NLS support */
  if (errcode != 0) {
    while (*buff) {
      if (*buff < 128) { /* apply only to 7bit (low ascii) values */
        *buff &= 0xDF;
      }
      buff++;
    }
  }
}


/* reload nls ressources from svarcom.lng into svarlang_mem and rmod */
void nls_langreload(char *buff, struct rmod_props far *rmod) {
  const char far *dosdir;
  const char far *lang;
  static unsigned short lastlang;
  unsigned short dosdirlen;
  unsigned short rmodenvseg = *(unsigned short far *)MK_FP(rmod->rmodseg, RMOD_OFFSET_ENVSEG);
  unsigned char far *rmodcritmsg = MK_FP(rmod->rmodseg, RMOD_OFFSET_CRITMSG);
  int i;

  /* supplied through DEFLANG.C */
  extern char svarlang_mem[];
  extern unsigned short svarlang_dict[];
  extern const unsigned short svarlang_memsz;
  extern const unsigned short svarlang_string_count;

  /* look up the LANG env variable and copy to lang */
  lang = env_lookup_val(rmodenvseg, "LANG");
  if ((lang == NULL) || (lang[0] == 0)) return;
  memcpy_ltr_far(buff, lang, 2);
  buff[2] = 0;

  /* check if there is need to reload at all */
  if (lastlang == *((unsigned short *)buff)) return;

  /* mark the new lang as current even though I do not know yet if I will
   * succeed. in case I fail, I do not want to retry reloading the lang block
   * after every command. https://github.com/SvarDOS/bugz/issues/12 */
  memcpy_ltr_far(&lastlang, lang, 2);

  /* load lng from rmod if it's there (/M) */
  if ((rmod->lng_segmem) && (rmod->lng_segdict) && (rmod->lng_current == *((unsigned short *)buff))) {
    rmod->lng_current = lastlang;
    memcpy_ltr_far(svarlang_mem, MK_FP(rmod->lng_segmem, 0), svarlang_memsz);
    memcpy_ltr_far(svarlang_dict, MK_FP(rmod->lng_segdict, 0), svarlang_string_count * 4);
    return;
  }

  buff[4] = 0;
  dosdir = env_lookup_val(rmodenvseg, "DOSDIR");
  if (dosdir == NULL) return;

  sv_strcpy_far(buff + 4, dosdir);
  dosdirlen = sv_strlen(buff + 4);
  if (buff[4 + dosdirlen - 1] == '\\') dosdirlen--;
  memcpy_ltr(buff + 4 + dosdirlen, "\\SVARCOM.LNG", 13);

  /* try loading %DOSDIR%\SVARCOM.LNG */
  if (svarlang_load(buff + 4, buff) != 0) {
    /* failed! try %DOSDIR%\BIN\SVARCOM.LNG */
    memcpy_ltr(buff + 4 + dosdirlen, "\\BIN\\SVARCOM.LNG", 17);
    if (svarlang_load(buff + 4, buff) != 0) return;
  }

  /* update RMOD's critical handler with new strings */
  for (i = 0; i < 9; i++) {
    int len;
    len = sv_strlen(svarlang_str(3, i));
    if (len > 15) len = 15;
    memcpy_ltr_far(rmodcritmsg + (i * 16), svarlang_str(3, i), len);
    memcpy_ltr_far(rmodcritmsg + (i * 16) + len, "$", 1);
  }
  /* The ARIF string is special: always 4 bytes long and no $ terminator */
  memcpy_ltr_far(rmodcritmsg + (9 * 16), svarlang_str(3,9), 4);

  /* if /M then back up the new lang into rmod memory */
  if ((rmod->lng_segmem) && (rmod->lng_segdict)) {
    rmod->lng_current = lastlang;
    memcpy_ltr_far(MK_FP(rmod->lng_segmem, 0), svarlang_mem, svarlang_memsz);
    memcpy_ltr_far(MK_FP(rmod->lng_segdict, 0), svarlang_dict, svarlang_string_count * 4);
  }
}


/* locates executable fname in path and fill res with result. returns 0 on success,
 * -1 on failed match and -2 on failed match + "don't even try with other paths"
 * extptr is filled with a ptr to the extension in fname (NULL if no extension) */
int lookup_cmd(char *res, const char *fname, const char *path, const char **extptr) {
  unsigned short lastbslash = 0;
  unsigned short i, len;
  unsigned char explicitpath = 0;
  const char *exec_ext[] = {"COM", "EXE", "BAT", NULL};

  /* does the original fname has an explicit path prefix or explicit ext? */
  *extptr = NULL;
  for (i = 0; fname[i] != 0; i++) {
    switch (fname[i]) {
      case ':':
      case '\\':
        explicitpath = 1;
        *extptr = NULL; /* extension is the last dot AFTER all path delimiters */
        break;
      case '.':
        *extptr = fname + i + 1;
        break;
    }
  }

  /* if explicit ext found, make sure it is executable */
  if (*extptr != NULL) {
    for (i = 0; exec_ext[i] != NULL; i++) if (imatch(*extptr, exec_ext[i])) break;
    if (exec_ext[i] == NULL) return(-2); /* bad extension - don't try running it ever */
  }

  /* normalize filename */
  if (file_truename(fname, res) != 0) return(-2);

  /* printf("truename: %s\r\n", res); */

  /* figure out where the command starts */
  for (len = 0; res[len] != 0; len++) {
    switch (res[len]) {
      case '?':   /* abort on any wildcard character */
      case '*':
        return(-2);
      case '\\':
        lastbslash = len;
        break;
    }
  }

  /* printf("lastbslash=%u\r\n", lastbslash); */

  /* if no path prefix was found in fname (no colon or backslash) AND we have
   * a path arg, then assemble path+filename */
  if ((!explicitpath) && (path != NULL) && (path[0] != 0)) {
    i = sv_strlen(path);
    if (path[i - 1] != '\\') i++; /* add a byte for inserting a bkslash after path */
    /* move the filename at the place where path will end */
    sv_memmove(res + i, res + lastbslash + 1, len - lastbslash);
    /* copy path in front of the filename and make sure there is a bkslash sep */
    memcpy_ltr(res, path, i);
    res[i - 1] = '\\';
  }

  /* if no extension was initially provided, try matching COM, EXE, BAT */
  if (*extptr == NULL) {
    int attr;
    len = sv_strlen(res);
    res[len++] = '.';
    for (i = 0; exec_ext[i] != NULL; i++) {
      sv_strcpy(res + len, exec_ext[i]);
      /* printf("? '%s'\r\n", res); */
      *extptr = exec_ext[i];
      attr = file_getattr(res);
      if (attr < 0) continue; /* file not found */
      if (attr & DOS_ATTR_DIR) continue; /* this is a directory */
      if (attr & DOS_ATTR_VOL) continue; /* this is a volume */
      return(0);
    }
  } else { /* try finding it as-is */
    /* printf("? '%s'\r\n", res); */
    int attr = file_getattr(res);
    if ((attr >= 0) &&  /* file exists */
        ((attr & DOS_ATTR_DIR) == 0) && /* is not a directory */
        ((attr & DOS_ATTR_VOL) == 0)) { /* is not a volume */
      return(0);
    }
  }

  /* not found */
  if (explicitpath) return(-2); /* don't bother trying other paths, the caller had its own path preset anyway */
  return(-1);
}


/* fills fname with the path and filename to the linkfile related to the
 * executable link "linkname". returns 0 on success. */
int link_computefname(char *fname, const char *linkname, unsigned short env_seg) {
  unsigned short pathlen, doserr = 0;

  /* fetch %DOSDIR% */
  pathlen = env_lookup_valcopy(fname, 128, env_seg, "DOSDIR");
  if (pathlen == 0) return(-1);

  /* prep filename: %DOSDIR%\LINKS\PKG.LNK */
  if (fname[pathlen - 1] == '\\') pathlen--;
  pathlen += sv_strcpy(fname + pathlen, "\\LINKS");
  /* create \LINKS if not exists */
  if (file_getattr(fname) < 0) {
    _asm {
      push dx
      mov ah, 0x39
      mov dx, fname
      int 0x21
      jnc DONE
      mov doserr, ax
      DONE:
      pop dx
    }
    if (doserr) {
      output(fname);
      output(" - ");
      nls_outputnl(255, doserr);
      return(-1);
    }
  }
  /* quit early if dir does not exist (or is not a dir) */
  if (file_getattr(fname) != DOS_ATTR_DIR) {
    output(fname);
    output(" - ");
    nls_outputnl(255,3); /* path not found */
    return(-1);
  }
  sv_strcat(fname + pathlen, "\\");
  sv_strcat(fname + pathlen, linkname);
  sv_strcat(fname + pathlen, ".LNK");

  return(0);
}


/* like memcpy() but guarantees to copy from left to right */
void memcpy_ltr(void *d, const void *s, unsigned short len) {
  unsigned char const *ss = s;
  unsigned char *dd = d;

  while (len--) {
    *dd = *ss;
    ss++;
    dd++;
  }
}


/* like memcpy_ltr() but operates on far pointers */
void memcpy_ltr_far(void far *d, const void far *s, unsigned short len) {
  unsigned char const far *ss = s;
  unsigned char far *dd = d;

  while (len--) {
    *dd = *ss;
    ss++;
    dd++;
  }
}


/* like memcpy() but guarantees to copy from right to left */
void memcpy_rtl(void *d, const void *s, unsigned short len) {
  unsigned char const *ss = s;
  unsigned char *dd = d;

  dd += len - 1;
  ss += len - 1;
  while (len--) {
    *dd = *ss;
    ss--;
    dd--;
  }
}


void sv_memmove(void *dst, const void *src, unsigned short len) {
  if (dst < src) {
    memcpy_ltr(dst, src, len);
  } else {
    memcpy_rtl(dst, src, len);
  }
}


/* like bzero(), but accepts far pointers */
void sv_bzero(void far *dst, unsigned short len) {
  char far *d = dst;
  while (len--) {
    *d = 0;
    d++;
  }
}


/* like memset() */
void sv_memset(void *dst, unsigned char c, unsigned short len) {
  unsigned char *d = dst;
  while (len--) {
    *d = c;
    d++;
  }
}


/* replaces characters a by b in s */
void sv_strtr(char *s, char a, char b) {
  while (*s) {
    if (*s == a) *s = b;
    s++;
  }
}


/* inserts string s2 into s1 in place of the first % character */
void sv_insert_str_in_str(char *s1, const char *s2) {
  unsigned short s2len;

  /* fast forward s1 to either % or 0 */
  while ((*s1 != 0) && (*s1 != '%')) s1++;

  /* if not % then quit */
  if (*s1 != '%') return;

  /* make room for s2, unless s2 is exactly 1 byte long */
  s2len = sv_strlen(s2);
  if (s2len == 0) {
    memcpy_ltr(s1, s1 + 1, sv_strlen(s1 + 1) + 1);
  } else if (s2len > 1) {
    memcpy_rtl(s1 + s2len, s1 + 1, sv_strlen(s1 + 1) + 1);
  }

  /* write s2 to the cleared space */
  if (s2len != 0) memcpy_ltr(s1, s2, s2len);
}


/* allocates a block of paras paragraphs, as high as possible
 * returns segment of allocated block on success, 0 on failure */
unsigned short alloc_high_seg(unsigned short paras) {
  unsigned short res = 0;

  _asm {
    push bx
    push cx
    push dx

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

    /* ask for a memory block */
    mov ah, 0x48
    mov bx, paras
    int 0x21
    jc ALLOC_FAIL
    mov res, ax

    ALLOC_FAIL:
    /* restore initial allocation strategy */
    mov ax, 0x5801
    mov bx, dx
    int 0x21
    /* restore initial UMB memory link state */
    mov ax, 0x5803
    pop bx       /* pop initial UMB link state from stack */
    int 0x21

    pop dx
    pop cx
    pop bx
  }

  return(res);
}


/* free previously allocated memory block at segment segm */
void freeseg(unsigned short segm) {
  _asm {
    push es

    mov ah, 0x49
    mov es, segm
    int 0x21

    pop es
  }
}
