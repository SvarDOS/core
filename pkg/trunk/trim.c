/*
 * trims any space, tab, cr or lf
 * Copyright (C) 2012-2021 Mateusz Viste
 */

#include "trim.h"

void trim(char *str) {
  int x, y, firstchar = -1, lastchar = -1;

  /* find out first and last non-whitespace char */
  for (x = 0; str[x] != 0; x++) {
    switch (str[x]) {
      case ' ':
      case '\t':
      case '\n':
      case '\r':
        break;
      default:
        if (firstchar < 0) firstchar = x;
        lastchar = x;
        break;
    }
  }

  /* right trim */
  str[lastchar + 1] = 0;

  /* left trim (shift to the left ) */
  if (firstchar > 0) {
    y = 0;
    for (x = firstchar; str[x] != 0; x++) str[y++] = str[x];
    str[y] = 0;
  }
}
