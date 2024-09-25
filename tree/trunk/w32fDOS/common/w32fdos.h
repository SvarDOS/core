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

#define FILE_ATTRIBUTE_DEVICE               0x0040
#define FILE_ATTRIBUTE_NORMAL               0x0080
#define FILE_ATTRIBUTE_TEMPORARY            0x0100
#define FILE_ATTRIBUTE_SPARSE_FILE          0x0200
#define FILE_ATTRIBUTE_REPARSE_POINT        0x0400
#define FILE_ATTRIBUTE_COMPRESSED           0x0800
#define FILE_ATTRIBUTE_OFFLINE              0x1000
#define FILE_ATTRIBUTE_NOT_CONTENT_INDEXED  0x2000
#define FILE_ATTRIBUTE_ENCRYPTED            0x4000

/* These two are used by FindFirstFileEx, NT specific */
typedef enum FINDEX_INFO_LEVELS { FindExInfoStandard, FindExInfoMaxInfoLevel } FINDEX_INFO_LEVELS;
typedef enum FINDEX_SEARCH_OPS
{
  FindExSearchNameMatch,
  FindExSearchLimitToDirectories,
  FindExSearchLimitToDevices,
  FindExSearchMaxSearchOp
} FINDEX_SEARCH_OPS;

typedef short BOOL;
typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned long DWORD;

typedef struct FILETIME   /* should correspond to a quad word */
{
  WORD ldw[2];  /* LowDoubleWord  */
  DWORD hdw;    /* HighDoubleWord */
} FILETIME;

typedef struct  WIN32_FIND_DATAA
{
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
} WIN32_FIND_DATAA;

typedef struct  WIN32_FIND_DATAW
{
  DWORD dwFileAttributes;
  FILETIME ftCreationTime;
  FILETIME ftLastAccessTime;
  FILETIME ftLastWriteTime;
  DWORD    nFileSizeHigh;
  DWORD    nFileSizeLow;
  DWORD    dwReserved0;
  DWORD    dwReserved1;
  WORD cFileName[ 130 /* 260 */ ];        /* Correct values are 260 & 14, but as Unicode */
  WORD cAlternateFileName[ 7 /* 14 */ ];  /* is unsupported, make size same as Ansi one  */
} WIN32_FIND_DATAW;

#define WIN32_FIND_DATA WIN32_FIND_DATAA

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

#define STDCALL

HANDLE STDCALL FindFirstFileA(const char *pathname, WIN32_FIND_DATAA *findData);
int STDCALL FindNextFileA(HANDLE hnd, WIN32_FIND_DATAA *findData);
void STDCALL FindClose(HANDLE hnd);

#define FindFirstFile FindFirstFileA
#define FindNextFile FindNextFileA

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


/* The functions below are to aid in determining if output is redirected */

#define STD_INPUT_HANDLE  ((DWORD)-10)
#define STD_OUTPUT_HANDLE ((DWORD)-11)
#define STD_ERROR_HANDLE  ((DWORD)-12)

/* returns the handle to specified standard device (standard input,
 * standard output, or standard error).
 * Input: one of the predefined values STD_INPUT_HANDLE,
 *        STD_OUTPUT_HANDLE, or STD_ERROR_HANDLE.
 * Ouput: a file HANDLE to specified device.
 *
 * Current implementation simply returns standard DOS handle #,
 * and does not ensure it is valid -- however, the handle returned
 * should be opened and in r/w mode [though if redirected may
 * wait indefinitely for data] if program exec'd via normal DOS
 * routine and not purposely closed prior to invoking this function.
 * TODO: should actually open CON device and return that handle.
 * WARNING: this handle should only be passed to GetFileType or
 * a DOS function that takes a file handle, it is NOT compatible
 * the HANDLE used with FindFile functions above.  Cast to int.
 */
HANDLE GetStdHandle(DWORD nStdHnd);


#define FILE_TYPE_UNKNOWN 0x0000
#define FILE_TYPE_DISK    0x0001
#define FILE_TYPE_CHAR    0x0002
#define FILE_TYPE_PIPE    0x0003
#define FILE_TYPE_REMOTE  0x8000

/* Returns file type.
 * Input, an opened file handle.
 * Output, one of predefined values above indicating if
 *         handle refers to file (FILE_TYPE_DISK), a
 *         device such as CON (FILE_TYPE_CHAR), a
 *         pipe (FILE_TYPE_PIPE), or unknown.
 * On errors or unspecified input, FILE_TYPE_UNKNOWN
 * is returned.  Under DOS, piped output is implemented
 * via a temp file, so FILE_TYPE_PIPE is never returned.
 */
DWORD GetFileType(HANDLE hFile);

#endif
