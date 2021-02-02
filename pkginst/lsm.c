/*
 * This file is part of pkginst
 * Copyright (C) 2013-2021 Mateusz Viste, All rights reserved.
 */

#include <stdio.h>   /* fopen, fclose... */
#include <string.h>  /* strcasecmp() */

#include "helpers.h"

#include "lsm.h"     /* include self for control */


/* Loads metadata from an LSM file. Returns 0 on success, non-zero on error. */
int readlsm(const char *filename, char *version, size_t version_maxlen) {
  char linebuff[128];
  char *valuestr;
  FILE *fd;
  /* reset fields to be read to empty values */
  version[0] = 0;
  /* open the file */
  fd = fopen(filename, "rb");
  if (fd == NULL) return(-1);
  /* read the LSM file line by line */

  while (freadtokval(fd, linebuff, sizeof(linebuff), &valuestr, ':') == 0) {
    if (valuestr != NULL) {
      if (strcasecmp(linebuff, "version") == 0) snprintf(version, version_maxlen, "%s", valuestr);
    }
  }
  fclose(fd);
  return(0);
}
