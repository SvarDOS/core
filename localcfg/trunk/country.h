/*
 * functions that read/write from/to the localcfg country.sys-like file.
 * Copyright (C) Mateusz Viste 2015
 */

#ifndef country_h_sentinel
#define country_h_sentinel

enum COUNTRY_DATEFMT {
  COUNTRY_DATE_MDY = 0, /* Month, Day, Year */
  COUNTRY_DATE_DMY = 1, /* Day, Month, Year */
  COUNTRY_DATE_YMD = 2  /* Year, Month, Day */
};

enum COUNTRY_TIMEFMT {
  COUNTRY_TIME12 = 0, /* AM/PM format (like 6:32 PM) */
  COUNTRY_TIME24 = 1  /* 24h format   (like 18:32) */
};


struct country {
  char currency[5]; /* currency symbold, ASCIIZ, 4 letters max */
  enum COUNTRY_DATEFMT datefmt; /* date format */
  enum COUNTRY_TIMEFMT timefmt; /* time format */
  short id;         /* international ID */
  short codepage;   /* usual codepage */
  unsigned char currencyprec; /* currency precision (2 = 0.12) */
  unsigned char currencydecsym; /* set if the currency symbol should replace the decimal point */
  unsigned char currencyspace; /* set if the currency symbol should be one space away from the value */
  unsigned char currencypos; /* 0=currency symbol precedes the value, 1=follows it */
  char decimal;     /* decimal separator (like . or ,) */
  char thousands;   /* thousands separatot */
  char datesep;     /* date separator (usually '-', '.' or '/') */
  char timesep;     /* time separator (usually ':') */
  char yes;         /* the "yes" character (example: 'Y') */
  char no;          /* the "no" character (example: 'N') */
};

/* Loads data from a country.sys file into a country struct.
 * Returns 0 on success, non-zero otherwise. */
int country_read(struct country *countrydata, char *fname);

/* Computes a new country.sys file based on data from a country struct.
 * Returns 0 on success, non-zero otherwise. */
int country_write(char *fname, struct country *countrydata);

#endif
