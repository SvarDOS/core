/*
 * This file is part of the pkg (SvarDOS) project.
 * Copyright (C) Mateusz Viste 2012-2024
 */

#include <stdio.h>
#include <string.h>    /* strlen() */
#include <stdlib.h>    /* free() */

#include "crc32.h"
#include "helpers.h"
#include "kprintf.h"
#include "svarlang.lib\svarlang.h"

#include "healthck.h"


/* converts a CRC32 into a (hex) string */
static char *crc32tostring(char *s, unsigned long val) {
  signed char i;
  static char h[] = "0123456789ABCDEF";
  for (i = 7; i >= 0; i--) {
    s[i] = h[val & 15];
    val >>= 4;
  }
  s[8] = 0;
  return(s);
}


/* checks the health of a package (or all packages).
 * Returns 0 on success, non-zero otherwise */
int healthcheck(const char *pkgname, const char *dosdir) {
  char buff[1024];
  char *crc, *ext;
  FILE *flist, *fd;
  unsigned long goodcrc, realcrc;
  unsigned short errcount = 0;

  if (pkgname == NULL) {
    outputnl("system-wide healthcheck not implemented yet");
    return(-1);
  }

  /* open the (legacy) listing file at %DOSDIR%\packages\pkgname.lst
   * if not exists then fall back to appinfo\pkgname.lsm */
  sprintf(buff, "%s\\appinfo\\%s.lsm", dosdir, pkgname);
  flist = fopen(buff, "rb");
  if (flist == NULL) {
    kitten_printf(4, 0, pkgname); /* "Package %s is not installed, so not removed." */
    puts("");
    return(-1);
  }

  /* iterate over all files listed in pkgname.lsm */
  while (freadtokval(flist, buff, sizeof(buff), NULL, 0) == 0) {

    /* skip empty lines */
    if (buff[0] == 0) continue;

    /* change all slash to backslash */
    slash2backslash(buff);

    /* skip garbage */
    if ((buff[1] != ':') || (buff[2] != '\\')) continue;

    /* trim out CRC information and get the ptr to it (if present) */
    crc = trimfnamecrc(buff);
    if (crc == NULL) continue;

    goodcrc = strtoul(crc, NULL, 16);
    ext = getfext(buff);
    strlwr(ext);

    /* TODO skip non-executable files */

    output("[");
    output(pkgname);
    output("] ");
    output(buff);

    realcrc = CRC32_INITVAL;
    fd = fopen(buff, "rb");
    if (fd == NULL) {
      outputnl(": NOT FOUND");
      continue;
    }
    for (;;) {
      unsigned short bufflen;
      bufflen = fread(buff, 1, sizeof(buff), fd);
      if (bufflen == 0) break;
      crc32_feed(&realcrc, buff, bufflen);
    }
    fclose(fd);

    crc32_finish(&realcrc);

    if (goodcrc != realcrc) {
      sprintf(buff, ": %s", "BAD CRC", crc32tostring(buff + 128, realcrc), crc32tostring(buff + 160, goodcrc));
      outputnl(buff);
      errcount++;
      continue;
    }

    output("\r                                   \r");

  }

  /* close the lsm file */
  fclose(flist);

  if (errcount == 0) {
    outputnl(svarlang_strid(0x0A00));
  } else {
    sprintf(buff, "%u errors.", errcount);
    outputnl(buff);
  }
  return(0);
}
