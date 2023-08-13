/*
 * Reading from and writing to an RS-232 port
 *
 * This file is part of the Mateusz' DOS Routines (MDR): http://mdr.osdn.io
 * Published under the terms of the MIT License, as stated below.
 *
 * Copyright (C) 2015-2022 Mateusz Viste
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#ifndef MDR_RS232_H
#define MDR_RS232_H

/* get the I/O port for COMx (1..4) */
unsigned short rs232_getport(int x);

/* check if the COM port is ready for write. loops for some time waiting.
 * returns 0 if port seems ready eventually, non-zero otherwise. can be used
 * to verify the rs232 presence */
int rs232_check(unsigned short port);

/* write a byte to the COM port at 'port'. this function will block if the
 * UART is not ready to transmit yet. */
void rs232_write(unsigned short port, int data);

/* read a byte from COM port at 'port'. returns the read byte, or -1 if
 * nothing was available to read. */
int rs232_read(unsigned short port);

#endif
