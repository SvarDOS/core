/*
 * simple unzip tool that unzips the content of a zip archive to current directory
 * if listonly is set to a non-zero value then unzip() only lists the files
 * returns 0 on success
 *
 * this file is part of pkg (SvarDOS)
 * copyright (C) 2021-2024 Mateusz Viste
 */

#include <stdio.h>

#include "fileexst.h"
#include "helpers.h"
#include "libunzip.h"
#include "svarlang.lib\svarlang.h"

#include "unzip.h"


int unzip(const char *zipfile, unsigned char listonly, unsigned char *buff15k) {
  struct ziplist *zlist, *znode;
  FILE *fd;
  int r = 0;

  fd = fopen(zipfile, "rb");
  if (fd == NULL) {
    outputnl(svarlang_str(10, 1)); /* "ERROR: Failed to open the archive file" */
    return(1);
  }

  zlist = zip_listfiles(fd);
  if (zlist == NULL) {
    outputnl(svarlang_str(10, 2)); /* "ERROR: Invalid ZIP archive" */
    fclose(fd);
    return(-1);
  }

  if (listonly != 0) {
    /* just list the files inside the archive */
    for (znode = zlist; znode != NULL; znode = znode->nextfile) {
      outputnl(znode->filename);
    }
  } else {
    /* examine the list of zipped files - make sure that no file currently
     * exists and that files are neither encrypted nor compressed with an
     * unsupported method */
    for (znode = zlist; znode != NULL; znode = znode->nextfile) {
      int zres;
      /* convert slash to backslash, print filename and create the directories path */
      slash2backslash(znode->filename);
      printf("%s ", znode->filename);
      mkpath(znode->filename);
      /* if a dir, we good already */
      if (znode->flags == ZIP_FLAG_ISADIR) goto OK;
      /* file already exists? */
      if (fileexists(znode->filename) != 0) {
        outputnl(svarlang_str(10, 3)); /* "ERROR: File already exists" */
        r = 1;
        continue;
      }
      /* uncompress */
      zres = zip_unzip(fd, znode, znode->filename, buff15k);
      if (zres != 0) {
        sprintf(buff15k, svarlang_str(10,4), zres);
        outputnl(buff15k);
        continue;
      }
      OK:
      outputnl(svarlang_str(10, 0)); /* "OK" */
    }
  }

  zip_freelist(&zlist);
  fclose(fd);
  return(r);
}
