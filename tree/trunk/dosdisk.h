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

#define INVALID_HANDLE_VALUE (NULL)

#define FILE_A_READONLY  0x0001
#define FILE_A_HIDDEN    0x0002
#define FILE_A_SYSTEM    0x0004
#define FILE_A_VOL       0x0008
#define FILE_A_DIR       0x0010
#define FILE_A_ARCH      0x0020

typedef short BOOL;
typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned long DWORD;

typedef struct FILETIME   /* should correspond to a quad word */
{
  WORD ldw[2];  /* LowDoubleWord  */
  DWORD hdw;    /* HighDoubleWord */
} FILETIME;

struct WIN32_FIND_DATA {
  unsigned short attrib;
  FILETIME ftCreationTime;
  FILETIME ftLastAccessTime;
  FILETIME ftLastWriteTime;
  DWORD    nFileSizeHigh;
  DWORD    nFileSizeLow;
  DWORD    dwReserved0;
  DWORD    dwReserved1;
  char cFileName[ 260 ];
  char cAlternateFileName[ 14 ];
};


_Packed struct FFDTA { /* same format as a ffblk struct */
  BYTE reserved[21]; /* dos positioning info */
  BYTE ff_attr;      /* file attributes */
  WORD ff_ftime;     /* time when file created/modified */
  WORD ff_fdate;     /* date when file created/modified */
  DWORD ff_fsize;    /* low word followed by high word */
  BYTE ff_name[13];  /* file name, not space padded, period, '\0' terminated, wildcards replaced */
};


struct FFDTA *FindFirstFile(const char *pathname, struct WIN32_FIND_DATA *findData);
int FindNextFile(struct FFDTA *hnd, struct WIN32_FIND_DATA *findData);
void FindClose(struct FFDTA *hnd);

int GetFileAttributes(unsigned short *attr, const char *pathname);

/* Only the 1st 4 arguments are used and returns zero on error */
int GetVolumeInformation(const char *lpRootPathName, char *lpVolumeNameBuffer,
  DWORD nVolumeNameSize, DWORD *lpVolumeSerialNumber);

#endif
