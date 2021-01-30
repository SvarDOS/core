/*
 * This file is part of pkginst (SvarDOS).
 *
 * Reads environment variables that will be used by pkginst.
 * Returns 0 on success, non-zero otherwise.
 *
 * Copyright (C) 2012-2021 Mateusz Viste
 */

#include <stdio.h>    /* snprintf() */
#include <stdlib.h>   /* getenv() */
#include "kprintf.h"  /* kprintf(), kputs() */

#include "readenv.h"


int readenv(char **dosdir, char *cfgfile, int cfgfilemaxlen) {
  char *cfg;

  /* check if %DOSDIR% is set, and retrieve it */
  *dosdir = getenv("DOSDIR");
  if (*dosdir == NULL) {
    kitten_puts(2, 2, "%DOSDIR% not set! You should make it point to the FreeDOS main directory.");
    kitten_puts(2, 3, "Example: SET DOSDIR=C:\\FDOS");
    return(-1);
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
