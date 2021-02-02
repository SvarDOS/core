/*
 * This file is part of pkginst
 * Copyright (C) 2012-2021 Mateusz Viste
 *
 * It contains a few helper function...
 */


#include <ctype.h>    /* tolower() */
#include <direct.h>   /* provides the mkdir() prototype */
#include <string.h>   /* */
#include <stdio.h>    /* sprintf() */
#include <stdlib.h>   /* atoi() */
#include <sys/stat.h> /* mkdir() */

#include "rtrim.h"
#include "helpers.h"


/* change all / to \ in a string */
void slash2backslash(char *str) {
  int x;
  for (x = 0; str[x] != 0; x++) {
    if (str[x] == '/') str[x] = '\\';
  }
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
        mkdir(dirs);
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
