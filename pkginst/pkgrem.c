/*
 * This file is part of the pkginst (SvarDOS) project.
 * Copyright (C) Mateusz Viste 2012-2021. All rights reserved.
 */

#include <ctype.h>    /* toupper() */
#include <stdio.h>
#include <string.h>    /* strlen() */
#include <stdlib.h>    /* free() */
#include <unistd.h>    /* rmdir(), unlink() */
#include <direct.h>  /* watcom needs this for the rmdir() prototype */

#include "helpers.h"   /* slash2backslash() */
#include "kprintf.h"

#include "pkgrem.h"


struct dirliststruct {
  struct dirliststruct *next;
  char dirname[2]; /* this must be the last item in the structure */
};


/* adds a directory to dirlist, if not already present */
static struct dirliststruct *rememberdir(struct dirliststruct *dirlist, const char *path) {
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
int pkgrem(const char *pkgname, const char *dosdir) {
  char fpath[256];
  char buff[256];
  FILE *flist;
  int lastdirsep;
  struct dirliststruct *dirlist = NULL; /* used to remember directories to remove */

  /* open the file %DOSDIR%\packages\pkgname.lst */
  sprintf(fpath, "%s\\packages\\%s.lst", dosdir, pkgname);
  flist = fopen(fpath, "rb");
  if (flist == NULL) {
    kitten_printf(4, 0, "Package %s is not installed, so not removed.", pkgname);
    puts("");
    return(-1);
  }

  /* remove all files/folders listed in pkgname.lst but NOT pkgname.lst */
  while (freadtokval(flist, buff, sizeof(buff), NULL, 0) == 0) {
    int x;
    slash2backslash(buff); /* change all slash to backslash */
    if (buff[0] == 0) continue; /* skip empty lines */

    /* remember the path part for removal later */
    lastdirsep = -1;
    for (x = 1; buff[x] != 0; x++) {
      if ((buff[x] == '\\') && (buff[x - 1] != ':')) lastdirsep = x;
    }
    if (lastdirsep > 0) {
      buff[lastdirsep] = 0;
      dirlist = rememberpath(dirlist, buff);
      buff[lastdirsep] = '\\';
    }

    /* if it's a directory, skip it */
    if (buff[strlen(buff) - 1] == '\\') continue;

    /* do not delete pkgname.lst at this point - I am using it (will be deleted later) */
    if (strcasecmp(buff, fpath) == 0) continue;

    /* remove it */
    kitten_printf(4, 4, "removing %s", buff);
    puts("");
    unlink(buff);
  }

  /* close the lst file */
  fclose(flist);

  /* iterate over dirlist and remove directories if empty, from longest to shortest */
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
    /* printf("RMDIR %s\n", dirlist->dirname); */
    rmdir(dirlist->dirname);
    /* free the allocated memory for this entry */
    dirlistpos = dirlist;
    dirlist = dirlistpos->next;
    free(dirlistpos);
  }

  /* remove the lst file */
  unlink(fpath);
  kitten_printf(4, 5, "Package %s has been removed.", pkgname);
  puts("");
  return(0);
}
