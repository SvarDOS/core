/*
 * $Id: crc32.h,v 1.1.1.1 1996/02/18 21:38:11 ylo Exp $
 * $Log: crc32.h,v $
 * Revision 1.1.1.1  1996/02/18 21:38:11  ylo
 *     Imported ssh-1.2.13.
 *
 * Revision 1.2  1995/07/13  01:21:45  ylo
 *     Removed "Last modified" header.
 *     Added cvs log.
 *
 * $Endlog$
 */

#ifndef CRC32_H_SENTINEL
#define CRC32_H_SENTINEL

unsigned long crc32_init(void);

/* This computes a 32 bit CRC of the data in the buffer, and returns the
   CRC.  The polynomial used is 0xedb88320. */
void crc32_feed(unsigned long *crc32val, const unsigned char *buf, unsigned int len);

void crc32_finish(unsigned long *crc32val);

#endif /* CRC32_H_SENTINEL */
