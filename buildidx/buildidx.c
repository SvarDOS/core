/*
  FDNPKG idx builder
  Copyright (C) Mateusz Viste 2012, 2013, 2014, 2015, 2016, 2017

  buildidx computes idx files for FDNPKG-compatible repositories.
  it must be executed pointing to a directory that stores FreeDOS
  packages (zip) files. buildidx will generate the index file and
  save it into the package repository.

  23 apr 2017: uncompressed index is no longer created, added CRC32 of zib (bin only) files, if present
  28 aug 2016: listing.txt is always written inside the repo dir (instead of inside current dir)
  27 aug 2016: accepting full paths to repos (starting with /...)
  07 dec 2013: rewritten buildidx in ANSI C89
  19 aug 2013: add a compressed version of the index file to repos (index.gz)
  22 jul 2013: creating a listing.txt file with list of packages
  18 jul 2013: writing the number of packaged into the first line of the lst file
  11 jul 2013: added a switch to 7za to make it case insensitive when extracting lsm files
  10 jul 2013: changed unzip calls to 7za (to handle cases when appinfo is compressed with lzma)
  04 feb 2013: added CRC32 support
  22 sep 2012: forked 1st version from FDUPDATE builder
*/

#include <errno.h>
#include <stdio.h>   /* fopen, fclose... */
#include <stdlib.h>  /* system() */
#include <string.h>  /* strcasecmp() */
#include <time.h>    /* time(), ctime() */
#include <unistd.h>  /* read() */
#include <dirent.h>
#include <sys/types.h>

#define pVer "2017-04-23"


#include "crc32lib.c"


/* computes the CRC32 of file and returns it. returns 0 on error. */
static unsigned long file2crc(char *filename) {
  unsigned long result;
  unsigned char buff[16 * 1024];
  int buffread;
  FILE *fd;
  fd = fopen(filename, "rb");
  if (fd == NULL) return(0);
  result = crc32_init();
  while ((buffread = fread(buff, 1, sizeof(buff), fd)) > 0) {
    if (buffread > 0) crc32_feed(&result, buff, buffread);
  }
  crc32_finish(&result);
  fclose(fd);
  if (buffread < 0) puts("read() error!");
  return(result);
}


static void trim(char *str) {
  int x, y, firstchar = -1, lastchar = -1;
  for (x = 0; str[x] != 0; x++) {
    switch (str[x]) {
      case ' ':
      case '\t':
      case '\n':
      case '\r':
        break;
      default:
        if (firstchar < 0) firstchar = x;
        lastchar = x;
        break;
    }
  }
  str[lastchar + 1] = 0; /* right trim */
  if (firstchar > 0) { /* left trim (shift to the left ) */
    y = 0;
    for (x = firstchar; str[x] != 0; x++) str[y++] = str[x];
    str[y] = 0;
  }
}


/* reads a line from a file descriptor, and writes it to *line, the *line array is filled no more than maxlen bytes. returns the number of byte read on success, or a negative value on failure (reaching EOF is considered an error condition) */
static int readline_fromfile(FILE *fd, char *line, int maxlen) {
  int bytebuff, linelen = 0;
  for (;;) {
    bytebuff = fgetc(fd);
    if (bytebuff == EOF) {
      line[linelen] = 0;
      if (linelen == 0) return(-1);
      return(linelen);
    }
    if (bytebuff < 0) return(-1);
    if (bytebuff == '\r') continue; /* ignore CR */
    if (bytebuff == '\n') {
      line[linelen] = 0;
      return(linelen);
    }
    if (linelen < maxlen) line[linelen++] = bytebuff;
  }
}

static int readlsm(char *filename, char *version, char *title, char *description) {
  char linebuff[1024];
  char *valuestr;
  int x;
  FILE *fd;
  /* reset fields to be read to empty values */
  version[0] = 0;
  description[0] = 0;
  title[0] = 0;
  /* open the file */
  fd = fopen(filename, "rb");
  if (fd == NULL) return(-1);
  /* check the file's header */
  if (readline_fromfile(fd, linebuff, 64) < 0) return(-1);
  if (strcasecmp(linebuff, "begin3") != 0) return(-1);
  /* read the LSM file line by line */
  while (readline_fromfile(fd, linebuff, 127) >= 0) {
    for (x = 0;; x++) {
      if (linebuff[x] == 0) {
        x = -1;
        break;
      } else if (linebuff[x] == ':') {
        break;
      }
    }
    if (x > 0) {
      linebuff[x] = 0;
      valuestr = linebuff + x + 1;
      trim(linebuff);
      trim(valuestr);
      if (strcasecmp(linebuff, "version") == 0) {
          sprintf(version, "%s", valuestr);
        } else if (strcasecmp(linebuff, "title") == 0) {
          sprintf(title, "%s", valuestr);
        } else if (strcasecmp(linebuff, "description") == 0) {
          sprintf(description, "%s", valuestr);
      }
    }
  }
  fclose(fd);
  return(0);
}


static int cmpstring(const void *p1, const void *p2) {
    /* The actual arguments to this function are "pointers to
       pointers to char", but strcmp(3) arguments are "pointers
       to char", hence the following cast plus dereference */
   return(strcasecmp(* (char * const *) p1, * (char * const *) p2));
}


static char *getlastdir(char *s) {
  char *r = s;
  for (; *s != 0; s++) {
    if ((*s == '/') && (s[1] != 0)) r = s+1;
  }
  return(r);
}


static void GenIndexes(char *repodir) {
  char *LsmFileList[4096];
  char tmpbuf[64];
  char *LsmFile, LSMpackage[64], LSMtitle[128], LSMversion[128], LSMdescription[1024];
  int LsmCount = 0, x;
  FILE *idx, *listing;
  time_t curtime;
  DIR *dir;
  struct dirent *diritem;

  dir = opendir("appinfo");
  if (dir == NULL) {
    printf("ERROR: Unable to open directory '%s' (%s)\n", repodir, strerror(errno));
    return;
  }

  /* load the content of the appinfo directory */
  while ((diritem = readdir(dir)) != NULL) {
    if (strstr(diritem->d_name, ".lsm") == NULL) continue; /* skip files that do not contain '.lsm' */
    LsmFileList[LsmCount] = strdup(diritem->d_name);
    /* printf("Loaded LSM file: %s\n", LsmFileList[LsmCount]); */
    LsmCount += 1;
  }

  closedir(dir);

  printf("Found %d LSM files\n", LsmCount);

  /* sort the entries */
  qsort(&LsmFileList[0], LsmCount, sizeof(char *), cmpstring);

  /* Create the index file */
  sprintf(tmpbuf, "%s/index.lst", repodir);
  idx = fopen(tmpbuf, "wb");
  sprintf(tmpbuf, "%s/listing.txt", repodir);
  listing = fopen(tmpbuf, "wb");

  /* Write out the index header */
  curtime = time(NULL);
  fprintf(idx, "FD-REPOv1\t'%s' built at unix time %ld, lists %d packages\n", getlastdir(repodir), curtime, LsmCount);
  fprintf(listing, "\n");
  fprintf(listing, "*** Repository '%s' - build time: %s\n", getlastdir(repodir), ctime(&curtime));

  /* Read every LSM */
  for (x = 0; x < LsmCount; x++) {
    unsigned long crc32, crc32zib;
    LsmFile = LsmFileList[x];
    sprintf(LSMpackage, "%s", LsmFile);
    LSMpackage[strlen(LSMpackage) - 4] = 0;

    /* compute the CRC of the zip package, and its zib version, if present */
    sprintf(tmpbuf, "%s/%s.zip", repodir, LSMpackage);
    crc32 = file2crc(tmpbuf);
    sprintf(tmpbuf, "%s/%s.zib", repodir, LSMpackage);
    crc32zib = file2crc(tmpbuf);

    if (crc32zib != 0) {
      printf("Processing %s... CRC %08lX (zib: %08lX)\n", LsmFile, crc32, crc32zib);
    } else {
      printf("Processing %s... CRC %08lX\n", LsmFile, crc32);
    }

    sprintf(tmpbuf, "appinfo/%s", LsmFile);
    readlsm(tmpbuf, LSMversion, LSMtitle, LSMdescription);

    if (strlen(LSMpackage) > 8) printf("Warning: %s.zip is not in 8.3 format!\n", LSMpackage);
    if (LSMtitle[0] == 0) printf("Warning: no LSM title for %s.zip\n", LSMpackage);
    if (LSMversion[0] == 0) printf("Warning: no LSM version for %s.zip!\n", LSMpackage);
    if (LSMdescription[0] == 0) printf("Warning: no LSM description for %s.zip!\n", LSMpackage);
    if (crc32zib != 0) {
      fprintf(idx, "%s\t%s\t%s\t%08lX\t%08lX\n", LSMpackage, LSMversion, LSMdescription, crc32, crc32zib);
    } else {
      fprintf(idx, "%s\t%s\t%s\t%08lX\t\n", LSMpackage, LSMversion, LSMdescription, crc32);
    }
    fprintf(listing, "%s %s - %s\n", LSMpackage, LSMversion, LSMdescription);
  }
  fprintf(listing, "\n");
  fclose(idx);
  fclose(listing);
  /* create the compressed version of the index file using gzip */
  sprintf(tmpbuf, "gzip -9 < %s/index.lst > %s/index.gz", repodir, repodir);
  system(tmpbuf);
  /* remove the uncompressed version */
  sprintf(tmpbuf, "%s/index.lst", repodir);
  unlink(tmpbuf);
  printf("%d packages found.\n", LsmCount);
}



int main(int argc, char **argv) {
  char *repodir;
  char cmdbuff[1024];

  puts("FDNPKG server repository generator version " pVer);

  if (argc != 2) {
    puts("Usage: buildidx repodir");
    return(1);
  }

  if ((argv[1][0] == '?') || (argv[1][0] == '-') || (argv[1][0]) == 0) {
    puts("Usage: buildidx repodir");
    return(1);
  }

  repodir = argv[1];

  printf("Will build index for %s directory.\n", repodir);
  puts("Recreating the appinfo directory...");

  system("rm -rf appinfo");
  system("mkdir appinfo");

  puts("Populating appinfo with LSM files from archives...");
  sprintf(cmdbuff, "unzip -C -j -L -o '%s/*.zip' 'appinfo/*.lsm' -d appinfo", repodir);
  system(cmdbuff);

  puts("Generating the index file...");
  GenIndexes(repodir);

  system("rm -rf appinfo");
  return(0);
}
