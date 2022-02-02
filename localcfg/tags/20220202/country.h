/*
 * functions that read/write from/to the localcfg country.sys-like file.
 * Copyright (C) Mateusz Viste 2015-2022
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

  struct {
    unsigned short id;          /* international id (48=PL, 33=FR, 01=US...) */
    unsigned short codepage;    /* usual codepage */
    unsigned short datefmt;     /* date format */
    char currsym[5];            /* currency symbol */
    char thousands[2];          /* thousands separator */
    char decimal[2];            /* decimal separator (like . or ,) */
    char datesep[2];            /* date separator (usually '-', '.' or '/') */
    char timesep[2];            /* time separator (usually ':') */
    unsigned char currpos:1;    /* 0=currency precedes the value, 1=follows it */
    unsigned char currspace:1;  /* set if the currency symbol should be one space away from the value */
    unsigned char currdecsym:1; /* set if the currency symbol should replace the decimal point */
    unsigned char ZEROED:5;
    unsigned char currprec;     /* currency precision (2 = 0.12) */
    unsigned char timefmt;      /* time format */
  } CTYINFO;

  struct {
    char yes[2];
    char no[2];
  } YESNO;

};

/* Loads data from a country.sys file into a country struct.
 * Returns 0 on success, non-zero otherwise. */
int country_read(struct country *countrydata, const char *fname);

/* Computes a new country.sys file based on data from a country struct.
 * Returns 0 on success, non-zero otherwise. */
int country_write(const char *fname, struct country *countrydata);

#endif
