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

#define INVALID_HANDLE_VALUE ((HANDLE)-1)

#define FILE_ATTRIBUTE_READONLY  0x0001
#define FILE_ATTRIBUTE_HIDDEN    0x0002
#define FILE_ATTRIBUTE_SYSTEM    0x0004
#define FILE_ATTRIBUTE_LABEL     0x0008
#define FILE_ATTRIBUTE_DIRECTORY 0x0010
#define FILE_ATTRIBUTE_ARCHIVE   0x0020

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
  DWORD dwFileAttributes;
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


typedef struct FFDTA  /* same format as a ffblk struct */
{
  BYTE reserved[21]; /* dos positioning info */
  BYTE ff_attrib;    /* file attributes */
  WORD ff_ftime;     /* time when file created/modified */
  WORD ff_fdate;     /* date when file created/modified */
  DWORD ff_fsize;    /* low word followed by high word */
  BYTE ff_name[13];  /* file name, not space padded, period, '\0' terminated, wildcards replaced */
} FFDTA;


#define FINDFILELFN 1
#define FINDFILEOLD 0

typedef union FHND  /* Stores either a handle (LFN) or FFDTA (oldstyle) */
{
  WORD handle;
  FFDTA *ffdtaptr;
} FHND;

typedef struct FindFileStruct
{
  short flag;        /* indicates whether this is for the old or new style find file & thus contents */
  FHND fhnd;         /* The data stored */
} FindFileStruct;

typedef FindFileStruct *HANDLE;

HANDLE FindFirstFile(const char *pathname, struct WIN32_FIND_DATA *findData);
int FindNextFile(HANDLE hnd, struct WIN32_FIND_DATA *findData);
void FindClose(HANDLE hnd);

DWORD GetFileAttributes(const char *pathname);

/* Only the 1st 4 arguments are used and returns zero on error */
int GetVolumeInformation(char *lpRootPathName,char *lpVolumeNameBuffer,
  DWORD nVolumeNameSize, DWORD *lpVolumeSerialNumber,
  DWORD *lpMaximumComponentLength, DWORD *lpFileSystemFlags,
  char *lpFileSystemNameBuffer, DWORD nFileSystemNameSize);


/* If this variable is nonzero then will 1st attempt LFN findfirst
 * (findfirst calls sets flag, so findnext/findclose know proper method to continue)
 * else if 0 then only attempt old 0x4E findfirst.
 * This is mostly a debugging tool, may be useful during runtime.
 * Default is LFN_ENABLE.
 */
#define LFN_ENABLE 1
#define LFN_DISABLE 0
extern int LFN_Enable_Flag;

#endif
