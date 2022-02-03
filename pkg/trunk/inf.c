/* This file is part of the FDNPKG project. It is an adaptation of the
 * "zpipe.c" example of zlib's inflate() and deflate() usage.
 *
 * Not copyrighted -- provided to the public domain
 * original version 1.4  11 December 2005  Mark Adler
 * adaptations for FDNPKG integration by Mateusz Viste, 2015
 */

#include <stdio.h>
#include <string.h>

#include "crc32.h"
#include "zlib\zlib.h"

#include "inf.h"

#define CHUNK 16384

/* Decompress from file source to file dest until stream ends or EOF.
 * inf() returns Z_OK on success, Z_MEM_ERROR if memory could not be allocated
 * for processing, Z_DATA_ERROR if the deflate data is invalid or incomplete,
 * Z_VERSION_ERROR if the version of zlib.h and the version of the library
 * linked do not match, or Z_ERRNO if there is an error reading or writing the
 * files. */
int inf(FILE *source, FILE *dest, unsigned char *buff32K, unsigned long *cksum, long streamlen) {
  int ret;
  unsigned int have;
  z_stream strm;
  unsigned char *in = buff32K;
  unsigned char *out = buff32K + CHUNK;

  /* allocate inflate state */
  strm.zalloc = Z_NULL;
  strm.zfree = Z_NULL;
  strm.opaque = Z_NULL;
  strm.avail_in = 0;
  strm.next_in = Z_NULL;
  ret = inflateInit2(&strm, -15); /* according to the zlib doc, passing -15 to inflateInit2() means "this is a raw deflate stream" (as opposed to a zlib- or gz- wrapped stream) */
  if (ret != Z_OK) return(ret);

  /* decompress until deflate stream ends or end of file */
  do {
    strm.avail_in = fread(in, 1, (streamlen > CHUNK ? CHUNK : streamlen), source);
    if (ferror(source)) {
      (void)inflateEnd(&strm);
      return(Z_ERRNO);
    }
    streamlen -= strm.avail_in;
    if (strm.avail_in == 0) break;
    strm.next_in = in;

    /* run inflate() on input until output buffer not full */
    do {
      strm.avail_out = CHUNK;
      strm.next_out = out;
      ret = inflate(&strm, Z_NO_FLUSH);
      switch (ret) {
        case Z_NEED_DICT:
          ret = Z_DATA_ERROR;     /* and fall through */
        case Z_DATA_ERROR:
        case Z_MEM_ERROR:
          (void)inflateEnd(&strm);
          return(ret);
      }
      have = CHUNK - strm.avail_out;
      if ((fwrite(out, 1, have, dest) != have) || (ferror(dest))) {
        (void)inflateEnd(&strm);
        return(Z_ERRNO);
      }
      /* feed the CRC32 */
      crc32_feed(cksum, out, have);
    } while (strm.avail_out == 0);

    /* done when inflate() says it's done */
  } while (ret != Z_STREAM_END);

  /* clean up and return */
  (void)inflateEnd(&strm);

  if (Z_STREAM_END) {
    return(Z_OK);
  } else {
    return(Z_DATA_ERROR);
  }
}
