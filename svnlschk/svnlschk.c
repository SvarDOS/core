/*
 * svnlschk - Svarog's NLS checker
 * Copyright (C) 2016 Mateusz Viste
 *
 * analyzes a Svarog386 package and outputs a single integer number:
 *  -1 error (returns a non-zero errorlevel, too)
 *   0 no NLS data
 *   x any positive number = number of strings for given language
 *
 * usage: svnlschk pkg.zip lang filetype
 *
 * note: this program relies on the availability of info-zip's UNZIP.
 */

#include <stdio.h>
#include <stdlib.h> /* system() */
#include <string.h>
#include <libgen.h> /* basename() */


/* trims the extension part of a filename */
static void trimext(char *s) {
  char *lastdot = NULL;
  /* find last dot, if any */
  for (;*s != 0; s++) if (*s == '.') lastdot = s;
  /* trim last dot, if any */
  if (lastdot != NULL) *lastdot = 0;
}

/* trim trailing and white spaces */
static void trim(char *str) {
  int x, y, firstchar = -1, lastchar = -1;
  /* trim out the UTF-8 BOM, if present */
  if (((unsigned char)str[0] == 0xEF) && ((unsigned char)str[1] == 0xBB) && ((unsigned char)str[2] == 0xBF)) {
    memmove(str, str + 3, strlen(str + 3) + 1);
  }
  /* */
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

/* count the number of strings in a CATS-style nls file */
static long countnlsstrings(char *file, int filetype) {
  /* filetype 0 = NLS file ; 1 = COMMAND-style LNG file */
  long res = 0;
  int stringongoing = 0;
  char line[1024];
  while (*file != 0) {
    int i = 0;
    for (;; file++) {
      if (*file == '\r') continue;
      if (*file == '\n') {
        line[i] = 0;
        file++;
        break;
      }
      if (*file == 0) {
        line[i] = 0;
        break;
      }
      line[i] = *file;
      if (i < 1023) i++;
    }
    trim(line);
    /* skip empty lines and comments */
    if ((line[0] == 0) || (line[0] == '#')) continue;
    /* */
    if (filetype == 0) { /* CATS-like */
      res++;
    } else if (filetype == 1) { /* lng (FreeCOM) */
      if ((line[0] == ':') && (stringongoing == 0)) stringongoing = 1;
      if ((line[0] == '.') && (stringongoing != 0)) {
        res++;
        stringongoing = 0;
      }
    } else { /* err (FreeCOM) */
      res++;
    }
  }
  return(res);
}

int main(int argc, char **argv) {
  char buff[512];
  char *lang;
  char *pkgfile;
  char pkgshortname[64];
  char *file;
  long filelen;
  long result;
  int filetype;
  int popenres;
  #define FILEALLOC 1024*1024
  FILE *fd;
  /* read arg list */
  if ((argc != 4) || (argv[1][0] == '-') || (argv[3][0] > '2') || (argv[3][0] < '0')) {
    printf("-1\n");
    fprintf(stderr, "svnlschk - Svarog's NLS checker - Copyright (C) 2016 Mateusz Viste\n"
                    "usage: svnlschk pkg.zip lang nlstype\n"
                    "\n"
                    "where nlstype is:\n"
                    " 0 = standard CATS-like NLS file\n"
                    " 1 = FreeCOM-style LNG file\n"
                    " 2 = FreeCOM-style ERR file\n");
    return(1);
  }
  pkgfile = argv[1];
  lang = argv[2];
  filetype = atoi(argv[3]);
  strcpy(buff, pkgfile);
  snprintf(pkgshortname, sizeof(pkgshortname), basename(buff));
  trimext(pkgshortname);
  /* is this a valid zip archive? */
  snprintf(buff, sizeof(buff), "unzip -qq -t %s", pkgfile);
  if (system(buff) != 0) {
    printf("-1\n");
    fprintf(stderr, "ERROR: %s is not a valid ZIP archive!\n", pkgfile);
    return(1);
  }
  /* extract wanted file through popen() and read it into memory */
  if (filetype == 0) { /* CATS-like */
    snprintf(buff, sizeof(buff), "unzip -pC %s nls/%s.%s", pkgfile, pkgshortname, lang);
  } else if (filetype == 1) { /* lng (FreeCOM) */
    snprintf(buff, sizeof(buff), "unzip -pC %s source/%s/strings/%s.lng", pkgfile, pkgshortname, lang);
  } else { /* err (FreeCOM) */
    snprintf(buff, sizeof(buff), "unzip -pC %s source/%s/strings/%s.err", pkgfile, pkgshortname, lang);
  }
  /* */
  fd = popen(buff, "r");
  if (fd == NULL) {
    printf("-1\n");
    fprintf(stderr, "ERROR: popen() failed\n");
    return(1);
  }
  file = malloc(FILEALLOC);
  if (file == NULL) {
    printf("-1\n");
    fprintf(stderr, "ERROR: out of memory\n");
    return(1);
  }
  filelen = fread(file, 1, FILEALLOC - 1, fd);
  file[filelen] = 0; /* make sure to NULL-terminate */
  result = countnlsstrings(file, filetype);
  free(file);
  popenres = WEXITSTATUS(pclose(fd));
  if (popenres != 0) {
    printf("0\n");
    return(0);
  }
  printf("%ld\n", result);
  return(0);
}
