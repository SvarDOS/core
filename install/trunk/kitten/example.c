#include <stdio.h>
#include "kitten.h"               /* kittenopen/kittengets */

int main (void) {
  char *s;

  kittenopen("example");

  s = kittengets(7, 4, "Failure writing to drive A:");
  printf("%s\n", s);

  kittenclose();
  return(0);
}
