/*
 * translates a binary file to a C include.
 * used by the SvarCOM build process to embedd rcom inside COMMAND.COM
 *
 * Copyright (C) 2021 Mateusz Viste
 */

#include <stdio.h>

int main(int argc, char **argv) {
  FILE *fdin, *fdout;
  unsigned long len;
  int c;

  if (argc != 4) {
    puts("usage: file2c infile.dat outfile.c varname");
    return(1);
  }

  fdin = fopen(argv[1], "rb");
  if (fdin == NULL) {
    puts("ERROR: failed to open input file");
    return(1);
  }

  fdout = fopen(argv[2], "wb");
  if (fdout == NULL) {
    fclose(fdin);
    puts("ERROR: failed to open output file");
    return(1);
  }

  fprintf(fdout, "const char %s[] = {", argv[3]);
  for (len = 0;; len++) {
    c = getc(fdin);
    if (c == EOF) break;
    if (len > 0) fprintf(fdout, ",");
    if ((len & 15) == 0) fprintf(fdout, "\r\n");
    fprintf(fdout, "%3u", c);
  }
  fprintf(fdout, "};\r\n");
  fprintf(fdout, "#define %s_len %lu\r\n", argv[3], len);

  fclose(fdin);
  fclose(fdout);
  return(0);
}
