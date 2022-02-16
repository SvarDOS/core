/*
 * PKG - SvarDOS package manager
 *
 * PUBLISHED UNDER THE TERMS OF THE MIT LICENSE
 *
 * COPYRIGHT (C) 2016-2022 MATEUSZ VISTE, ALL RIGHTS RESERVED.
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
  puts("");
  puts(svarlang_str(1, 25)); /* "PKG is published under the MIT license." */
  puts(svarlang_str(1, 26)); /* "It is configured through %DOSDIR%\CFG\PKG.CFG" */
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
  } else {
    return(ACTION_HELP);
  }
}


static int pkginst(const char *file, int flags, const char *dosdir, const struct customdirs *dirlist) {
  char pkgname[9];
  int t, lastpathdelim = -1, lastdot = -1, res = 1;
  struct ziplist *zipfileidx;
  FILE *zipfilefd;
  /* copy the filename into pkgname (without path elements and without extension) */
  for (t = 0; file[t] != 0; t++) {
    switch (file[t]) {
      case '/':
      case '\\':
        lastpathdelim = t;
        break;
      case '.':
        lastdot = t;
        break;
    }
  }
  if (lastdot <= lastpathdelim) lastdot = t; /* a dot before last path delimiters is not an extension prefix */
  t = lastdot - (lastpathdelim + 1);
  if (t + 1 > sizeof(pkgname)) {
    puts(svarlang_str(3, 24)); /* "ERROR: package name too long" */
    return(1);
  }
  memcpy(pkgname, file + lastpathdelim + 1, t);
  pkgname[t] = 0;
  /* prepare the zip file and install it */
  zipfileidx = pkginstall_preparepackage(pkgname, file, flags, &zipfilefd, dosdir, dirlist);
  if (zipfileidx != NULL) {
    /* remove the old version of the package if we are UPDATING it */
    res = 0;
    if (flags & PKGINST_UPDATE) res = pkgrem(pkgname, dosdir);
    if (res == 0) res = pkginstall_installpackage(pkgname, dosdir, dirlist, zipfileidx, zipfilefd);
    zip_freelist(&zipfileidx);
  }

  fclose(zipfilefd);
  return(res);
}


int main(int argc, char **argv) {
  int res = 1;
  enum ACTIONTYPES action;
  const char *dosdir;
  struct customdirs *dirlist;

  svarlang_autoload("pkg"); /* NLS init */

  action = parsearg(argc, argv);
  if (action == ACTION_HELP) {
    showhelp();
    goto GAMEOVER;
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
    case ACTION_UNZIP:
      res = unzip(argv[2]);
      break;
    default:
      res = showhelp();
      break;
  }

  GAMEOVER:
  if (res != 0) return(1);
  return(0);
}
