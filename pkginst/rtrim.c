/*
 * Right trim any space, tab, cr or lf
 * Copyright (C) 2012-2016 Mateusz Viste
 */

#include "rtrim.h"

void rtrim(char *str) {
  int x, realendofstr = 0;
  for (x = 0; str[x] != 0; x++) {
    switch (str[x]) {
      case ' ':
      case '\t':
      case '\r':
      case '\n':
        break;
      default:
        realendofstr = x + 1;
        break;
    }
  }
  str[realendofstr] = 0;
}


void trim(char *str) {
  int x, y, firstchar = -1, lastchar = -1;
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
  str[lastchar + 1] = 0; /* right trim */
  if (firstchar > 0) { /* left trim (shift to the left ) */
    y = 0;
    for (x = firstchar; str[x] != 0; x++) str[y++] = str[x];
    str[y] = 0;
  }
}
