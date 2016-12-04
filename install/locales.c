/*
 * This program creates a header file for the Svarog386 installer, that
 * contains the list of all supported keyboard layouts, along with a way to
 * select sets of keyboard layouts that apply to only a specific region.
 *
 * Copyright (C) 2016 Mateusz Viste
 */

/* the kblayouts list is a NULL-terminated array that contains entries in the
 * following format:
 * human description string <0> layout code string <0> <codepage number as a 16bit value> <ega.sys file number as a single byte> <keyboard.sys file number as a single byte>

    char *kblayouts[] = {
    "English (US)\0xxxx",
    "Polish\0pl\0xxxx"};
  };
 */


#include <stdio.h>
#include <string.h>


/* global file pointers */
FILE *fdkeyb, *fdoff;


/* Converts a positive base_10 into base_8 */
static unsigned int dec2oct(int n) {
  int res=0, digitPos=1;
  while (n) {
    res += (n & 7) * digitPos;
    n >>= 3;
    digitPos *= 10;
  }
  return(res);
}


static void addnew(char *countrycode, char *humanlang, char *keybcode, unsigned short cp, unsigned char egafile, unsigned char keybfile) {
  static char lastcountry[4] = {0};
  static int curoffset = 0, curcountryoffset = 0;
  /* if new country, declare an offset */
  if (strcmp(countrycode, lastcountry) != 0) {
    /* close previous one, if any */
    if (lastcountry[0] != 0) {
      fprintf(fdoff, "#define OFFLEN_%s %d\r\n", lastcountry, curoffset - curcountryoffset);
    } else {
      fprintf(fdkeyb, "char *kblayouts[] = {\r\n");
    }
    /* open new one, if any */
    if (countrycode[0] != 0) {
      fprintf(fdkeyb, "  /****** %s ******/\r\n", countrycode);
      curcountryoffset = curoffset;
      fprintf(fdoff, "#define OFFLOC_%s %d\r\n", countrycode, curoffset);
      strcpy(lastcountry, countrycode);
    } else {
      fprintf(fdoff, "#define OFFCOUNT %d\r\n", curoffset);
    }
  }
  /* */
  if (countrycode[0] != 0) {
    fprintf(fdkeyb, "  \"%s\\0%s\\0\\%d\\%d\\%d\\%d\",\r\n", humanlang, keybcode, dec2oct(cp >> 8), dec2oct(cp & 0xff), dec2oct(egafile), dec2oct(keybfile));
  } else {
    fprintf(fdkeyb, "  NULL};\r\n");
  }
  curoffset++;
}


int main(void) {

  /*** open files ***/
  fdkeyb = fopen("keylay.h", "wb");
  fdoff = fopen("keyoff.h", "wb");

  /******************* LAYOUTS LIST START *******************/

  /* English */
  addnew("EN", "English (US)", "en", 437, 0, 0);
  addnew("EN", "English (UK)", "uk", 437, 0, 1);

  /* French */
  addnew("FR", "French (France)", "fr", 858, 1, 1);
  addnew("FR", "French (Canada, standard)", "cf", 863, 9, 1);
  addnew("FR", "French (Canada, legacy)", "cf501", 863, 9, 1);

  /* German */
  addnew("DE", "German", "de", 858, 1, 1);

  /* Hungarian */
  addnew("HU", "Hungarian", "hu208", 852, 1, 1);

  /* Polish */
  addnew("PL", "Polish", "pl", 991, 10, 1);

  /* Spanish */
  addnew("ES", "Spanish", "es", 858, 1, 1);

  /* Turkish */
  addnew("TR", "Turkish", "tr", 857, 1, 2);

  /******************* LAYOUTS LIST STOP *******************/

  /* end of list - DO NOT REMOVE */
  addnew("", "", "", 0, 0, 0);

  /* close files */
  fclose(fdoff);
  fclose(fdkeyb);

  return(0);
}
