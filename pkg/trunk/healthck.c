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
int healthcheck(unsigned char *buff15k, const char *pkgname, const char *dosdir, unsigned char extendedcheck) {
  char *crc, *ext;
  FILE *flist, *fd;
  unsigned long goodcrc, realcrc;
  unsigned short errcount = 0;
  DIR *dp = NULL;
  struct dirent *ep;

  if (pkgname == NULL) {
    sprintf(buff15k, "%s\\appinfo", dosdir);
    dp = opendir(buff15k);
    if (dp == NULL) {
      output(svarlang_str(9, 0)); /* "ERROR: Could not access directory:" */
      outputnl(buff15k);
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
  sprintf(buff15k, "%s\\appinfo\\%s.lsm", dosdir, pkgname);
  flist = fopen(buff15k, "rb");
  if (flist == NULL) {
    sprintf(buff15k, svarlang_str(4,0), pkgname); /* "Package %s is not installed, so not removed." */
    outputnl(buff15k);
    return(-1);
  }

  /* iterate over all files listed in pkgname.lsm */
  while (freadtokval(flist, buff15k, 15 * 1024, NULL, 0) == 0) {

    /* skip empty lines */
    if (buff15k[0] == 0) continue;

    /* change all slash to backslash */
    slash2backslash(buff15k);

    /* skip garbage */
    if ((buff15k[1] != ':') || (buff15k[2] != '\\')) continue;

    /* trim out CRC information and get the ptr to it (if present) */
    crc = trimfnamecrc(buff15k);
    if (crc == NULL) continue;

    goodcrc = strtoul(crc, NULL, 16);
    strlwr(buff15k); /* turn filename lower case - this is needed for aesthetics
    when printing errors, but also for matching extensions */
    ext = getfext(buff15k);

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
    output(buff15k);

    realcrc = CRC32_INITVAL;
    fd = fopen(buff15k, "rb");
    if (fd == NULL) {
      output(" ");
      outputnl(svarlang_str(11,1));
      continue;
    }
    for (;;) {
      unsigned short bufflen;
      bufflen = fread(buff15k, 1, 15 * 1024, fd);
      if (bufflen == 0) break;
      crc32_feed(&realcrc, buff15k, bufflen);
    }
    fclose(fd);

    crc32_finish(&realcrc);

    if (goodcrc != realcrc) {
      output(" ");
      outputnl(svarlang_str(11,0)); /* BAD CRC */
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
    sprintf(buff15k, svarlang_str(11,2), errcount);
    outputnl(buff15k);
  }
  return(0);
}
