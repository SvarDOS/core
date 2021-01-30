/*
 * This file is part of the FDNPKG project.
 * Copyright (C) Mateusz Viste 2012-2016. All rights reserved.
 */

#include <ctype.h>    /* toupper() */
#include <stdio.h>
#include <string.h>    /* strlen() */
#include <stdlib.h>    /* free() */
#include <unistd.h>    /* rmdir(), unlink() */

#ifdef __WATCOMC__
#include <direct.h>  /* watcom needs this for the rmdir() prototype */
#endif

#include "fileexst.h"
#include "getdelim.h"
#include "helpers.h"   /* slash2backslash() */
#include "kprintf.h"
#include "pkgrem.h"
#include "rtrim.h"
#include "version.h"


struct dirliststruct {
  struct dirliststruct *next;
  char dirname[2]; /* this must be the last item in the structure */
};


/* adds a directory to dirlist, if not already present */
static struct dirliststruct *rememberdir(struct dirliststruct *dirlist, char *path) {
  struct dirliststruct *res;
  /* if already present, do nothing */
  for (res = dirlist; res != NULL; res = res->next) {
    if (strcasecmp(res->dirname, path) == 0) return(dirlist);
  }
  /* not in the list yet - add it */
  res = malloc(sizeof(struct dirliststruct) + strlen(path));
  if (res == NULL) {  /* out of memory */
    kitten_printf(4, 3, "Out of memory! Could not store directory %s!", path);
    puts("");
    return(NULL);
  }
  strcpy(res->dirname, path);
  res->next = dirlist;
  return(res);
}


/* explode a path into subdirectories, and remember each one inside dirlist */
static struct dirliststruct *rememberpath(struct dirliststruct *dirlist, char *path) {
  int x, gameover = 0;
  /* iterate on the path, and add each subdirectory */
  for (x = 0;; x++) {
    switch (path[x]) {
      case 0:
        gameover = 1;
      case '/':
      case '\\':
        path[x] = 0;
        dirlist = rememberdir(dirlist, path);
        path[x] = '\\';
    }
    if (gameover != 0) break;
  }
  return(dirlist);
}


/* removes a package from the system. Returns 0 on success, non-zero otherwise */
int pkgrem(char *pkgname, char *dosdir) {
  char fpath[512];
  char shellcmd[512];
  char *lineptr;
  FILE *flist;
  int getdelimlen;
  int lastdirsep;
  int x;
  size_t getdelimcount = 0;
  struct dirliststruct *dirlist = NULL; /* used to remember directories to remove */
  char pkglistfile[512];

  /* Check if the file %DOSDIR%\packages\pkgname.lst exists (if not, the package is not installed) */
  sprintf(fpath, "%s\\packages\\%s.lst", dosdir, pkgname);
  if (fileexists(fpath) == 0) { /* file does not exist */
    kitten_printf(4, 0, "Package %s is not installed, so not removed.", pkgname);
    puts("");
    return(-1);
  }

  /* open the file %DOSDIR%\packages\pkgname.lst */
  flist = fopen(fpath, "r");
  if (flist == NULL) {
    kitten_puts(4, 1, "Error opening lst file!");
    return(-2);
  }

  sprintf(pkglistfile, "packages\\%s.lst", pkgname);

  /* remove all files/folders listed in pkgname.lst but NOT pkgname.lst */
  for (;;) {
    /* read line from file */
    lineptr = NULL;
    getdelimlen = getdelim(&lineptr, &getdelimcount, '\n', flist);
    if (getdelimlen < 0) {
      free(lineptr);
      break;
    }
    rtrim(lineptr);  /* right-trim the filename */
    slash2backslash(lineptr); /* change all / to \ */
    if ((lineptr[0] == 0) || (lineptr[0] == '\r') || (lineptr[0] == '\n')) {
      free(lineptr); /* free the memory occupied by the line */
      continue; /* skip empty lines */
    }
    /* remember the path part for removal later */
    lastdirsep = -1;
    for (x = 1; lineptr[x] != 0; x++) {
      if ((lineptr[x] == '\\') && (lineptr[x - 1] != ':')) lastdirsep = x;
    }
    if (lastdirsep > 0) {
      lineptr[lastdirsep] = 0;
      dirlist = rememberpath(dirlist, lineptr);
      lineptr[lastdirsep] = '\\';
    }
    /* if it's a directory, skip it */
    if (lineptr[strlen(lineptr) - 1] == '\\') {
      free(lineptr); /* free the memory occupied by the line */
      continue;
    }
    /* it's a file - remove it */
    if (strcasecmp(pkglistfile, lineptr) != 0) { /* never delete pkgname.lst at this point - it will be deleted later */
      if ((lineptr[0] == '\\') || (lineptr[1] == ':')) { /* this is an absolute path */
        sprintf(shellcmd, "%s", lineptr);
      } else { /* else it's a relative path starting at %dosdir% */
        sprintf(shellcmd, "%s\\%s", dosdir, lineptr);
      }
      kitten_printf(4, 4, "removing %s", shellcmd);
      puts("");
      unlink(shellcmd);
    }
    free(lineptr); /* free the memory occupied by the line */
  }

  /* close the file */
  fclose(flist);

  /* iterate through dirlist and remove directories if empty, from longest to shortest */
  while (dirlist != NULL) {
    struct dirliststruct *dirlistpos, *previousdir;
    /* find the longest path, and put it on top */
    previousdir = dirlist;
    for (dirlistpos = dirlist->next; dirlistpos != NULL; dirlistpos = dirlistpos->next) {
      if (strlen(dirlistpos->dirname) > strlen(dirlist->dirname)) {
        previousdir->next = dirlistpos->next;
        dirlistpos->next = dirlist;
        dirlist = dirlistpos;
        dirlistpos = previousdir;
      } else {
        previousdir = dirlistpos;
      }
    }
    if ((dirlist->dirname[0] == '\\') || (dirlist->dirname[1] == ':')) { /* this is an absolute path */
      sprintf(shellcmd, "%s", dirlist->dirname);
    } else { /* else it's a relative path starting at %dosdir% */
      sprintf(shellcmd, "%s\\%s", dosdir, dirlist->dirname);
    }
    /* printf("RMDIR %s\n", shellcmd); */
    rmdir(shellcmd);
    /* free the allocated memory for this entry */
    dirlistpos = dirlist;
    dirlist = dirlistpos->next;
    free(dirlistpos);
  }

  /* remove %DOSDIR%\packages\pkgname.lst */
  unlink(fpath);
  kitten_printf(4, 5, "Package %s has been removed.", pkgname);
  puts("");
  return(0);
}
