/*
 * This file is part of pkg (SvarDOS)
 * Copyright (C) 2013-2024 Mateusz Viste
 */

#include <stdio.h>   /* fopen, fclose... */
#include <string.h>  /* strcasecmp() */

#include "helpers.h"

#include "lsm.h"     /* include self for control */


/* Loads metadata from an LSM file. Returns 0 on success, non-zero on error. */
int readlsm(const char *filename, const char *field, char *result, size_t result_maxlen) {
  char linebuff[128];
  char *valuestr;
  FILE *fd;

  /* open the file */
  fd = fopen(filename, "rb");
  result[0] = 0; /* reset fields to be read to empty values (now in case it is the same ptr as filename) */
  if (fd == NULL) return(-1);

  /* read the LSM file line by line */
  while (freadtokval(fd, linebuff, sizeof(linebuff), &valuestr, ':') == 0) {
    if (valuestr == NULL) continue;
    if (strcasecmp(linebuff, field) == 0) {
      snprintf(result, result_maxlen, "%s", valuestr);
      break;
    }
  }
  fclose(fd);
  return(0);
}
