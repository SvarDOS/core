#include <stdio.h>

#include "../svarlang.h"


int main(void) {
  const char *larr[] = {"EN", "PL", NULL};
  const char **l;
  int r;
  int comp;

  for (l = larr; *l != NULL; l++) {
    for (comp = 0; comp < 2; comp++) {
      printf("\nLOADING LANG=%s (comp=%d)\n", *l, comp);

      if (comp) {
        r = svarlang_load("outc.lng", *l);
      } else {
        r = svarlang_load("out.lng", *l);
      }
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
  }

  return(0);
}
