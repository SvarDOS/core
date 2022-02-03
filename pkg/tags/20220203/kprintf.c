/*
 * This file provides dummy functions that simulate kitten-enabled routines
 * without actually having kitten.
 *
 * Copyright (C) 2015-2022 Mateusz Viste
 */

#include <stdio.h>    /* vprintf() */
#include <stdarg.h>   /* va_list, va_start()... */

#include "svarlang.lib\svarlang.h"

#include "kprintf.h"

void kitten_printf(short x, short y, ...) {
  va_list args;
  va_start(args, y);
  vprintf(svarlang_str(x, y), args);
  va_end(args);
}
