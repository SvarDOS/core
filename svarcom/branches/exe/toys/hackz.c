/*
 * reads stdin and writes to stdout after hacker-like conversion
 */

#include <stdio.h>

int main(void) {
  int c;

  while ((c = getchar()) != EOF) {
    switch (c) {
      case 'o':
      case 'O':
        c = '0';
        break;
      case 'i':
      case 'I':
        c = '1';
        break;
      case 'A':
        c = '4';
        break;
      case 'a':
        c = '@';
        break;
      case 'S':
        c = '$';
        break;
      case 'e':
      case 'E':
        c = '3';
        break;
      case '0':
        c = 'o';
        break;
    }
    putchar(c);
  }

  return(0);
}
