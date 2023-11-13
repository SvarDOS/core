
#ifndef LIBC_H
#define LIBC_H

#include <stddef.h>

size_t strlen(const char *s);
void bzero(void *ptr, size_t len);
void fmemmove(void far *dst, const void far *src, size_t len);
unsigned short mdr_dos_fopen(const char *fname, unsigned short *fhandle);
unsigned short mdr_dos_fclose(unsigned short handle);
unsigned short _dos_freemem(unsigned short segn);
unsigned short mdr_dos_allocmem(unsigned short siz);
unsigned short mdr_dos_resizeblock(unsigned short siz, unsigned short segn);
unsigned short mdr_dos_read(unsigned short handle, void far *buf, unsigned short count, unsigned short *bytes);

unsigned short mdr_dos_write(unsigned short handle, const void far *buf, unsigned short count, unsigned short *bytes);

#endif
