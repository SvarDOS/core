/****************************************************************************

  Win32 For DOS compatibility API, console functions.

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

#ifndef WINCON_H
#define WINCON_H

typedef struct SMALL_RECT
{
  short Left, Top, Right, Bottom;
} SMALL_RECT;

typedef struct COORD
{
  short X,Y;
} COORD;

typedef struct CONSOLE_SCREEN_BUFFER_INFO
{
  COORD      dwSize;
  COORD      dwCursorPosition;
  WORD       wAttributes;
  SMALL_RECT srWindow;
  COORD      dwMaximumWindowSize;
} CONSOLE_SCREEN_BUFFER_INFO;
typedef CONSOLE_SCREEN_BUFFER_INFO * PCONSOLE_SCREEN_BUFFER_INFO;


/*
  Returns information about the console.
  Input: 
       hCon is the HANDLE (such as returned by GetStdHandle()) to the console buffer to obtain info about
       pConScrBufInfo is a pointer to a CONSOLE_SCREEN_BUFFER_INFO struct that is filled in.
*/
BOOL GetConsoleScreenBufferInfo(HANDLE hCon, PCONSOLE_SCREEN_BUFFER_INFO pConScrBufInfo);


#endif /* WINCON_H */
