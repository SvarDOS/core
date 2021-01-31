/*
 * This file is part of FDNPKG
 * Copyright (C) 2013-2016 Mateusz Viste, All rights reserved.
 */

#include <stdio.h>   /* fopen, fclose... */
#include <string.h>  /* strcasecmp() */

#include "lsm.h"     /* include self for control */
#include "rtrim.h"
#include "version.h"

/* reads a line from a file descriptor, and writes it to *line, the *line array is filled no more than maxlen bytes. returns the number of byte read on success, or a negative value on failure (reaching EOF is considered an error condition) */
static int readline_fromfile(FILE *fd, char *line, int maxlen) {
  int bytebuff, linelen = 0;
  for (;;) {
    bytebuff = fgetc(fd);
    if (bytebuff == EOF) {
      line[linelen] = 0;
      if (linelen == 0) return(-1);
      return(linelen);
    }
    if (bytebuff < 0) return(-1);
    if (bytebuff == '\r') continue; /* ignore CR */
    if (bytebuff == '\n') {
      line[linelen] = 0;
      return(linelen);
    }
    if (linelen < maxlen) line[linelen++] = bytebuff;
  }
}


/* Loads metadata from an LSM file. Returns 0 on success, non-zero on error. */
int readlsm(const char *filename, char *version, int version_maxlen) {
  char linebuff[128];
  char *valuestr;
  int x;
  FILE *fd;
  /* reset fields to be read to empty values */
  version[0] = 0;
  /* open the file */
  fd = fopen(filename, "rb");
  if (fd == NULL) return(-1);
  /* read the LSM file line by line */
  while (readline_fromfile(fd, linebuff, sizeof(linebuff) - 1) >= 0) {
    for (x = 0;; x++) {
      if (linebuff[x] == 0) {
        x = -1;
        break;
      } else if (linebuff[x] == ':') {
        break;
      }
    }
    if (x > 0) {
      linebuff[x] = 0;
      valuestr = linebuff + x + 1;
      trim(linebuff);
      trim(valuestr);
      if (strcasecmp(linebuff, "version") == 0) snprintf(version, version_maxlen, "%s", valuestr);
    }
  }
  fclose(fd);
  return(0);
}
