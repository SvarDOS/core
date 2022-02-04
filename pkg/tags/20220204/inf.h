
#ifndef INF_H
#define INF_H

/* Decompress from file source to file dest until stream ends or EOF.
 * inf() returns Z_OK on success, Z_MEM_ERROR if memory could not be allocated
 * for processing, Z_DATA_ERROR if the deflate data is invalid or incomplete,
 * Z_VERSION_ERROR if the version of zlib.h and the version of the library
 * linked do not match, or Z_ERRNO if there is an error reading or writing the
 * files. */
int inf(FILE *source, FILE *dest, unsigned char *buff32k, unsigned long *cksum, long streamlen);

#endif
