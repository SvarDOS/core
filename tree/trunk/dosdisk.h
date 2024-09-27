/****************************************************************************

  Win32 File compatibility for DOS.
  [This version does support LFNs, if available.]

  Written by: Kenneth J. Davis
  Date:       August, 2000
  Contact:    jeremyd@computer.org


Copyright (c): Public Domain [United States Definition]

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR AUTHORS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.

****************************************************************************/

#ifndef W32FDOS_H
#define W32FDOS_H

#define FILE_A_RDONLY    0x0001
#define FILE_A_HIDDEN    0x0002
#define FILE_A_SYSTEM    0x0004
#define FILE_A_VOLID     0x0008
#define FILE_A_SUBDIR    0x0010
#define FILE_A_ARCH      0x0020


_Packed struct FFDTA { /* same format as a ffblk struct */
  char reserved[21];        /* dos positioning info */
  unsigned char attrib;     /* file attributes */
  unsigned short wr_ftime;  /* time when file created/modified */
  unsigned short wr_fdate;  /* date when file created/modified */
  unsigned long size;       /* low word followed by high word */
  char name[13];  /* file name, not space padded, period, '\0' terminated, wildcards replaced */
};


int FindFirstFile(const char *pathname, struct FFDTA *dta);

int FindNextFile(struct FFDTA *hnd);

void FindClose(struct FFDTA *hnd);

/* Only the 1st 4 arguments are used and returns zero on error */
int GetVolumeInformation(const char *lpRootPathName, char *lpVolumeNameBuffer, size_t nVolumeNameSize, unsigned long *lpVolumeSerialNumber);

#endif
