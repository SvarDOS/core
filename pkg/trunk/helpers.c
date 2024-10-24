/*
 * This file is part of pkg (SvarDOS package manager)
 * Copyright (C) 2012-2024 Mateusz Viste
 *
 * It contains a few helper function...
 */


#include <ctype.h>    /* tolower() */
#include <direct.h>   /* provides the mkdir() prototype */
#include <string.h>   /* */
#include <strings.h>  /* strcasecmp() */
#include <stdio.h>    /* sprintf() */
#include <stdlib.h>   /* atoi() */
#include <sys/stat.h> /* mkdir() */

#include "trim.h"
#include "helpers.h"


/* converts a CRC32 into a (hex) string */
char *crc32tostring(char *s, unsigned long val) {
  signed char i;
  static char h[] = "0123456789ABCDEF";
  for (i = 7; i >= 0; i--) {
    s[i] = h[val & 15];
    val >>= 4;
  }
  s[8] = 0;
  return(s);
}


/* outputs a NUL-terminated string to stdout */
void output(const char *s) {
  _asm {
    push ds
    push es
    /* get length of s into CX */
    mov ax, 0x4000 /* ah=DOS "write to file" and AL=0 for NUL matching */
    lds dx, s      /* set DS:DX to string (required for later) */
    push ds
    pop es         /* make sure es=ds (scasb uses es) */
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
    mov bl, 1 /* set handle (1=stdout 2=stderr) */
    /* mov cx, xxx */ /* write CX bytes */
    /* mov dx, s   */ /* DS:DX is the source of bytes to "write" */
    int 0x21
    WRITEDONE:
    pop es
    pop ds
  }
}


void outputnl(const char *s) {
  output(s);
  output("\r\n");
}


/* change all / to \ in a string */
void slash2backslash(char *str) {
  int x;
  for (x = 0; str[x] != 0; x++) {
    if (str[x] == '/') str[x] = '\\';
  }
}


/* trim CRC from a filename and returns a pointer to the CRC part.
 * this is used to parse filename lines from LSM files */
char *trimfnamecrc(char *fname) {
  while (*fname) {
    if (*fname == '?') {
      *fname = 0;
      return(fname + 1);
    }
    fname++;
  }
  return(NULL);
}


void removeDoubleBackslashes(char *str) {
  char *curpos;
  int x;
  for (;;) {
    curpos = strstr(str, "\\\\");
    if (curpos == NULL) return; /* job done */
    for (x = 1; curpos[x] != 0; x++) {
      curpos[x - 1] = curpos[x];
    }
    curpos[x - 1] = 0;
  }
}


/* Find the first occurrence of find in s, ignore case. */
char *fdnpkg_strcasestr(const char *s, const char *find) {
  char c, sc;
  size_t len;
  if ((c = *find++) != 0) {
    c = tolower((unsigned char)c);
    len = strlen(find);
    do {
      do {
        if ((sc = *s++) == 0) return(NULL);
      } while ((char)tolower((unsigned char)sc) != c);
    } while (strncasecmp(s, find, len) != 0);
    s--;
  }
  return((char *)s);
}


/* Creates directories recursively */
void mkpath(char *dirs) {
  int x;
  char savechar;
  for (x = 0; dirs[x] != 0; x++) {
    if (((dirs[x] == '/') || (dirs[x] == '\\')) && (x > 0)) {
      if (dirs[x - 1] != ':') { /* avoid d:\ stuff */
        savechar = dirs[x];
        dirs[x] = 0;
        /* make the dir */
        mkdir(dirs);
        dirs[x] = savechar;
      }
    }
  }
}


/* returns a pointer to the start of the filename, out of a path\to\file string, and
   fills respath with the local folder where the file should be placed. */
char *computelocalpath(char *longfilename, char *respath, const char *dosdir, const struct customdirs *dirlist, char bootdrive) {
  int x, lastsep = 0, firstsep = -1;
  char savedchar;
  char *shortfilename, *pathstart;
  pathstart = longfilename;
  for (x = 0; longfilename[x] != 0; x++) {
    if ((longfilename[x] == '/') || (longfilename[x] == '\\')) {
      lastsep = x;
      if (firstsep < 0) firstsep = x;
    }
  }
  /* if it's a file without any directory, then it goes to BOOTDRIVE:\ (COMMAND.COM, KERNEL.SYS...) */
  if (firstsep < 0) {
    sprintf(respath, "%c:\\", bootdrive);
    return(longfilename);
  }
  /* */
  shortfilename = &longfilename[lastsep + 1];
  /* look for possible custom path */
  if (firstsep > 0) {
    savedchar = longfilename[firstsep];
    longfilename[firstsep] = 0;
    for (; dirlist != NULL; dirlist = dirlist->next) {
      if (fdnpkg_strcasestr(longfilename, dirlist->name) == longfilename) { /* found! */
        /* sprintf(respath, "%s\\%s", dirlist->location, &longfilename[firstsep + 1]); */
        pathstart = &longfilename[firstsep + 1];
        dosdir = dirlist->location;
        break;
      }
    }
    longfilename[firstsep] = savedchar; /* restore longfilename as it was */
  }
  /* apply the default (DOSDIR) path */
  savedchar = longfilename[lastsep + 1];
  longfilename[lastsep + 1] = 0;
  sprintf(respath, "%s\\%s", dosdir, pathstart);
  slash2backslash(respath);
  removeDoubleBackslashes(respath);
  longfilename[lastsep + 1] = savedchar;
  return(shortfilename);
}


/* analyzes a filename string and returns the pointer to the file's extension
 * (which can be empty) */
char *getfext(char *fname) {
  char *res = NULL;
  for (; *fname != 0; fname++) {
    if (*fname == '.') res = fname + 1;
  }
  /* if no dot found, then point to the string's null terminator */
  if (res == NULL) return(fname);
  return(res);
}


/* reads a line from a "token = value" file, returns 0 on success
 * val (if not NULL) is updated with a pointer to the "value" part
 * delim is the delimiter char (typically ':' or '=' but can be anything) */
int freadtokval(FILE *fd, char *line, size_t maxlen, char **val, char delim) {
  int bytebuff, linelen = 0;
  if (val != NULL) *val = NULL;
  for (;;) {
    bytebuff = fgetc(fd);
    if (bytebuff == EOF) {
      if (linelen == 0) return(-1);
      break;
    }
    if (bytebuff < 0) return(-1);
    if ((*val == NULL) && (bytebuff == delim)) {
      line[linelen++] = 0;
      *val = line + linelen;
      continue;
    }
    if (bytebuff == '\r') continue; /* ignore CR */
    if (bytebuff == '\n') break;
    if (linelen < maxlen - 1) line[linelen++] = bytebuff;
  }
  /* terminate line and trim token and value (if any) */
  line[linelen] = 0;
  trim(line);
  if ((val != NULL) && (*val != NULL)) trim(*val);
  return(0);
}
