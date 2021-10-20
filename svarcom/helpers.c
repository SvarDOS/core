/*
 * a variety of helper functions
 * Copyright (C) 2021 Mateusz Viste
 */

#include "helpers.h"

/* case-insensitive comparison of strings, returns non-zero on equality */
int imatch(const char *s1, const char *s2) {
  for (;;) {
    char c1, c2;
    c1 = *s1;
    c2 = *s2;
    if ((c1 >= 'a') && (c1 <= 'z')) c1 -= ('a' - 'A');
    if ((c2 >= 'a') && (c2 <= 'z')) c2 -= ('a' - 'A');
    /* */
    if (c1 != c2) return(0);
    if (c1 == 0) return(1);
    s1++;
    s2++;
  }
}


/* returns zero if s1 starts with s2 */
int strstartswith(const char *s1, const char *s2) {
  while (*s2 != 0) {
    if (*s1 != *s2) return(-1);
    s1++;
    s2++;
  }
  return(0);
}
