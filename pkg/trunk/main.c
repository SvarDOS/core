/*
 * PKG - SvarDOS package manager
 *
 * PUBLISHED UNDER THE TERMS OF THE MIT LICENSE
 *
 * COPYRIGHT (C) 2016-2024 MATEUSZ VISTE, ALL RIGHTS RESERVED.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */


#include <stdio.h>    /* FILE */
#include <stdlib.h>   /* malloc() and friends */
#include <string.h>
#include <strings.h>  /* strcasecmp() */

#include "svarlang.lib/svarlang.h"
#include "healthck.h"
#include "crc32.h"
#include "helpers.h"
#include "libunzip.h"
#include "pkginst.h"
#include "pkgrem.h"
#include "showinst.h"
#include "unzip.h"
#include "version.h"


enum ACTIONTYPES {
  ACTION_INSTALL,
  ACTION_REMOVE,
  ACTION_LISTFILES,
  ACTION_LISTLOCAL,
  ACTION_HEALTHCHECK,
  ACTION_UNZIP,
  ACTION_CRC32,
  ACTION_HELP
};


static int showhelp(void) {
  outputnl("PKG ver " PVER " Copyright (C) " PDATE " Mateusz Viste");
  outputnl("");
  outputnl(svarlang_str(1, 0)); /* "PKG is the SvarDOS package manager." */
  outputnl("");
  outputnl(svarlang_str(1, 19)); /* "Usage:" */
  outputnl("");
  outputnl(svarlang_str(1, 20)); /* "pkg install package.svp */
  outputnl(svarlang_str(1, 21)); /* "pkg update package.svp" */
  outputnl(svarlang_str(1, 22)); /* "pkg rm package" */
  outputnl(svarlang_str(1, 23)); /* "pkg files package" */
  outputnl(svarlang_str(1, 24)); /* "pkg list [filter]" */
  outputnl(svarlang_str(1, 25)); /* "pkg check [pkg]" */
  outputnl(svarlang_str(1, 26)); /* "pkg check+ [pkg]" */
  outputnl(svarlang_str(1, 27)); /* "pkg unzip file.zip" */
  outputnl(svarlang_str(1, 29)); /* "pkg listzip file.zip" */
  outputnl(svarlang_str(1, 28)); /* "pkg crc32 file" */
  outputnl("");
  outputnl(svarlang_str(1, 40)); /* "PKG is published under the MIT license." */
  outputnl(svarlang_str(1, 41)); /* "It is configured through %DOSDIR%\CFG\PKG.CFG" */
  return(1);
}


static enum ACTIONTYPES parsearg(int argc, char * const *argv, unsigned char *flags) {
  *flags = 0;

  /* look for valid actions */
  if ((argc == 3) && (strcasecmp(argv[1], "install") == 0)) {
    return(ACTION_INSTALL);
  } else if ((argc == 3) && (strcasecmp(argv[1], "inowarn") == 0)) {
    /* hidden action used by the SvarDOS installer to avoid onscreen warnings
     * during system installation */
    *flags = PKGINST_HIDEWARN;
    return(ACTION_INSTALL);
  } else if ((argc == 3) && (strcasecmp(argv[1], "update") == 0)) {
    *flags = PKGINST_UPDATE;
    return(ACTION_INSTALL);
  } else if ((argc == 3) && (strcasecmp(argv[1], "rm") == 0)) {
    return(ACTION_REMOVE);
  } else if ((argc == 3) && (strcasecmp(argv[1], "files") == 0)) {
    return(ACTION_LISTFILES);
  } else if ((argc >= 2) && (argc <= 3) && (strcasecmp(argv[1], "list") == 0)) {
    return(ACTION_LISTLOCAL);
  } else if ((argc >= 2) && (argc <= 3) && (strcasecmp(argv[1], "check") == 0)) {
    return(ACTION_HEALTHCHECK);
  } else if ((argc >= 2) && (argc <= 3) && (strcasecmp(argv[1], "check+") == 0)) {
    *flags = 1; /* extended check */
    return(ACTION_HEALTHCHECK);
  } else if ((argc == 3) && (strcasecmp(argv[1], "unzip") == 0)) {
    return(ACTION_UNZIP);
  } else if ((argc == 3) && (strcasecmp(argv[1], "ziplist") == 0)) {
    *flags = 1; /* list only */
    return(ACTION_UNZIP);
  } else if ((argc == 3) && (strcasecmp(argv[1], "crc32") == 0)) {
    return(ACTION_CRC32);
  } else {
    return(ACTION_HELP);
  }
}


static int pkginst(const char *file, int flags, const char *dosdir, const struct customdirs *dirlist, char bootdrive, unsigned char *buff15k) {
  char pkgname[9];
  int res = 1;
  struct ziplist *zipfileidx;
  FILE *zipfilefd;

  /* prepare the zip file for installation and fetch package's name */
  zipfileidx = pkginstall_preparepackage(pkgname, file, flags, &zipfilefd, dosdir, dirlist, bootdrive);
  if (zipfileidx != NULL) {

    /* package name must be all lower-case for further file matching in the zip file */
    strlwr(pkgname);

    /* remove the old version of the package if we are UPDATING it */
    res = 0;
    if (flags & PKGINST_UPDATE) res = pkgrem(pkgname, dosdir);

    if (res == 0) res = pkginstall_installpackage(pkgname, dosdir, dirlist, zipfileidx, zipfilefd, bootdrive, buff15k, flags);
    zip_freelist(&zipfileidx);

    fclose(zipfilefd);
  }

  return(res);
}


/* pkg crc32 file */
static int crcfile(const char *fname, unsigned char *buff4k) {
  FILE *fd;
  unsigned long crc;
  unsigned int len;

  fd = fopen(fname, "rb");
  if (fd == NULL) {
    outputnl(svarlang_str(10, 1)); /* failed to open file */
    return(1);
  }

  crc = CRC32_INITVAL;

  for (;;) {
    len = fread(buff4k, 1, 4096, fd);
    if (len == 0) break;
    crc32_feed(&crc, buff4k, len);
  }
  fclose(fd);

  crc32_finish(&crc);

  crc32tostring(buff4k, crc);
  outputnl(buff4k);

  return(0);
}


int main(int argc, char **argv) {
  static unsigned char buff15k[15 * 1024];
  int res = 1;
  enum ACTIONTYPES action;
  unsigned char actionflags;
  const char *dosdir;
  struct customdirs *dirlist;
  char bootdrive;

  svarlang_autoload_exepath(argv[0], getenv("LANG"));   /* NLS init */

  action = parsearg(argc, argv, &actionflags);
  switch (action) {
    case ACTION_HELP:
      res = showhelp();
      goto GAMEOVER;
    case ACTION_UNZIP:
      res = unzip(argv[2], actionflags, buff15k);
      goto GAMEOVER;
    case ACTION_CRC32:
      res = crcfile(argv[2], buff15k);
      goto GAMEOVER;
  }

  /* read the DOSDIR environment variable */
  dosdir = getenv("DOSDIR");
  if (dosdir == NULL) {
    outputnl(svarlang_str(2, 2)); /* "%DOSDIR% not set! You should make it point to the SvarDOS main directory." */
    outputnl(svarlang_str(2, 3)); /* "Example: SET DOSDIR=C:\SVARDOS" */
    goto GAMEOVER;
  }

  /* load configuration */
  if (loadconf(dosdir, &dirlist, &bootdrive) != 0) goto GAMEOVER;

  switch (action) {
    case ACTION_INSTALL:
      res = pkginst(argv[2], actionflags, dosdir, dirlist, bootdrive, buff15k);
      break;
    case ACTION_REMOVE:
      res = pkgrem(argv[2], dosdir);
      break;
    case ACTION_LISTFILES:
      res = listfilesofpkg(argv[2], dosdir);
      break;
    case ACTION_LISTLOCAL:
      res = showinstalledpkgs((argc == 3)?argv[2]:NULL, dosdir);
      break;
    case ACTION_HEALTHCHECK:
      res = healthcheck(buff15k, (argc == 3)?argv[2]:NULL, dosdir, actionflags);
      break;
    default:
      res = showhelp();
      break;
  }

  GAMEOVER:
  if (res != 0) return(1);
  return(0);
}
