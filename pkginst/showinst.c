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

#include "fdnpkg.h"   /* PKGINST_UPDATE */
#include "fileexst.h"
#include "getdelim.h"
#include "helpers.h"  /* fdnpkg_strcasestr(), slash2backslash()... */
#include "kprintf.h"
#include "libunzip.h"  /* zip_freelist()... */
#include "lsm.h"
#include "pkgdb.h"
#include "pkginst.h"
#include "pkgrem.h"
#include "rtrim.h"
#include "showinst.h"  /* include self for control */
#include "version.h"


/* this is a wrapper around strcasecmp(), to be used by qsort() */
static int strcompare(const void *str1, const void *str2) {
  char **s1 = (char **)str1;
  char **s2 = (char **)str2;
  return(strcasecmp(*s1, *s2));
}


static int loadinstpkglist(char **packagelist, char **packagelist_ver, int packagelist_maxlen, char *filterstr, char *dosdir) {
  DIR *dp;
  int packagelist_len = 0, x;
  struct dirent *ep;
  #define verstr_maxlen 64
  char pkgdir[512], lsmfilename[1024], verstr[verstr_maxlen];
  sprintf(pkgdir, "%s\\packages", dosdir);
  dp = opendir(pkgdir);
  if (dp != NULL) {
    while ((ep = readdir(dp)) != NULL) {
      if (ep->d_name[0] != '.') { /* ignore '.', '..', and hidden directories */
        if (strlen(ep->d_name) > 4) {
          int tlen = strlen(ep->d_name);
          if ((ep->d_name[tlen - 4] != '.') || (tolower(ep->d_name[tlen - 3]) != 'l') || (tolower(ep->d_name[tlen - 2]) != 's') || (tolower(ep->d_name[tlen - 1]) != 't')) continue;  /* if it's not an .lst file, skip it silently */

          if (filterstr != NULL) {
            if (fdnpkg_strcasestr(ep->d_name, filterstr) == NULL) continue; /* if it's not matching the non-NULL filter, skip it */
          }
          if (packagelist_len >= packagelist_maxlen) {
            closedir(dp);
            return(-1); /* if not enough place in the list - return an error */
          }
          packagelist[packagelist_len] = strdup(ep->d_name);
          packagelist[packagelist_len][strlen(packagelist[packagelist_len]) - 4] = 0; /* cut out the .lst extension */
          packagelist_len += 1;
        }
      }
    }
    closedir(dp);
    qsort(packagelist, packagelist_len, sizeof (char **), strcompare); /* sort the package list */
    for (x = 0; x < packagelist_len; x++) {
      /* for each package, load the metadata from %DOSDIR\APPINFO\*.lsm */
      sprintf(lsmfilename, "%s\\appinfo\\%s.lsm", dosdir, packagelist[x]);
      if (readlsm(lsmfilename, verstr, verstr_maxlen) != 0) sprintf(verstr, "(unknown version)");
      packagelist_ver[x] = strdup(verstr);
    }
    return(packagelist_len);
  } else {
    kitten_printf(9, 0, "Error: Could not access the %s directory.", pkgdir);
    puts("");
    return(-1);
  }
}

#define packagelist_maxlen 1024

void showinstalledpkgs(char *filterstr, char *dosdir) {
  char *packagelist[packagelist_maxlen];
  char *packagelist_ver[packagelist_maxlen];
  int packagelist_len, x;

  /* load the list of packages */
  packagelist_len = loadinstpkglist(packagelist, packagelist_ver, packagelist_maxlen, filterstr, dosdir);   /* Populate the packages list */
  if (packagelist_len < 0) return;
  if (packagelist_len == 0) {
    kitten_puts(5, 0, "No package matched the search.");
    return;
  }

  /* iterate through all packages */
  for (x = 0; x < packagelist_len; x++) {
    /* print the package/version couple on screen */
    printf("%s %s\n", packagelist[x], packagelist_ver[x]);
  }
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
struct flist_t *pkg_loadflist(char *pkgname, char *dosdir) {
  struct flist_t *res = NULL, *newnode;
  FILE *fd;
  char *lineptr;
  char buff[512];
  int getdelimlen, fnamelen;
  size_t getdelimcount = 0;
  sprintf(buff, "%s\\packages\\%s.lst", dosdir, pkgname);
  if (fileexists(buff) == 0) { /* file does not exist */
    kitten_printf(9, 1, "Error: Local package %s not found.", pkgname);
    puts("");
    return(NULL);
  }
  fd = fopen(buff, "rb");
  if (fd == NULL) {
    kitten_puts(4, 1, "Error opening lst file!");
    return(NULL);
  }
  /* iterate through all lines of the file */
  for (;;) {
    lineptr = NULL;
    getdelimlen = getdelim(&lineptr, &getdelimcount, '\n', fd);
    if (getdelimlen < 0) { /* EOF */
      free(lineptr);
      break;
    }
    rtrim(lineptr);  /* right-trim the filename */
    slash2backslash(lineptr); /* change all / to \ */
    if ((lineptr[0] == 0) || (lineptr[0] == '\r') || (lineptr[0] == '\n')) continue; /* skip empty lines */
    if (lineptr[strlen(lineptr) - 1] == '\\') continue; /* skip directories */
    if ((lineptr[0] == '\\') || (lineptr[1] == ':')) { /* this is an absolute path */
      fnamelen = snprintf(buff, sizeof(buff), "%s", lineptr);
    } else { /* else it's a relative path starting at %dosdir% */
      fnamelen = snprintf(buff, sizeof(buff), "%s\\%s\n", dosdir, lineptr);
    }
    free(lineptr); /* free the memory occupied by the line */
    /* add the new node to the result */
    newnode = malloc(sizeof(struct flist_t) + fnamelen);
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
void listfilesofpkg(char *pkgname, char *dosdir) {
  struct flist_t *flist, *flist_ptr;
  /* load the list of files belonging to pkgname */
  flist = pkg_loadflist(pkgname, dosdir);
  /* display each filename on screen */
  for (flist_ptr = flist; flist_ptr != NULL; flist_ptr = flist_ptr->next) {
    printf("%s\n", flist_ptr->fname);
  }
  /* free the list of files */
  pkg_freeflist(flist);
}
