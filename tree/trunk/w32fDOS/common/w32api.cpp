/****************************************************************************

  Win32 For DOS compatibility API.

  Written by: Kenneth J. Davis
  Date:       May, 2004
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

#include "w32fdos.h"  /* File API and some types */
#include "wincon.h"   /* console functions */

#include <string.h>   /* strcpy and friends */
#include <dos.h>      /* REGS and intdos */
#include <stdio.h>

/* retrieve attributes (ReadOnly/System/...) about file or directory
 * returns (DWORD)-1 on error
 */
DWORD GetFileAttributes(const char *pathname)
{
  union REGS r;
  struct SREGS s;

  /* 1st try LFN - Extended get/set attributes (in case LFN used) */
  if (LFN_Enable_Flag)
  {
    r.x.ax = 0x7143;                  /* LFN API, Extended Get/Set Attributes */
    r.x.bx = 0x00;                    /* BL=0, get file attributes            */
    r.x.dx = FP_OFF(pathname);        /* DS:DX points to ASCIIZ filename      */

    segread(&s);                      /* load with current segment values     */
    s.ds = FP_SEG(pathname);          /* get Segment of our filename pointer  */

    r.x.cflag = 1;                    /* should be set when unsupported ***   */
    asm stc;                          /* but clib usually ignores on entry    */

    /* Actually perform the call, carry should be set on error or unuspported */
    intdosx(&r, &r, &s);         /* Clib function to invoke DOS int21h call   */

    if (!r.x.cflag)              /* if carry not set then cx has desired info */
      return (DWORD)r.x.cx;
    /* else error other than unsupported LFN api or invalid function [FreeDOS]*/
    else if ((r.x.ax != 0x7100) || (r.x.ax != 0x01))
      return (DWORD)-1;
    /* else fall through to standard get/set file attribute call */
  }

  /* we must remove any slashes from end */
  int slen = strlen(pathname) - 1;  /* Warning, assuming pathname is not ""   */
  char buffer[260];
  strcpy(buffer, pathname);
  if ((buffer[slen] == '\\') || (buffer[slen] == '/')) /* ends in a slash */
  {
    /* don't remove from root directory (slen == 0),
     * ignore UNC paths as SFN doesn't handle them anyway
     * if slen == 2, then check if drive given (e.g. C:\)
     */
    if (slen && !(slen == 2 &&  buffer[1] == ':'))
      buffer[slen] = '\0';
  }
  /* return standard attributes */
  r.x.ax = 0x4300;                  /* standard Get/Set File Attributes */
  r.x.dx = FP_OFF(buffer);          /* DS:DX points to ASCIIZ filename      */
  segread(&s);                      /* load with current segment values     */
  s.ds = FP_SEG(buffer);            /* get Segment of our filename pointer  */
  intdosx(&r, &r, &s);              /* invoke the DOS int21h call           */

  //if (r.x.cflag) printf("ERROR getting std attributes of %s, DOS err %i\n", buffer, r.x.ax);
  if (r.x.cflag) return (DWORD)-1;  /* error obtaining attributes           */
  return (DWORD)(0x3F & r.x.cx); /* mask off any DRDOS bits     */
}


/* The functions below are to aid in determining if output is redirected */

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
HANDLE GetStdHandle(DWORD nStdHnd)
{
  /* We simply return one of the 5 preopened DOS handles,
     STDIN==0, STDOUT==1, STDERR==2, STDAUX==4, and STDPRN==5
  */
  switch (nStdHnd)
  {
    case STD_INPUT_HANDLE  : return (HANDLE)0x00;
    case STD_OUTPUT_HANDLE : return (HANDLE)0x01;
    case STD_ERROR_HANDLE  : return (HANDLE)0x02;
  }
  return INVALID_HANDLE_VALUE;
}


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
DWORD GetFileType(HANDLE hFile)
{
  union REGS r;

  r.x.ax = 0x4400;                 /* DOS 2+, IOCTL Get Device Info */
  r.x.bx = (unsigned short)hFile;  /* DOS file handle to obtain info about */

  /* We assume hFile is an opened DOS handle, & if invalid call should fail. */
  intdos(&r, &r);     /* Clib function to invoke DOS int21h call   */
  if (!r.x.cflag)     /* if carry not set then dx has desired info */
  {
    if (r.x.dx & 0x80) /* if bit 7 is set */
      return FILE_TYPE_CHAR;  /* a [character] device */
    else
    {
      if (r.x.dx & 0x8000)    /* file is remote */
        return FILE_TYPE_REMOTE;
      else
        return FILE_TYPE_DISK;  /* assume valid file handle */
    }
  }

  return FILE_TYPE_UNKNOWN;
}


/* console API functions */

/*
  Returns information about the console.
  Input:
       pConScrBufInfo is a pointer to a CONSOLE_SCREEN_BUFFER_INFO struct that is filled in.
  Output:
       returns 0 (FALSE) on any error.
       The origin is at (0,0); so width = right-left+1 and height = bottom-top+1.
       Note: current implementation only sets srWindow field).
*/
BOOL GetConsoleScreenBufferInfo(PCONSOLE_SCREEN_BUFFER_INFO pConScrBufInfo)
{
  /*
   The word (2bytes) at memory address 0x0040:004A contains the # of cols of current display mode.
   The word at 0x0040:004C contains the screen buffer length in bytes.
   The byte at 0x0040:0084 { *(unsigned char far *)MK_FP(0x40,0x84) } is the rows-1, EGA+ (may fail on PCjr & others).
  */
  unsigned short cols=80, rows=25;

  unsigned short far * bios_cols = (unsigned short far *)MK_FP(0x40,0x4A);
  unsigned short far * bios_size = (unsigned short far *)MK_FP(0x40,0x4C);

  if (*bios_cols)
  {
    cols = *bios_cols;
    if (*bios_size)
      rows = *bios_size / cols / 2;
  }
  /* else use default values of 25x80 screen */


  if (pConScrBufInfo)
  {
    /* dwSize and dwMaximumWindowSize refer to console buffer and may
       be logically larger than actual viewable window size. */
    pConScrBufInfo->dwSize.X = pConScrBufInfo->dwMaximumWindowSize.X = cols;
    pConScrBufInfo->dwSize.Y = pConScrBufInfo->dwMaximumWindowSize.Y = rows;
    /* supports scrolling in buffer, but also provides viewable window size */
    pConScrBufInfo->srWindow.Left = pConScrBufInfo->srWindow.Top = 0;
    pConScrBufInfo->srWindow.Right = cols - 1;
    pConScrBufInfo->srWindow.Bottom = rows - 1;

    /* TODO: cursor location and color of output text */
    pConScrBufInfo->dwCursorPosition.X = pConScrBufInfo->dwCursorPosition.Y = 0;
    pConScrBufInfo->wAttributes = 0;

    return 1;  /* TRUE, success */
  }
  else
    return 0;  /* FALSE on any errors */
}
