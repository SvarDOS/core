
#ifndef LIBC_H
#define LIBC_H

unsigned short mdr_dos_resizeblock(unsigned short siz, unsigned short segn);
unsigned short mdr_dos_write(unsigned short handle, const void far *buf, unsigned short count, unsigned short *bytes);

#endif
