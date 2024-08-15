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


#include <stdio.h>    /* printf() */
#include <stdlib.h>   /* malloc() and friends */
#include <string.h>   /* strcasecmp() */

#include "svarlang.lib/svarlang.h"
#include "crc32.h"
#include "helpers.h"
#include "kprintf.h"
#include "libunzip.h"
#include "pkginst.h"
#include "pkgrem.h"
#include "showinst.h"
#include "unzip.h"
#include "version.h"


enum ACTIONTYPES {
  ACTION_INSTALL,
  ACTION_UPDATE,
  ACTION_REMOVE,
  ACTION_LISTFILES,
  ACTION_LISTLOCAL,
  ACTION_UNZIP,
  ACTION_CRC32,
  ACTION_HELP
};


static int showhelp(void) {
  puts("PKG ver " PVER " Copyright (C) " PDATE " Mateusz Viste");
  puts("");
  puts(svarlang_str(1, 0)); /* "PKG is the SvarDOS package manager." */
  puts("");
  puts(svarlang_str(1, 20)); /* "Usage: pkg install package.svp */
  puts(svarlang_str(1, 21)); /* "       pkg update package.svp" */
  puts(svarlang_str(1, 22)); /* "       pkg remove package" */
  puts(svarlang_str(1, 23)); /* "       pkg listfiles package" */
  puts(svarlang_str(1, 24)); /* "       pkg listlocal [filter]" */
  puts(svarlang_str(1, 27)); /* "       pkg unzip file.zip" */
  puts(svarlang_str(1, 28)); /* "       pkg crc32 file" */
  puts("");
  puts(svarlang_str(1, 40)); /* "PKG is published under the MIT license." */
  puts(svarlang_str(1, 41)); /* "It is configured through %DOSDIR%\CFG\PKG.CFG" */
  return(1);
}


static enum ACTIONTYPES parsearg(int argc, char * const *argv) {
  /* look for valid actions */
  if ((argc == 3) && (strcasecmp(argv[1], "install") == 0)) {
    return(ACTION_INSTALL);
  } else if ((argc == 3) && (strcasecmp(argv[1], "update") == 0)) {
    return(ACTION_UPDATE);
  } else if ((argc == 3) && (strcasecmp(argv[1], "remove") == 0)) {
    return(ACTION_REMOVE);
  } else if ((argc == 3) && (strcasecmp(argv[1], "listfiles") == 0)) {
    return(ACTION_LISTFILES);
  } else if ((argc >= 2) && (argc <= 3) && (strcasecmp(argv[1], "listlocal") == 0)) {
    return(ACTION_LISTLOCAL);
  } else if ((argc == 3) && (strcasecmp(argv[1], "unzip") == 0)) {
    return(ACTION_UNZIP);
  } else if ((argc == 3) && (strcasecmp(argv[1], "crc32") == 0)) {
    return(ACTION_CRC32);
  } else {
    return(ACTION_HELP);
  }
}


static int pkginst(const char *file, int flags, const char *dosdir, const struct customdirs *dirlist) {
  char pkgname[9];
  int res = 1;
  struct ziplist *zipfileidx;
  FILE *zipfilefd;

  /* prepare the zip file for installation and fetch package's name */
  zipfileidx = pkginstall_preparepackage(pkgname, file, flags, &zipfilefd, dosdir, dirlist);
  if (zipfileidx != NULL) {

    /* package name must be all lower-case for further file matching in the zip file */
    strlwr(pkgname);

    /* remove the old version of the package if we are UPDATING it */
    res = 0;
    if (flags & PKGINST_UPDATE) res = pkgrem(pkgname, dosdir);

    if (res == 0) res = pkginstall_installpackage(pkgname, dosdir, dirlist, zipfileidx, zipfilefd);
    zip_freelist(&zipfileidx);
  }

  fclose(zipfilefd);
  return(res);
}


/* pkg crc32 file */
static int crcfile(const char *fname) {
  FILE *fd;
  unsigned long crc;
  unsigned char buff[512];
  unsigned int len;

  fd = fopen(fname, "rb");
  if (fd == NULL) {
    puts(svarlang_str(10, 1)); /* failed to open file */
    return(1);
  }

  crc = crc32_init();

  for (;;) {
    len = fread(buff, 1, sizeof(buff), fd);
    if (len == 0) break;
    crc32_feed(&crc, buff, len);
  }
  fclose(fd);

  crc32_finish(&crc);

  printf("%08lX", crc);
  puts("");

  return(0);
}


int main(int argc, char **argv) {
  int res = 1;
  enum ACTIONTYPES action;
  const char *dosdir;
  struct customdirs *dirlist;

  svarlang_autoload_exepath(argv[0], getenv("LANG"));   /* NLS init */

  action = parsearg(argc, argv);
  switch (action) {
    case ACTION_HELP:
      res = showhelp();
      goto GAMEOVER;
      break;
    case ACTION_UNZIP:
      res = unzip(argv[2]);
      goto GAMEOVER;
      break;
    case ACTION_CRC32:
      res = crcfile(argv[2]);
      goto GAMEOVER;
      break;
  }

  /* read the DOSDIR environment variable */
  dosdir = getenv("DOSDIR");
  if (dosdir == NULL) {
    puts(svarlang_str(2, 2)); /* "%DOSDIR% not set! You should make it point to the SvarDOS main directory." */
    puts(svarlang_str(2, 3)); /* "Example: SET DOSDIR=C:\SVARDOS" */
    goto GAMEOVER;
  }

  /* load configuration */
  if (loadconf(dosdir, &dirlist) != 0) goto GAMEOVER;

  switch (action) {
    case ACTION_UPDATE:
    case ACTION_INSTALL:
      res = pkginst(argv[2], (action == ACTION_UPDATE)?PKGINST_UPDATE:0, dosdir, dirlist);
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
    default:
      res = showhelp();
      break;
  }

  GAMEOVER:
  if (res != 0) return(1);
  return(0);
}
