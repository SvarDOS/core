/* This file is part of the SvarCOM project and is published under the terms
 * of the MIT license.
 *
 * Copyright (C) 2021-2024 Mateusz Viste
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifndef CRT_H
#define CRT_H

/*
 * FileInfoRec (DTA) format:
 * offset size desc
 *    +0   21  reserved
 *  +15h    1  file attr (1=RO 2=Hidden 4=System 8=VOL 16=DIR 32=Archive
 *  +16h    2  time: bits 0-4=bi-seconds (0-30), bits 5-10=minutes (0-59), bits 11-15=hour (0-23)
 *  +18h    2  date: bits 0-4=day(0-31), bits 5-8=month (1-12), bits 9-15=years since 1980
 *  +1ah    4  DWORD file size, in bytes
 *  +1eh   13  13-bytes max ASCIIZ filename
 */
_Packed struct DTA {
  char reserved[21];
  unsigned char attr;
  unsigned short time_sec2:5;
  unsigned short time_min:6;
  unsigned short time_hour:5;
  unsigned short date_dy:5;
  unsigned short date_mo:4;
  unsigned short date_yr:7;
  unsigned long size;
  char fname[13];
};

#define PSP_PTR(ofs)MK_FP(crt_psp,(ofs))

/* segment (frame) number of our PSP */
extern unsigned short crt_psp;

/* pointer to command line character array (COM: PSP:80h, EXE: _DATA:crt_cmdline)
   WHY TEMP? because it is shared with the crt_temp_dta buffer,
   MEANING crt_temp_cmdline IS OVERWRITTEN AS SOON an operation that requires a DTA
   is performed. */
extern char *crt_temp_cmdline;

/* pointer to default DTA (COM: PSP:80h, EXE: _DATA:crt_dta)
   WHY TEMP? because it is shared with the crt_temp_cmdline buffer */
extern struct DTA *crt_temp_dta; 

void crt_exit(int exitcode);

#endif
