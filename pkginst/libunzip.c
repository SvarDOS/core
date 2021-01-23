/*
 * This file is part of the FDNPKG project
 * http://fdnpkg.sourceforge.net
 *
 * Copyright (C) 2012-2016 Mateusz Viste. All rights reserved.
 *
 * Simple library providing functions to unzip files from zip archives.
 */

#include <stdio.h>     /* printf(), FILE, fclose()... */
#include <stdlib.h>    /* NULL */
#include <string.h>    /* memset() */
#include <time.h>      /* mktime() */
#include <utime.h>     /* utime() */
#include <unistd.h>   /* unlink() */

#include "crc32.h"
#include "kprintf.h"
#include "parsecmd.h"
#ifndef NOLZMA
  #include "lzmadec.h"   /* LZMA support */
#endif
#include "inf.h"   /* DEFLATE support */
#include "version.h"

#include "libunzip.h"  /* include self for control */


/* converts a "DOS format" timestamp into unix timestamp. The DOS timestamp is constructed an array of 4 bytes, that contains following data at the bit level:
 * HHHHHMMM MMMSSSSS YYYYYYYM MMMDDDDD
 *  where:
 * day of month is always within 1-31 range;
 * month is always within 1-12 range;
 * year starts from 1980 and continues for 127 years
 * seconds are actually not 0-59 but rather 0-29 as there are only 32 possible values – to get actual seconds multiply this field by 2;
 * minutes are always within 0-59 range;
 * hours are always within 0-23 range.     */
static time_t dostime2unix(unsigned char *buff) {
  struct tm curtime;
  time_t result;
  memset(&curtime, 0, sizeof(curtime)); /* make sure to set everything in curtime to 0's */
  curtime.tm_sec = (buff[0] & 31) << 1; /* seconds (0..60) */
  curtime.tm_min = (((buff[1] << 8) | buff[0]) >> 5) & 63 ; /* minutes after the hour (0..59) */
  curtime.tm_hour = (buff[1] >> 3); /* hours since midnight (0..23) */
  curtime.tm_mday = buff[2] & 31; /* day of the month (1..31) */
  curtime.tm_mon = ((((buff[3] << 8) | buff[2]) >> 5) & 15) - 1; /* months since January (0, 11) */
  curtime.tm_year = (buff[3] >> 1) + 80; /* years since 1900 */
  curtime.tm_wday = 0; /* days since Sunday (0..6) - leave 0, mktime() will set it */
  curtime.tm_yday = 0; /* days since January 1 (0..365]) - leave 0, mktime() will set it */
  curtime.tm_isdst = -1; /* Daylight Saving Time flag. Positive if DST is in effect, zero if not and negative if no information is available */
  result = mktime(&curtime);
  if (result == (time_t)-1) return(0);
  return(result);
}


#ifndef NOLZMA
/* this is a wrapper on malloc(), used as a callback by lzmadec */
static void *SzAlloc(void *p, size_t size) {
  p = p; /* for gcc to not complain */
  if (size == 0) return(0);
  return(malloc(size));
}

/* this is a wrapper on free(), used as a callback by lzmadec */
static void SzFree(void *p, void *address) {
  p = p;  /* for gcc to not complain */
  free(address);
}
#endif



/* opens a zip file and provides the list of files in the archive.
   returns a pointer to a ziplist (linked list) with all records, or NULL on error.
   The ziplist is allocated automatically, and must be freed via zip_freelist. */
struct ziplist *zip_listfiles(FILE *fd) {
  struct ziplist *reslist = NULL;
  struct ziplist *newentry;
  unsigned long entrysig;
  unsigned short filenamelen, extrafieldlen, filecommentlen;
  unsigned long compfilelen;
  int centraldirectoryfound = 0;
  unsigned int ux;
  unsigned char hdrbuff[64];

  rewind(fd);  /* make sure the file cursor is at the very beginning of the file */

  for (;;) { /* read entry after entry */
    int x, eofflag;
    long longbuff;
    entrysig = 0;
    eofflag = 0;
    /* read the entry signature first */
    for (x = 0; x < 32; x += 8) {
      if ((longbuff = fgetc(fd)) == EOF) {
        eofflag = 1;
        break;
      }
      entrysig |= (longbuff << x);
    }
    if (eofflag != 0) break;
    /* printf("sig: 0x%08x\n", entrysig); */
    if (entrysig == 0x04034b50ul) { /* local file */
      unsigned int generalpurposeflags;
      /* read and parse the zip header */
      fread(hdrbuff, 1, 26, fd);
      /* read filename's length so I can allocate the proper amound of mem */
      filenamelen = hdrbuff[23];
      filenamelen <<= 8;
      filenamelen |= hdrbuff[22];
      /* create new entry and link it into the list */
      newentry = calloc(sizeof(struct ziplist) + filenamelen, 1);
      if (newentry == NULL) {
        kitten_puts(8, 0, "Out of memory!");
        zip_freelist(&reslist);
        break;
      }
      newentry->nextfile = reslist;
      newentry->flags = 0;
      reslist = newentry;
      /* read further areas of the header, and fill zip entry */
      generalpurposeflags = hdrbuff[3];  /* parse the general */
      generalpurposeflags <<= 8;         /* purpose flags and */
      generalpurposeflags |= hdrbuff[2]; /* save them for later */
      newentry->compmethod = hdrbuff[4] | (hdrbuff[5] << 8);
      newentry->timestamp = dostime2unix(&hdrbuff[6]);
      newentry->crc32 = 0;
      for (x = 13; x >= 10; x--) {
        newentry->crc32 <<= 8;
        newentry->crc32 |= hdrbuff[x];
      }
      newentry->compressedfilelen = 0;
      for (x = 17; x >= 14; x--) {
        newentry->compressedfilelen <<= 8;
        newentry->compressedfilelen |= hdrbuff[x];
      }
      newentry->filelen = 0;
      for (x = 21; x >= 18; x--) {
        newentry->filelen <<= 8;
        newentry->filelen |= hdrbuff[x];
      }
      extrafieldlen = hdrbuff[25];
      extrafieldlen <<= 8;
      extrafieldlen |= hdrbuff[24];
      /* printf("Filename len: %d / extrafield len: %d / compfile len: %ld / filelen: %ld\n", filenamelen, extrafieldlen, newentry->compressedfilelen, newentry->filelen); */
      /* check general purpose flags */
      if ((generalpurposeflags & 1) != 0) newentry->flags |= ZIP_FLAG_ENCRYPTED;
      /* parse the filename */
      for (ux = 0; ux < filenamelen; ux++) newentry->filename[ux] = fgetc(fd); /* store filename */
      if (newentry->filename[filenamelen - 1] == '/') newentry->flags |= ZIP_FLAG_ISADIR; /* if filename ends with / it's a dir. Note that ZIP forbids the usage of '\' in ZIP paths anyway */
      /* printf("Filename: %s (%ld bytes compressed)\n", newentry->filename, newentry->compressedfilelen); */
      newentry->dataoffset = ftell(fd) + extrafieldlen;
      /* skip rest of fields and data */
      fseek(fd, (extrafieldlen + newentry->compressedfilelen), SEEK_CUR);
    } else if (entrysig == 0x02014b50ul) { /* central directory */
      centraldirectoryfound = 1;
      /* parse header now */
      fread(hdrbuff, 1, 42, fd);
      filenamelen = hdrbuff[22] | (hdrbuff[23] << 8);
      extrafieldlen = hdrbuff[24] | (hdrbuff[25] << 8);
      filecommentlen = hdrbuff[26] | (hdrbuff[27] << 8);
      compfilelen = 0;
      for (x = 17; x >= 14; x--) {
        compfilelen <<= 8;
        compfilelen |= hdrbuff[x];
      }
      /* printf("central dir\n"); */
      /* skip rest of fields and data */
      fseek(fd, (filenamelen + extrafieldlen + compfilelen + filecommentlen), SEEK_CUR);
    } else if (entrysig == 0x08074b50ul) { /* Data descriptor header */
      /* no need to read the header we just have to skip it */
      fseek(fd, 12, SEEK_CUR); /* the header is 3x4 bytes (CRC + compressed len + uncompressed len) */
    } else { /* unknown sig */
      kitten_printf(8, 1, "unknown zip sig: 0x%08lx", entrysig);
      puts("");
      zip_freelist(&reslist);
      break;
    }
  }
  /* if we got no central directory record, the file is incomplete */
  if (centraldirectoryfound == 0) zip_freelist(&reslist);
  return(reslist);
}



/* unzips a file. zipfd points to the open zip file, curzipnode to the entry to extract, and fulldestfilename is the destination file where to unzip it. returns 0 on success, non-zero otherwise. */
int zip_unzip(FILE *zipfd, struct ziplist *curzipnode, char *fulldestfilename) {
  #define buffsize 32 * 1024l /* MUST be at least 32K */
  FILE *filefd;
  unsigned long cksum;
  int extract_res;
  unsigned char *buff;
  struct utimbuf filetimestamp;

  /* first of all, check we support the compression method */
  switch (curzipnode->compmethod) {
    case 0:  /* stored */
    case 8:  /* deflated */
#ifndef NOLZMA
    case 14: /* lzma */
#endif
      break;
    default: /* unsupported compression method, sorry */
      return(-1);
      break;
  }

  /* open the dst file */
  filefd = fopen(fulldestfilename, "wb");
  if (filefd == NULL) return(-2);  /* failed to open the dst file */

  /* allocate buffers for data I/O */
  buff = malloc(buffsize);
  if (buff == NULL) {
    fclose(filefd);
    unlink(fulldestfilename); /* remove the failed file once it is closed */
    return(-6);
  }

  if (fseek(zipfd, curzipnode->dataoffset, SEEK_SET) != 0) { /* set the reading position inside the zip file */
    free(buff);
    fclose(filefd);
    unlink(fulldestfilename); /* remove the failed file once it is closed */
    return(-7);
  }
  extract_res = -255;

  cksum = crc32_init(); /* init the crc32 */

  if (curzipnode->compmethod == 0) { /* if the file is stored, copy it over */
    long i, toread;
    extract_res = 0;   /* assume we will succeed */
    for (i = 0; i < curzipnode->filelen;) {
      toread = curzipnode->filelen - i;
      if (toread > buffsize) toread = buffsize;
      if (fread(buff, toread, 1, zipfd) != 1) extract_res = -3;   /* read a chunk of data */
      crc32_feed(&cksum, buff, toread); /* update the crc32 checksum */
      if (fwrite(buff, toread, 1, filefd) != 1) extract_res = -4; /* write data chunk to dst file */
      i += toread;
    }
  } else if (curzipnode->compmethod == 8) {  /* if the file is deflated, inflate it */
    extract_res = inf(zipfd, filefd, buff, &cksum, curzipnode->compressedfilelen);
#ifndef NOLZMA
  } else if (curzipnode->compmethod == 14) {  /* LZMA */
    #define lzmaoutbufflen 32768u
    long bytesread, bytesreadtotal = 0, byteswritetotal = 0;
    SizeT buffoutreslen;
    ISzAlloc g_alloc;
    ELzmaStatus lzmastatus;
    SRes lzmaresult;
    CLzmaDec lzmahandle;
    unsigned char lzmahdr[LZMA_PROPS_SIZE]; /* 5 bytes of properties */
    unsigned char *lzmaoutbuff;

    extract_res = -5; /* assume we will fail. if we don't - then we will update this flag */
    lzmaoutbuff = malloc(lzmaoutbufflen);
    if (lzmaoutbuff == NULL) {
      free(buff);
      fclose(filefd);   /* close the dst file */
      return(-33);
    }

    fread(lzmahdr, 4, 1, zipfd); /* load the 4 bytes long 'zip-lzma header */
    bytesreadtotal = 4; /* remember we read 4 bytes already */

    /* lzma properties should be exactly 5 bytes long. If it's not, it's either not valid lzma, or some version that wasn't existing yet when I wrote these words. Also, check that the lzma content is at least 9 bytes long and that our previous malloc() calls suceeded. */
    if ((lzmahdr[2] == 5) && (lzmahdr[3] == 0) && (curzipnode->compressedfilelen >= 9)) {

      extract_res = 0;  /* since we got so far, let's assume we will succeed now */

      g_alloc.Alloc = SzAlloc; /* these will be used as callbacks by lzma to manage memory */
      g_alloc.Free = SzFree;

      fread(lzmahdr, sizeof(lzmahdr), 1, zipfd); /* load the lzma header */
      bytesreadtotal += sizeof(lzmahdr);

      /* Note, that in a 'normal' lzma stream we would have now 8 bytes with the uncompressed length of the file. Here we don't. ZIP cut this information out, since it stores it already in its own header. */

      memset(&lzmahandle, 0, sizeof(lzmahandle)); /* reset the whole lzmahandle structure - not doing this leads to CRASHES!!! */
      LzmaDec_Init(&lzmahandle);
      lzmaresult = LzmaDec_Allocate(&lzmahandle, lzmahdr, LZMA_PROPS_SIZE, &g_alloc); /* forget not to LzmaDec_Free() later! */
      if (lzmaresult != 0) extract_res = -13;

      while (extract_res == 0) {
        bytesread = buffsize;
        if (bytesread > curzipnode->compressedfilelen - bytesreadtotal) bytesread = curzipnode->compressedfilelen - bytesreadtotal;
        buffoutreslen = lzmaoutbufflen;
        /* printf("Will read %d bytes from input stream\n", bytesread); */
        fread(buff, bytesread, 1, zipfd); /* read stuff from input stream */
        fseek(zipfd, 0 - bytesread, SEEK_CUR); /* get back to the position at the start of our chunk of data */
        lzmaresult = LzmaDec_DecodeToBuf(&lzmahandle, lzmaoutbuff, &buffoutreslen, buff, (SizeT *)&bytesread, LZMA_FINISH_ANY, &lzmastatus);
        bytesreadtotal += bytesread;
        /* printf("expanded %ld bytes into %ld (total read: %ld bytes)\n", (long)bytesread, (long)buffoutreslen, (long)bytesreadtotal); */
        fseek(zipfd, bytesread, SEEK_CUR); /* go forward to the position next to the input we processed */
        if (lzmaresult != SZ_OK) {
          extract_res = -20;
          if (lzmaresult == SZ_ERROR_DATA) extract_res = -21;        /* DATA ERROR */
          if (lzmaresult == SZ_ERROR_MEM) extract_res = -22;         /* MEMORY ALLOC ERROR */
          if (lzmaresult == SZ_ERROR_UNSUPPORTED) extract_res = -23; /* UNSUPPORTED PROPERTY */
          if (lzmaresult == SZ_ERROR_INPUT_EOF) extract_res = -24;   /* NEED MORE INPUT */
          break;
        }
        /* check that we haven't got TOO MUCH decompressed data, and trim if necessary. It happens that LZMA provides a few bytes more than it should at the end of the stream. */
        if (byteswritetotal + (long)buffoutreslen > curzipnode->filelen) {
          buffoutreslen = curzipnode->filelen - byteswritetotal;
        }
        byteswritetotal += buffoutreslen;
        fwrite(lzmaoutbuff, buffoutreslen, 1, filefd); /* write stuff to output file */
        crc32_feed(&cksum, lzmaoutbuff, buffoutreslen);
        /* if (lzmastatus == LZMA_STATUS_FINISHED_WITH_MARK) puts("lzma says we are done!"); */
        if ((lzmastatus == LZMA_STATUS_FINISHED_WITH_MARK) || (bytesreadtotal >= curzipnode->compressedfilelen)) {
          extract_res = 0; /* looks like we succeeded! */
          break;
        }
      }
      LzmaDec_Free(&lzmahandle, &g_alloc); /* this will free all the stuff we allocated via LzmaDec_Allocate() */
      /* printf("Processed %d bytes of input into %d bytes of output. CRC32: %08lX\n", bytesreadtotal, byteswritetotal, crc32); */
    }
    free(lzmaoutbuff);
#endif
  }

  /* clean up memory, close the dst file and terminates crc32 */
  free(buff);
  fclose(filefd);   /* close the dst file */
  crc32_finish(&cksum);

  /* printf("extract_res=%d / cksum_expected=%08lX / cksum_obtained=%08lX\n", extract_res, curzipnode->crc32, cksum); */
  if (extract_res != 0) {  /* was the extraction process successful? */
    unlink(fulldestfilename); /* remove the failed file */
    return(extract_res);
  }
  if (cksum != curzipnode->crc32) { /* is the crc32 ok after extraction? */
    unlink(fulldestfilename); /* remove the failed file */
    return(-9);
  }
  /* Set the timestamp of the new file to what was set in the zip file */
  filetimestamp.actime  = curzipnode->timestamp;
  filetimestamp.modtime = curzipnode->timestamp;
  utime(fulldestfilename, &filetimestamp);
  return(0);
}



/* Call this to free a ziplist computed by zip_listfiles() */
void zip_freelist(struct ziplist **ziplist) {
  struct ziplist *zipentrytobefreed;
  while (*ziplist != NULL) { /* iterate through the linked list and free all nodes */
    zipentrytobefreed = *ziplist;
    *ziplist = zipentrytobefreed->nextfile;
    /* free the node entry */
    free(zipentrytobefreed);
  }
  *ziplist = NULL;
}
