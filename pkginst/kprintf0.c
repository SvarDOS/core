/*
 * This file provides dummy functions that simulate kitten-enabled routines
 * without actually having kitten.
 *
 * Copyright (C) 2015-2016 Mateusz Viste
 */

#include <stdio.h>    /* vprintf() */
#include <stdarg.h>   /* va_list, va_start()... */

#include "kprintf.h"

void kitten_printf(short x, short y, char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  vprintf(fmt, args);
  va_end(args);
  x = y;
}

void kitten_puts(short x, short y, char *fmt) {
  x = y;
  puts(fmt);
}
