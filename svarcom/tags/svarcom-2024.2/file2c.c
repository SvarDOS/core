/*
 * translates a binary file to a C include.
 * used by the SvarCOM build process to embedd rcom inside COMMAND.COM
 *
 * Copyright (C) 2021 Mateusz Viste
 */

#include <stdio.h>


static void help(void) {
  puts("usage: file2c [/c] [/lxxx] infile.dat outfile.c varname");
  puts("");
  puts("/c    - define the output array as CONST");
  puts("/s    - define the output array as STATIC");
  puts("/lxxx - enforces the output array to be xxx bytes big");
}


int main(int argc, char **argv) {
  char *fnamein = NULL, *fnameout = NULL, *varname = NULL;
  char stortype = 0; /* 'c' = const ; 's' = static */
  char *flag_l = "";
  FILE *fdin, *fdout;
  unsigned long len;
  int c;

  for (c = 1; c < argc; c++) {
    if ((argv[c][0] == '/') && (argv[c][1] == 'l')) {
      flag_l = argv[c] + 2;
      continue;
    }
    if ((argv[c][0] == '/') && (argv[c][1] == 'c')) {
      stortype = 'c';
      continue;
    }
    if ((argv[c][0] == '/') && (argv[c][1] == 's')) {
      stortype = 's';
      continue;
    }
    if (argv[c][0] == '/') {
      help();
      return(1);
    }
    /* not a switch - so it's either infile, outfile or varname */
    if (fnamein == NULL) {
      fnamein = argv[c];
    } else if (fnameout == NULL) {
      fnameout = argv[c];
    } else if (varname == NULL) {
      varname = argv[c];
    } else {
      help();
      return(1);
    }
  }

  if (varname == NULL) {
    help();
    return(1);
  }

  fdin = fopen(fnamein, "rb");
  if (fdin == NULL) {
    puts("ERROR: failed to open input file");
    return(1);
  }

  fdout = fopen(fnameout, "wb");
  if (fdout == NULL) {
    fclose(fdin);
    puts("ERROR: failed to open output file");
    return(1);
  }

  if (stortype == 'c') fprintf(fdout, "const ");
  if (stortype == 's') fprintf(fdout, "static ");
  fprintf(fdout, "char %s[%s] = {", varname, flag_l);

  for (len = 0;; len++) {
    c = getc(fdin);
    if (c == EOF) break;
    if (len > 0) fprintf(fdout, ",");
    if ((len & 15) == 0) fprintf(fdout, "\r\n");
    fprintf(fdout, "%3u", c);
  }
  fprintf(fdout, "};\r\n");
  fprintf(fdout, "#define %s_len %lu\r\n", varname, len);

  fclose(fdin);
  fclose(fdout);
  return(0);
}
