/*
 * This file is part of PKGINST (SvarDOS)
 * Copyright (C) 2013-2021 Mateusz Viste. All rights reserved.
 */

#include <stdio.h>
#include <ctype.h>    /* tolower() */
#include <stdlib.h>   /* atoi(), qsort() - not using it after all, redefining it manually later */
#include <string.h>   /* strlen() */
#include <sys/types.h>
#include <direct.h> /* opendir() and friends */

#include "helpers.h"  /* fdnpkg_strcasestr(), slash2backslash()... */
#include "kprintf.h"
#include "libunzip.h"  /* zip_freelist()... */
#include "lsm.h"

#include "showinst.h"  /* include self for control */


int showinstalledpkgs(const char *filterstr, const char *dosdir) {
  DIR *dp;
  struct dirent *ep;
  char buff[256];
  char ver[16];
  int matchfound = 0;

  sprintf(buff, "%s\\packages", dosdir);
  dp = opendir(buff);
  if (dp == NULL) {
    kitten_printf(9, 0, "Error: Could not access directory %s", buff);
    puts("");
    return(-1);
  }

  while ((ep = readdir(dp)) != NULL) { /* readdir() result must never be freed (statically allocated) */
    int tlen = strlen(ep->d_name);
    if (ep->d_name[0] == '.') continue; /* ignore '.', '..', and hidden directories */
    if (tlen < 4) continue; /* files must be at least 5 bytes long ("x.lst") */
    if (strcasecmp(ep->d_name + tlen - 4, ".lst") != 0) continue;  /* if not an .lst file, skip it silently */
    ep->d_name[tlen - 4] = 0; /* trim out the ".lst" suffix */

    if (filterstr != NULL) {
      if (fdnpkg_strcasestr(ep->d_name, filterstr) == NULL) continue; /* if it's not matching the non-NULL filter, skip it */
    }

    /* load the metadata from %DOSDIR\APPINFO\*.lsm */
    sprintf(buff, "%s\\appinfo\\%s.lsm", dosdir, ep->d_name);
    readlsm(buff, ver, sizeof(ver));

    printf("%s %s", ep->d_name, ver);
    puts("");
    matchfound = 1;
  }
  if (matchfound == 0) kitten_puts(5, 0, "No package matched the search.");

  closedir(dp);
  return(0);
}


/* frees a linked list of filenames */
void pkg_freeflist(struct flist_t *flist) {
  while (flist != NULL) {
    struct flist_t *victim = flist;
    flist = flist->next;
    free(victim);
  }
}


/* returns a linked list of the files that belong to package pkgname */
struct flist_t *pkg_loadflist(const char *pkgname, const char *dosdir) {
  struct flist_t *res = NULL, *newnode;
  FILE *fd;
  char buff[256];

  sprintf(buff, "%s\\packages\\%s.lst", dosdir, pkgname);
  fd = fopen(buff, "rb");
  if (fd == NULL) {
    kitten_printf(9, 1, "Error: Local package %s not found.", pkgname);
    puts("");
    return(NULL);
  }
  /* iterate through all lines of the file */
  while (freadtokval(fd, buff, sizeof(buff), NULL, 0) == 0) {
    slash2backslash(buff); /* change all / to \ */
    if (buff[0] == 0) continue; /* skip empty lines */
    if (buff[strlen(buff) - 1] == '\\') continue; /* skip directories */
    /* add the new node to the result */
    newnode = malloc(sizeof(struct flist_t) + strlen(buff));
    if (newnode == NULL) {
      kitten_printf(2, 14, "Out of memory! (%s)", "malloc failure");
      continue;
    }
    newnode->next = res;
    strcpy(newnode->fname, buff);
    res = newnode;
  }
  fclose(fd);
  return(res);
}


/* Prints files owned by a package */
int listfilesofpkg(const char *pkgname, const char *dosdir) {
  struct flist_t *flist, *flist_ptr;
  /* load the list of files belonging to pkgname */
  flist = pkg_loadflist(pkgname, dosdir);
  if (flist == NULL) return(-1);
  /* display each filename on screen */
  for (flist_ptr = flist; flist_ptr != NULL; flist_ptr = flist_ptr->next) {
    puts(flist_ptr->fname);
  }
  /* free the list of files */
  pkg_freeflist(flist);
  return(0);
}
