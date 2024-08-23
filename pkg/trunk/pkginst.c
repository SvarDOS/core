/*
 * This file is part of pkg (SvarDOS)
 * Copyright (C) 2012-2024 Mateusz Viste
 */

#include <ctype.h>     /* toupper() */
#include <stdio.h>
#include <stdlib.h>    /* system() */
#include <string.h>    /* strcpy() */
#include <unistd.h>    /* read() */
#include <sys/types.h> /* struct utimbuf */

#include "helpers.h"   /* slash2backslash() */
#include "fileexst.h"
#include "kprintf.h"
#include "libunzip.h"  /* zip_listfiles()... */
#include "showinst.h"  /* pkg_loadflist() */
#include "svarlang.lib\svarlang.h"

#include "pkginst.h"   /* include self for control */


/* validate a filename (8+3, no weird characters, etc). returns 0 on success,
 * nonzero otherwise. */
static int validfilename(const char *fname) {
  int i, i2;
  char *validchars = "!#$%&'()-@^_`{}~";
  int elemlen = 0;
  int elemmaxlen = 8; /* switches from 8 to 3 depending wheter I am analyzing
                         a filename or an extension */
  /* look for invalid chars in the entire string, and check the length of the
   * element at the same time */
  for (i = 0; fname[i] != 0; i++) {
    /* director separators are threated specially */
    if (fname[i] == '\\') {
      elemlen = 0;
      elemmaxlen = 8;
      continue;
    }
    /* '.' switches the check into extension mode */
    if (fname[i] == '.') {
      if (elemlen == 0) return(-1); /* a file must have a base name */
      if (elemmaxlen == 3) return(-2); /* a file cannot have two extensions */
      elemlen = 0;
      elemmaxlen = 3;
      continue;
    }
    /* check that the element is not too long */
    if (++elemlen > elemmaxlen) return(-3);
    /* look for valid characters */
    if ((fname[i] >= 'a') && (fname[i] <= 'z')) continue;
    if ((fname[i] >= 'A') && (fname[i] <= 'Z')) continue;
    if ((fname[i] >= '0') && (fname[i] <= '9')) continue;
    if ((fname[i] & 128) != 0) continue; /* high bytes are okay (NLS) */
    /* look for other valid characters */
    for (i2 = 0; validchars[i2] != 0; i2++) {
      if (fname[i] == validchars[i2]) break;
    }
    if (validchars[i2] != 0) continue;
    /* if we are here, then the character is invalid */
    return(-4);
  }
  /* all checks passed */
  return(0);
}


/* returns 0 if pkgname is not installed, non-zero otherwise */
int is_package_installed(const char *pkgname, const char *dosdir) {
  char fname[256];
  sprintf(fname, "%s\\appinfo\\%s.lsm", dosdir, pkgname);
  return(fileexists(fname)); /* file exists -> package is installed */
}


/* checks that pkgname is NOT installed. return 0 on success, non-zero otherwise. */
static int validate_package_not_installed(const char *pkgname, const char *dosdir) {
  if (is_package_installed(pkgname, dosdir) != 0) {
    kitten_printf(3, 18, pkgname); /* "Package %s is already installed! You might want to use the 'update' action." */
    outputnl("");
    return(-1);
  }
  return(0);
}


/* find a filename in a flist linked list, and returns a pointer to it */
static struct flist_t *findfileinlist(struct flist_t *flist, const char *fname) {
  while (flist != NULL) {
    if (strcasecmp(flist->fname, fname) == 0) return(flist);
    flist = flist->next;
  }
  return(NULL);
}


/* prepare a package for installation. this is mandatory before installing it!
 * returns a pointer to the zip file's index on success, NULL on failure.
 * pkgname must be at least 9 bytes long and is filled with the package name.
 * the **zipfd pointer is updated with file descriptor of the open (to be
 * installed) zip file.
 * the returned ziplist is guaranteed to have the APPINFO file as first node
 * the ziplist is also guaranteed not to contain any directory entries */
struct ziplist *pkginstall_preparepackage(char *pkgname, const char *zipfile, int flags, FILE **zipfd, const char *dosdir, const struct customdirs *dirlist, char bootdrive) {
  char fname[256];
  struct ziplist *appinfoptr = NULL;
  char *shortfile;
  struct ziplist *ziplinkedlist = NULL, *curzipnode, *prevzipnode;
  struct flist_t *flist = NULL;

  /* copy zip filename into fname */
  strcpy(fname, zipfile);

  /* append an .SVP extension if not present */
  {
    int slen = strlen(fname);
    if ((slen < 4) || (fname[slen - 4] != '.')) strcat(fname, ".SVP");
  }

  /* does the file exist? */
  if (!fileexists(fname)) {
    outputnl(svarlang_str(10, 1)); /* ERROR: File not found */
    goto RAII;
  }

  /* open the SVP archive and get the list of files */
  *zipfd = fopen(fname, "rb");
  if (*zipfd == NULL) {
    outputnl(svarlang_str(3, 8)); /* "ERROR: Invalid zip archive! Package not installed." */
    goto RAII_ERR;
  }
  ziplinkedlist = zip_listfiles(*zipfd);
  if (ziplinkedlist == NULL) {
    outputnl(svarlang_str(3, 8)); /* "ERROR: Invalid zip archive! Package not installed." */
    goto RAII_ERR;
  }

  /* process the entire ziplist and sanitize it + locate the appinfo file so
   * I know the package name */
  prevzipnode = NULL;
  curzipnode = ziplinkedlist;
  while (curzipnode != NULL) {

    /* change all slashes to backslashes, and switch into all-lowercase */
    slash2backslash(curzipnode->filename);
    strlwr(curzipnode->filename);

    /* validate that the file has a valid filename (8+3, no shady chars...) */
    if (validfilename(curzipnode->filename) != 0) {
      outputnl(svarlang_str(3, 23)); /* "ERROR: Package contains an invalid filename:" */
      printf(" %s\n", curzipnode->filename);
      goto RAII_ERR;
    }

    /* remove 'directory' ZIP entries to avoid false alerts about directory already existing */
    if ((curzipnode->flags & ZIP_FLAG_ISADIR) != 0) goto DELETE_ZIP_NODE;

    /* abort if entry is encrypted */
    if ((curzipnode->flags & ZIP_FLAG_ENCRYPTED) != 0) {
      outputnl(svarlang_str(3, 20)); /* "ERROR: Package contains an encrypted file:" */
      printf(" %s\n", curzipnode->filename);
      goto RAII_ERR;
    }

    /* abort if file is compressed with an unsupported method */
    if ((curzipnode->compmethod != ZIP_METH_STORE) && (curzipnode->compmethod != ZIP_METH_DEFLATE)) { /* unsupported compression method */
      kitten_printf(8, 2, curzipnode->compmethod); /* "ERROR: Package contains a file compressed with an unsupported method (%d):" */
      outputnl("");
      printf(" %s\n", curzipnode->filename);
      goto RAII_ERR;
    }

    /* is it the appinfo file? detach it from the list for now */
    if (strstr(curzipnode->filename, "appinfo\\") == curzipnode->filename) {
      if (appinfoptr != NULL) {
        outputnl(svarlang_str(3, 12)); /* "ERROR: This is not a valid SvarDOS package" */
        goto RAII_ERR;
      }
      appinfoptr = curzipnode;
      curzipnode = curzipnode->nextfile;
      if (prevzipnode == NULL) {
        ziplinkedlist = curzipnode;
      } else {
        prevzipnode->nextfile = curzipnode;
      }
      continue;
    }

    /* all good, move to the next item in the list */
    prevzipnode = curzipnode;
    curzipnode = curzipnode->nextfile;
    continue;

    DELETE_ZIP_NODE:
    if (prevzipnode == NULL) {  /* take the item out of the list */
      ziplinkedlist = curzipnode->nextfile;
      free(curzipnode); /* free the item */
      curzipnode = ziplinkedlist;
    } else {
      prevzipnode->nextfile = curzipnode->nextfile;
      free(curzipnode); /* free the item */
      curzipnode = prevzipnode->nextfile;
    }
    /* go to the next item */
  }

  /* if appinfo file not found, this is not a SvarDOS package */
  if (appinfoptr == NULL) {
    outputnl(svarlang_str(3, 12)); /* "ERROR: This is not a valid SvarDOS package." */
    goto RAII_ERR;
  }

  /* attach the appinfo node to the top of the list (installation second stage
   * relies on this) */
  appinfoptr->nextfile = ziplinkedlist;
  ziplinkedlist = appinfoptr;

  /* fill in pkgname based on what was found in APPINFO */
  {
    unsigned short i;
    /* copy and stop at the nearest dot */
    for (i = 0; i < 8; i++) {
      if (appinfoptr->filename[8 + i] == '.') break;
      pkgname[i] = appinfoptr->filename[8 + i];
    }
    pkgname[i] = 0;
    if ((i == 0) || (strcmp(appinfoptr->filename + 8 + i, ".lsm") != 0)) {
      outputnl(svarlang_str(3, 12)); /* "ERROR: This is not a valid SvarDOS package." */
      goto RAII_ERR;
    }
  }

  /* if updating, load the list of files belonging to the current package */
  if ((flags & PKGINST_UPDATE) != 0) {
    flist = pkg_loadflist(pkgname, dosdir);
  } else {
    /* if NOT updating, check that package is not installed already */
    if (validate_package_not_installed(pkgname, dosdir) != 0) goto RAII_ERR;
  }

  /* Verify that there's no collision with existing local files, but skip the
   * first entry as it is the appinfo (LSM) file that is handled specially */

  for (curzipnode = ziplinkedlist->nextfile; curzipnode != NULL; curzipnode = curzipnode->nextfile) {

    /* look out for collisions with already existing files (unless we are
     * updating the package and the local file belongs to it */
    shortfile = computelocalpath(curzipnode->filename, fname, dosdir, dirlist, bootdrive);
    strcat(fname, shortfile);
    if ((findfileinlist(flist, fname) == NULL) && (fileexists(fname) != 0)) {
      outputnl(svarlang_str(3, 9)); /* "ERROR: Package contains a file that already exists locally:" */
      printf(" %s\n", fname);
      goto RAII_ERR;
    }
  }

  goto RAII;

  RAII_ERR:
  zip_freelist(&ziplinkedlist);
  ziplinkedlist = NULL;
  if ((zipfd != NULL) && (*zipfd != NULL)) {
    fclose(*zipfd);
    *zipfd = NULL;
  }

  RAII:
  pkg_freeflist(flist);
  return(ziplinkedlist);
}


/* look for a "warn" field in the package's LSM file and display it if found */
static void display_warn_if_exists(const char *pkgname, const char *dosdir, char *buff, size_t buffsz) {
  FILE *fd;
  char *msgptr;
  int warncount = 0, i;

  sprintf(buff, "%s\\appinfo\\%s.lsm", dosdir, pkgname);
  fd = fopen(buff, "rb");
  if (fd == NULL) return;

  while (freadtokval(fd, buff, buffsz, &msgptr, ':') == 0) {
    if (msgptr != NULL) {
      if (strcasecmp(buff, "warn") == 0) {
        /* print a visual delimiter */
        if (warncount == 0) {
          outputnl("");
          for (i = 0; i < 79; i++) putchar('*');
          outputnl("");
        }
        /* there may be more than one "warn" line */
        outputnl(msgptr);
        warncount++;
      }
    }
  }

  fclose(fd);

  /* if one or more warn lines have been displayed then close with a delimiter again */
  if (warncount > 0) {
    for (i = 0; i < 79; i++) putchar('*');
    outputnl("");
  }

}


/* install a package that has been prepared already. returns 0 on success,
 * or a negative value on error, or a positive value on warning */
int pkginstall_installpackage(const char *pkgname, const char *dosdir, const struct customdirs *dirlist, struct ziplist *ziplinkedlist, FILE *zipfd, char bootdrive) {
  char buff[256];
  char fulldestfilename[256];
  char *shortfile;
  long filesextractedsuccess = 0, filesextractedfailure = 0;
  struct ziplist *curzipnode;
  FILE *lsmfd;
  int unzip_result;

  /* create the %DOSDIR%/APPINFO directory, just in case it doesn't exist yet */
  sprintf(buff, "%s\\appinfo\\", dosdir);
  mkpath(buff);

  /* start by extracting the APPINFO (LSM) file - I need it so I can append the
   * list of files belonging to the packages later */
  output(ziplinkedlist->filename);
  output(" -> ");
  output(buff);
  strcat(buff, pkgname);
  strcat(buff, ".lsm");
  unzip_result = zip_unzip(zipfd, ziplinkedlist, buff);
  outputnl("");
  if (unzip_result != 0) {
    kitten_printf(10, 4, unzip_result); /* "ERROR: unzip failure (%d)" */
    outputnl("");
    return(-1);
  }
  filesextractedsuccess++;

  /* open the (freshly created) LSM file */
  lsmfd = fopen(buff, "ab"); /* opening in APPEND mode so I do not loose the LSM content */
  if (lsmfd == NULL) {
    kitten_printf(3, 10, buff); /* "ERROR: Could not create %s!" */
    outputnl("");
    return(-2);
  }
  fprintf(lsmfd, "\r\n"); /* in case the LSM does not end with a clear line already */

  /* write list of files in zip into the lst, and create the directories structure */
  for (curzipnode = ziplinkedlist->nextfile; curzipnode != NULL; curzipnode = curzipnode->nextfile) {

    /* substitute paths to custom dirs */
    shortfile = computelocalpath(curzipnode->filename, buff, dosdir, dirlist, bootdrive);

    /* log the filename to LSM metadata file + its CRC */
    fprintf(lsmfd, "%s%s?%08lX\r\n", buff, shortfile, curzipnode->crc32);

    /* create the path, just in case it doesn't exist yet */
    mkpath(buff);
    sprintf(fulldestfilename, "%s%s", buff, shortfile);

    /* Now unzip the file */
    output(curzipnode->filename);
    output(" -> ");
    output(buff);
    unzip_result = zip_unzip(zipfd, curzipnode, fulldestfilename);
    outputnl("");
    if (unzip_result != 0) {
      kitten_printf(10, 4, unzip_result); /* "ERROR: unzip failure (%d)" */
      outputnl("");
      filesextractedfailure += 1;
    } else {
      filesextractedsuccess += 1;
    }
  }
  fclose(lsmfd);

  kitten_printf(3, 19, pkgname, filesextractedfailure, filesextractedsuccess); /* "Package %s installed: %ld errors, %ld files extracted." */
  outputnl("");

  /* scan the LSM file for a "warn" message to display */
  display_warn_if_exists(pkgname, dosdir, buff, sizeof(buff));

  return(filesextractedfailure);
}
