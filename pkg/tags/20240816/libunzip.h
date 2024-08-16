/*
 * This file is part of the FDNPKG project
 * http://fdnpkg.sourceforge.net
 *
 * Copyright (C) 2012-2016 Mateusz Viste. All rights reserved.
 *
 * Simple library providing functions to unzip files from zip archives.
 */


#ifndef libunzip_sentinel
#define libunzip_sentinel

#include <time.h>  /* required for the time_t definition */

#define ZIP_FLAG_ISADIR    1
#define ZIP_FLAG_ENCRYPTED 2

#define ZIP_METH_STORE 0
#define ZIP_METH_DEFLATE 8

struct ziplist {
  long filelen;
  long compressedfilelen;
  unsigned long crc32;
  long dataoffset;      /* offset in the file where compressed data starts */
  struct ziplist *nextfile;
  time_t timestamp;     /* the timestamp of the file */
  short compmethod;     /* compression method (ZIP_METH_xxx) */
  unsigned char flags;  /* see ZIP_FLAG_xxx above */
  char filename[1];     /* must be last element (gets expanded at runtime) */
};

struct ziplist *zip_listfiles(FILE *fd);
int zip_unzip(FILE *zipfd, struct ziplist *curzipnode, const char *fulldestfilename);
void zip_freelist(struct ziplist **ziplist);

#endif
