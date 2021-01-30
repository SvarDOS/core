/*
 * fileexists checks whether a file exists or not.
 * returns 0 if file not found, non-zero otherwise.
 */

#include <stdio.h>
#include "fileexst.h"

int fileexists(const char *filename) {
  FILE *fd;
  fd = fopen(filename, "rb");
  if (fd == NULL) return(0); /* file does not exists */
  fclose(fd);
  return(1);
}
