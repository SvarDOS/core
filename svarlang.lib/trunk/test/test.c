#include <stdio.h>

#include "../svarlang.h"


int main(void) {
  const char *larr[] = {"EN", "PL", NULL};
  const char **l;
  int r;

  for (l = larr; *l != NULL; l++) {
    printf("\nLOADING LANG=%s\n", *l);

    r = svarlang_load("out.lng", *l);
    if (r != 0) {
      printf("svarlang_load('out.lng', '%s') err %d\n", *l, r);
      return(1);
    }

    for (r = 0; r < 20; r++) {
      const char *s = svarlang_strid(r);
      if (s[0] == 0) continue;
      printf("0.%d = '%s'\n", r, s);
    }
  }

  return(0);
}
