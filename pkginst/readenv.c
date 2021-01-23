/*
 * This file is part of FDNPKG.
 *
 * Reads environment variables that will be used by FDNPKG and FDINST.
 * Returns 0 on success, non-zero otherwise.
 *
 * Copyright (C) 2012-2016 Mateusz Viste
 */

#include <stdio.h>    /* snprintf() */
#include <stdlib.h>   /* getenv() */
#include "kprintf.h"  /* kprintf(), kputs() */

#include "readenv.h"


int readenv(char **dosdir, char **tempdir, char *cfgfile, int cfgfilemaxlen) {
  char *cfg;

  /* check if %DOSDIR% is set, and retrieve it */
  *dosdir = getenv("DOSDIR");
  if (*dosdir == NULL) {
    kitten_puts(2, 2, "%DOSDIR% not set! You should make it point to the FreeDOS main directory.");
    kitten_puts(2, 3, "Example: SET DOSDIR=C:\\FDOS");
    return(-1);
  }

  /* check if %TEMP% is set, and retrieve it */
  *tempdir = getenv("TEMP");
  if (*tempdir == NULL) {
    kitten_puts(2, 0, "%TEMP% not set! You should make it point to a writeable directory.");
    kitten_puts(2, 1, "Example: SET TEMP=C:\\TEMP");
    return(-2);
  }

  /* look for the FDNPKG.CFG env. variable */
  cfg = getenv("FDNPKG.CFG");
  cfgfilemaxlen -= 1; /* make room for the null terminator */
  if (cfg != NULL) {
    snprintf(cfgfile, cfgfilemaxlen, "%s", cfg);
  } else { /* not set, so fallback to hardcoded location */
    snprintf(cfgfile, cfgfilemaxlen, "%s\\bin\\fdnpkg.cfg", *dosdir);
  }

  return(0);
}
