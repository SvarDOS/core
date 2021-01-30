/*
 * This file is part of pkginst
 * Copyright (C) 2012-2021 Mateusz Viste
 *
 * It contains a few helper function...
 */


#include <ctype.h>    /* tolower() */
#include <string.h>   /* */
#include <stdio.h>    /* sprintf() */
#include <stdlib.h>   /* atoi() */
#include <sys/stat.h> /* mkdir() */

#include "version.h"

#include <direct.h>  /* provides the mkdir() prototype */
#define MAKEDIR(x) mkdir(x);

#include "helpers.h"


/* translates a version string into a array of integer values. The array must be 8-position long.
   returns 0 if parsing was successful, non-zero otherwise.
   Accepted formats follow:
    300.12.1
    1
    12.2.34.2-4.5
    1.2c
    1.01
    2013-12-31   */
static int versiontointarr(char *verstr, int *arr) {
  int i, vlen, dotcounter = 1, firstcharaftersep = 0;
  char *digits[8];
  char verstrcopy[16];

  /* fill the array with zeroes first */
  for (i = 0; i < 8; i++) arr[i] = 0;

  /* first extensively validate the input */
  if (verstr == NULL) return(-1);
  vlen = strlen(verstr);
  if (vlen == 0) return(-1);
  if (vlen > 15) return(-1);
  if ((verstr[0] < '0') || (verstr[0] > '9')) return(-1); /* a version string must start with a 0..9 digit */
  if ((tolower(verstr[vlen - 1]) >= 'a') && (tolower(verstr[vlen - 1]) <= 'z') && (vlen > 1)) { /* remove any letter from the end, and use it as a (very) minor differenciator */
    vlen -= 1;
    arr[7] = tolower(verstr[vlen]);
  }
  if ((verstr[vlen - 1] < '0') || (verstr[vlen - 1] > '9')) return(-1); /* a version string must end with a 0..9 digit */

  digits[0] = verstrcopy;
  for (i = 0; i < vlen; i++) {
    verstrcopy[i] = verstr[i];
    switch (verstr[i]) {
      case '.':
      case '-':
        if (i == 0) return(-1);
        if (verstrcopy[i-1] == 0) return(-1); /* do not allow two separators in a row */
        if (dotcounter > 6) return(-1);
        digits[dotcounter++] = &verstrcopy[i + 1];
        verstrcopy[i] = 0;
        firstcharaftersep = 1;
        break;
      case '0':
        /* if this is a zero right after a separator, and trailed with a digit
         * (as in '1.01'), then enforce a separator first */
        if ((firstcharaftersep != 0) && (verstr[i+1] >= '0') && (verstr[i+1] <= '9')) {
          if (dotcounter > 6) return(-1);
          digits[dotcounter++] = &verstrcopy[i + 1];
          verstrcopy[i] = 0;
        }
        break;
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
        firstcharaftersep = 0;
        break;
      default: /* do not allow any character different than 0..9 or '.' */
        return(-1);
        break;
    }
  }
  verstrcopy[i] = 0;
  /* now that we know the input is sane, let's process it */
  for (i = 0; i < dotcounter; i++) {
    int tmpint;
    tmpint = atoi(digits[i]);
    if ((tmpint < 0) || (tmpint > 32000)) return(-1);
    arr[i] = tmpint;
  }
  return(0);
}


/* compares version strings v1 and v2. Returns 1 if v2 is newer than v1, 0 otherwise, and -1 if comparison is unable to tell */
int isversionnewer(char *v1, char *v2) {
  int x1[8], x2[8], i;
  if ((v1 == NULL) || (v2 == NULL)) return(-1); /* if input is invalid (NULL), don't continue */
  if (strcasecmp(v1, v2) == 0) return(0);  /* if both versions are the same, don't continue */
  /* check versions of the decimal format 1.23... */
  if ((versiontointarr(v1, x1) != 0) || (versiontointarr(v2, x2) != 0)) return(-1);
  for (i = 0; i < 8; i++) {
    if (x2[i] > x1[i]) return(1);
    if (x2[i] < x1[i]) return(0);
  }
  return(0);
}


/* change all / to \ in a string */
void slash2backslash(char *str) {
  int x;
  for (x = 0; str[x] != 0; x++) {
    if (str[x] == '/') str[x] = '\\';
  }
}


/* change all \ to / in a string */
void backslash2slash(char *str) {
  int x;
  for (x = 0; str[x] != 0; x++) {
    if (str[x] == '\\') str[x] = '/';
  }
}


void removeDoubleBackslashes(char *str) {
  char *curpos;
  int x;
  for (;;) {
    curpos = fdnpkg_strcasestr(str, "\\\\");
    if (curpos == NULL) return; /* job done */
    for (x = 1; curpos[x] != 0; x++) {
      curpos[x - 1] = curpos[x];
    }
    curpos[x - 1] = 0;
  }
}


/* converts a string to all lowercase */
void strtolower(char *mystring) {
  int x;
  for (x = 0; mystring[x] != 0; x++) mystring[x] = tolower(mystring[x]);
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
        MAKEDIR(dirs);
        dirs[x] = savechar;
      }
    }
  }
}


/* returns a pointer to the start of the filename, out of a path\to\file string, and
   fills respath with the local folder where the file should be placed. */
char *computelocalpath(char *longfilename, char *respath, const char *dosdir, const struct customdirs *dirlist) {
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


/* detect local paths (eg. C:\REPO). Returns 1 if the url looks like a local path, zero otherwise. */
int detect_localpath(char *url) {
  if (url[0] != 0) {
    if (url[1] != 0) {
      if ((url[1] == ':') && ((url[2] == '\\') || (url[2] == '/'))) return(1);
    }
  }
  return(0);
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
