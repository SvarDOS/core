/*
 * This file is part of the pkg (SvarDOS) project.
 * Copyright (C) Mateusz Viste 2012-2024
 */

#include <direct.h> /* opendir() and friends */
#include <stdio.h>
#include <string.h>    /* strlen() */
#include <stdlib.h>    /* free() */

#include "crc32.h"
#include "helpers.h"
#include "kprintf.h"
#include "svarlang.lib\svarlang.h"

#include "healthck.h"


static void clrcurline(unsigned short screenwidth);
#pragma aux clrcurline = \
"mov ah, 0x02" \
"mov dl, 0x0D" \
"int 0x21" \
"mov ax, 0x0A20" /* int 10h "write char to screen" */ \
"xor bx, bx"     /* video page and attribute (attr on PCjr) */ \
"int 0x10" \
parm [cx] \
modify [ax bx dx]


/* checks the health of a package (or all packages).
 * Returns 0 on success, non-zero otherwise */
int healthcheck(unsigned char *buff4k, const char *pkgname, const char *dosdir, unsigned char extendedcheck) {
  char *crc, *ext;
  FILE *flist, *fd;
  unsigned long goodcrc, realcrc;
  unsigned short errcount = 0;
  DIR *dp = NULL;
  struct dirent *ep;

  if (pkgname == NULL) {
    sprintf(buff4k, "%s\\appinfo", dosdir);
    dp = opendir(buff4k);
    if (dp == NULL) {
      kitten_printf(9, 0, buff4k); /* "ERROR: Could not access directory %s" */
      outputnl("");
      return(-1);
    }

    AGAIN:

    /* if check is system-wide then fetch next package */
    while ((ep = readdir(dp)) != NULL) { /* readdir() result must never be freed (static allocation) */
      int tlen = strlen(ep->d_name);
      if (ep->d_name[0] == '.') continue; /* ignore '.', '..', and hidden directories */
      if (tlen < 4) continue; /* files must be at least 5 bytes long ("x.lst") */
      if (strcasecmp(ep->d_name + tlen - 4, ".lsm") != 0) continue;  /* if not an .lsm file, skip it silently */
      ep->d_name[tlen - 4] = 0; /* trim out the ".lsm" suffix */
      break;
    }
    if (ep == NULL) {
      closedir(dp);
      goto DONE;
    }
    pkgname = ep->d_name;
  }


  /************************************************************
   * pkgname is valid now so let's proceed with serious stuff *
   ************************************************************/

  /* open the (legacy) listing file at %DOSDIR%\packages\pkgname.lst
   * if not exists then fall back to appinfo\pkgname.lsm */
  sprintf(buff4k, "%s\\appinfo\\%s.lsm", dosdir, pkgname);
  flist = fopen(buff4k, "rb");
  if (flist == NULL) {
    kitten_printf(4, 0, pkgname); /* "Package %s is not installed, so not removed." */
    outputnl("");
    return(-1);
  }

  /* iterate over all files listed in pkgname.lsm */
  while (freadtokval(flist, buff4k, 4096, NULL, 0) == 0) {

    /* skip empty lines */
    if (buff4k[0] == 0) continue;

    /* change all slash to backslash */
    slash2backslash(buff4k);

    /* skip garbage */
    if ((buff4k[1] != ':') || (buff4k[2] != '\\')) continue;

    /* trim out CRC information and get the ptr to it (if present) */
    crc = trimfnamecrc(buff4k);
    if (crc == NULL) continue;

    goodcrc = strtoul(crc, NULL, 16);
    ext = getfext(buff4k);
    strlwr(ext);

    /* skip non-executable files (unless healthcheck+) */
    if ((extendedcheck == 0) &&
        (strcmp(ext, "bat") != 0) &&
        (strcmp(ext, "bin") != 0) &&
        (strcmp(ext, "com") != 0) &&
        (strcmp(ext, "dll") != 0) &&
        (strcmp(ext, "drv") != 0) &&
        (strcmp(ext, "exe") != 0) &&
        (strcmp(ext, "ovl") != 0) &&
        (strcmp(ext, "sys") != 0)) continue;

    output("[");
    output(pkgname);
    output("] ");
    output(buff4k);

    realcrc = CRC32_INITVAL;
    fd = fopen(buff4k, "rb");
    if (fd == NULL) {
      outputnl(": NOT FOUND");
      continue;
    }
    for (;;) {
      unsigned short bufflen;
      bufflen = fread(buff4k, 1, 4096, fd);
      if (bufflen == 0) break;
      crc32_feed(&realcrc, buff4k, bufflen);
    }
    fclose(fd);

    crc32_finish(&realcrc);

    if (goodcrc != realcrc) {
      sprintf(buff4k, ": %s", "BAD CRC");
      outputnl(buff4k);
      errcount++;
      continue;
    }

    clrcurline(80);

  }

  /* close the lsm file */
  fclose(flist);

  if (dp != NULL) goto AGAIN;

  DONE:

  if (errcount == 0) {
    outputnl(svarlang_strid(0x0A00));
  } else {
    sprintf(buff4k, "%u errors.", errcount);
    outputnl(buff4k);
  }
  return(0);
}
