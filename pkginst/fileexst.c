/*
 * fileexists checks whether a file exists or not.
 * returns 0 if file not found, non-zero otherwise.
 */

#include <stdio.h>
#include "fileexst.h"
#include "version.h"

int fileexists(char *filename) {
  FILE *fd;
  fd = fopen(filename, "rb");
  if (fd != NULL) { /* file exists */
      fclose(fd);
      return(1);
    } else {
      return(0);
  }
}
