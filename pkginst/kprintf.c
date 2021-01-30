/*
 * This file provides dummy functions that simulate kitten-enabled routines
 * without actually having kitten.
 *
 * Copyright (C) 2015-2021 Mateusz Viste
 */

#include <stdio.h>    /* vprintf() */
#include <stdarg.h>   /* va_list, va_start()... */

#include "kitten/kitten.h"

#include "kprintf.h"

void kitten_printf(short x, short y, char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  vprintf(kittengets(x, y, fmt), args);
  va_end(args);
}

void kitten_puts(short x, short y, char *fmt) {
  puts(kittengets(x, y, fmt));
}
