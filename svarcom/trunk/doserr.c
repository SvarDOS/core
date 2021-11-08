/* This file is part of the SvarCOM project and is published under the terms
 * of the MIT license.
 *
 * Copyright (C) 2021 Mateusz Viste
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

/*
 * translates a DOS extended error into a human string
 * (as defined by INT 21/AH=59h/BX=0000h)
 */

#include <stdio.h>

#include "doserr.h"

const char *doserr(unsigned short err) {
  static char buf[24];
  switch (err) {
    case 0x00: return("Success");
    case 0x01: return("Function number invalid");
    case 0x02: return("File not found");
    case 0x03: return("Path not found");
    case 0x04: return("Too many open files (no handles available)");
    case 0x05: return("Access denied");
    case 0x06: return("Invalid handle");
    case 0x07: return("Memory control block destroyed");
    case 0x08: return("Insufficient memory");
    case 0x09: return("Memory block address invalid");
    case 0x0A: return("Environment invalid");
    case 0x0B: return("Format invalid");
    case 0x0C: return("Access code invalid");
    case 0x0D: return("Data invalid");
    case 0x0F: return("Invalid drive");
    case 0x10: return("Attemted to remove current directory");
    case 0x11: return("Not same device");
    case 0x12: return("No more files");
    case 0x13: return("Disk write-protected");
    case 0x14: return("Unknown unit");
    case 0x15: return("Drive not ready");
    case 0x16: return("Unknown command");
    case 0x17: return("Data error (CRC)");
    case 0x18: return("Bad request structure length");
    case 0x19: return("Seek error");
    case 0x1A: return("Unknown media type (non-DOS disk)");
    case 0x1B: return("Sector not found");
    case 0x1C: return("Printer out of paper");
    case 0x1D: return("Write fault");
    case 0x1E: return("Read fault");
    case 0x1F: return("General failure");
    case 0x20: return("Sharing violation");
    case 0x21: return("Lock violation");
    case 0x22: return("Disk change invalid");
    case 0x23: return("FCB unavailable");
    case 0x24: return("Sharing buffer overflow");
    case 0x25: return("Code page mismatch");
    case 0x26: return("Cannot complete file operations (EOF / out of input)");
    case 0x27: return("Insufficient disk space");
    default:
      snprintf(buf, sizeof(buf), "DOS ERROR 0x%02X", err);
      return(buf);
  }
}
