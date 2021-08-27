/*
 * test suite for the unchunk() function.
 *
 * Copyright (C) 2021 Mateusz Viste
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "unchunk.h"

static size_t randchunkdata(void *file_chunked, const void *file_raw, size_t file_raw_len, int maxchunksz) {
  size_t file_raw_read = 0;
  size_t file_chunked_len = 0;

  for (;;) {
    size_t chunklen = (rand() % maxchunksz) + 1;
    if (file_raw_read + chunklen > file_raw_len) chunklen = file_raw_len - file_raw_read;

    file_chunked_len += sprintf((char *)file_chunked + file_chunked_len, "%x\r\n", chunklen);
    if (chunklen > 0) memcpy((char *)file_chunked + file_chunked_len, (char *)file_raw + file_raw_read, chunklen);
    file_raw_read += chunklen;
    file_chunked_len += chunklen;
    file_chunked_len += sprintf((char *)file_chunked + file_chunked_len, "\r\n");
    if (chunklen == 0) return(file_chunked_len);
  }
}


/* writes buffer of fsize bytes to file fname */
static void dumpfile(const char *fname, const void *buff, size_t fsize) {
  FILE *fd;
  fd = fopen(fname, "wb");
  fwrite(buff, 1, fsize, fd);
  fclose(fd);
}


int main(int argc, char **argv) {
  static unsigned char file_raw[30000u];     /* original file */
  static unsigned char file_chunked[60000u]; /* chunked version */
  static unsigned char file_decoded[60000u]; /* after being un-chunked */
  size_t file_raw_len;
  size_t file_chunked_len;
  FILE *fd;
  int trycount;

  if ((argc != 2) || (argv[1][0] == '/')) {
    puts("Usage: unchtest <file>");
    return(1);
  }

  fd = fopen(argv[1], "rb");
  if (fd == NULL) {
    puts("ERROR: failed to open file");
    return(1);
  }
  file_raw_len = fread(file_raw, 1, sizeof(file_raw), fd);
  fclose(fd);

  printf("Loaded '%s' (%zu bytes)\r\n", argv[1], file_raw_len);
  srand(time(NULL));

  for (trycount = 0; trycount < 1000; trycount++) {
    size_t bytesprocessed = 0;
    size_t file_decoded_len = 0;
    int maxchunksz;

    /* segment file into chunks of random size */
    maxchunksz = (rand() % 1024) + 1;
    file_chunked_len = randchunkdata(file_chunked, file_raw, file_raw_len, maxchunksz);

    printf("=== TRY %d (CHUNKS: %d BYTES MAX) ======================\r\n", trycount + 1, maxchunksz);

    for (;;) {
      size_t bytes;
      int decodedbytes;
      unsigned char buffer[4096];

      bytes = min((rand() % 256) + 1, file_chunked_len - bytesprocessed);
      printf("processing %4zu bytes of chunked data", bytes);
      memcpy(buffer, file_chunked + bytesprocessed, bytes);

      /* decode the chunked version reading random amounts of data and build a decoded version */
      decodedbytes = unchunk(buffer, bytes);
      printf(" -> decoded into %4d raw bytes\r\n", decodedbytes);
      memcpy(file_decoded + file_decoded_len, buffer, decodedbytes);
      file_decoded_len += decodedbytes;
      bytesprocessed += bytes;
      if (bytesprocessed == file_chunked_len) break;
    }

    /* compare decoded and original */
    if ((file_decoded_len != file_raw_len) || (memcmp(file_decoded, file_raw, file_raw_len) != 0)) {
      printf("ERROR: decoded file does not match the original. see tst-orig.dat, tst-chnk.txt and tst-unch.dat\r\n");
      dumpfile("tst-orig.dat", file_raw, file_raw_len);
      dumpfile("tst-chnk.dat", file_chunked, file_chunked_len);
      dumpfile("tst-unch.dat", file_decoded, file_decoded_len);
      return(1);
    }
  }

  printf("OK\r\n");

  return(0);
}
