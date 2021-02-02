/*
 * PKGINST - SvarDOS package installer
 *
 * PUBLISHED UNDER THE TERMS OF THE MIT LICENSE
 *
 * COPYRIGHT (C) 2016-2021 MATEUSZ VISTE, ALL RIGHTS RESERVED.
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

#include "kitten/kitten.h"
#include "kprintf.h"
#include "libunzip.h"
#include "pkginst.h"
#include "pkgrem.h"
#include "showinst.h"
#include "version.h"


enum ACTIONTYPES {
  ACTION_INSTALL,
  ACTION_REMOVE,
  ACTION_LISTFILES,
  ACTION_LISTLOCAL,
  ACTION_HELP
};


static int showhelp(void) {
  printf("PKGINST ver " PVER " Copyright (C) " PDATE " Mateusz Viste\n"
         "\n"
         "PKGINST is the package installer for SvarDOS.\n"
         "\n"
         "Usage: PKGINST install package.zip\n"
         "       PKGINST remove package\n"
         "       PKGINST listfiles package\n"
         "       PKGINST listlocal [filter]\n"
         "\n"
         "PKGINST is published under the MIT license. It uses a configuration file\n"
         "located at %%DOSDIR%%\\CFG\\PKGINST.CFG\n"
         );
  return(1);
}


static enum ACTIONTYPES parsearg(int argc, char * const *argv) {
  /* look for valid actions */
  if ((argc == 3) && (strcasecmp(argv[1], "install") == 0)) {
    return(ACTION_INSTALL);
  } else if ((argc == 3) && (strcasecmp(argv[1], "remove") == 0)) {
    return(ACTION_REMOVE);
  } else if ((argc == 3) && (strcasecmp(argv[1], "listfiles") == 0)) {
    return(ACTION_LISTFILES);
  } else if ((argc >= 2) && (argc <= 3) && (strcasecmp(argv[1], "listlocal") == 0)) {
    return(ACTION_LISTLOCAL);
  } else {
    return(ACTION_HELP);
  }
}


static int pkginst(const char *file, int flags, const char *dosdir, const struct customdirs *dirlist) {
  char pkgname[32];
  int t, lastpathdelim = -1, u = 0;
  struct ziplist *zipfileidx;
  FILE *zipfilefd;
  for (t = 0; file[t] != 0; t++) {
    if ((file[t] == '/') || (file[t] == '\\')) lastpathdelim = t;
  }
  /* copy the filename into pkgname (without path elements) */
  for (t = lastpathdelim + 1; file[t] != 0; t++) pkgname[u++] = file[t];
  pkgname[u] = 0; /* terminate the string */
  /* truncate the file's extension (.zip) */
  for (t = u; t > 0; t--) {
    if (pkgname[t] == '.') {
      pkgname[t] = 0;
      break;
    }
  }
  /* prepare the zip file and install it */
  zipfileidx = pkginstall_preparepackage(pkgname, file, flags, &zipfilefd, dosdir, dirlist);
  if (zipfileidx != NULL) {
    int res = 0;
    if (pkginstall_installpackage(pkgname, dosdir, dirlist, zipfileidx, zipfilefd) != 0) res = 1;
    fclose(zipfilefd);
    return(res);
  } else {
    fclose(zipfilefd);
    return(1);
  }
}


int main(int argc, char **argv) {
  int res = 1, flags;
  enum ACTIONTYPES action;
  const char *dosdir;
  struct customdirs *dirlist;

  kittenopen("pkginst"); /* NLS init */

  action = parsearg(argc, argv);
  if (action == ACTION_HELP) {
    showhelp();
    goto GAMEOVER;
  }

  /* read the DOSDIR environment variable */
  dosdir = getenv("DOSDIR");
  if (dosdir == NULL) {
    kitten_puts(2, 2, "%DOSDIR% not set! You should make it point to the FreeDOS main directory.");
    kitten_puts(2, 3, "Example: SET DOSDIR=C:\\FDOS");
    goto GAMEOVER;
  }

  /* load configuration */
  flags = 0;
  dirlist = NULL;
  if (loadconf(dosdir, &dirlist, &flags) != 0) goto GAMEOVER;

  switch (action) {
    case ACTION_INSTALL:
      res = pkginst(argv[2], flags, dosdir, dirlist);
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
  kittenclose(); /* NLS de-init */
  if (res != 0) return(1);
  return(0);
}
