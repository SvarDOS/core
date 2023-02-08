/*
 * functions that reads/writes from/to the localcfg country.sys-like file.
 * Copyright (C) Mateusz Viste 2015-2022
 */

#include <stdio.h>
#include <string.h>

#include "country.h"


struct funchdr {
  unsigned char funcname[8];
  unsigned short funcsiz;
};

static unsigned char filebuff[1024];


/* fills a country struct with default values */
static void country_default(struct country *countrydata) {

  /* first clear the memory */
  bzero(countrydata, sizeof(struct country));

  /* fill in CTYINFO fields (non-zero values only) */
  countrydata->CTYINFO.id = 1;
  countrydata->CTYINFO.codepage = 437;
  /* countrydata->CTYINFO.datefmt = COUNTRY_DATE_MDY;
  countrydata->CTYINFO.timefmt = COUNTRY_TIME12; */
  countrydata->CTYINFO.currsym[0] = '$';
  countrydata->CTYINFO.decimal[0] = '.';
  countrydata->CTYINFO.thousands[0] = ',';
  countrydata->CTYINFO.datesep[0] = '/';
  countrydata->CTYINFO.timesep[0] = ':';
  countrydata->CTYINFO.currprec = 2;
  /* countrydata->CTYINFO.currencydecsym = 0; */
  /* countrydata->CTYINFO.currencyspace = 0; */
  /* countrydata->CTYINFO.currencypos = 0; */

  /* fill in YESNO fields (non-zero values only) */
  countrydata->YESNO.yes[0] = 'Y';
  countrydata->YESNO.no[0] = 'N';
}


/* Loads data from a country.sys file into a country struct.
 * Returns 0 on success, non-zero otherwise. */
int country_read(struct country *countrydata, const char *fname) {
  short firstentryoffs;
  unsigned char *subfunctions[16] = {NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
                                     NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL};
  short filesize;
  short subfunctionscount;
  unsigned char *functiondata;
  int x;
  FILE *fd;

  /* preload the country struct with default values */
  country_default(countrydata);

  /* load the file into buff, if file exists */
  fd = fopen(fname, "rb");
  if (fd == NULL) return(0); /* "file doesn't exist" is not an error condition */
  filesize = fread(filebuff, 1, sizeof(filebuff), fd);
  fclose(fd);

  /* check that it's a country file - should start with 0xFF COUNTRY 0x00 */
  if (memcmp(filebuff, "\377COUNTRY\0", 9) != 0) return(COUNTRY_ERR_INV_FORMAT);

  /* check that it's one of my country.sys files - should contain a trailer */
  if (memcmp(filebuff + filesize - 8, "LOCALCFG", 8) != 0) return(COUNTRY_ERR_NOT_LOCALCFG);

  /* read the offset of the entries index - must be at least 23 */
  functiondata = filebuff + 19;
  firstentryoffs = *((unsigned short *)functiondata);
  if ((firstentryoffs < 23) || (firstentryoffs >= filesize)) return(-4);
  functiondata = filebuff + firstentryoffs;

  /* how many entries do we have? I expect exactly one. */
  if (*((unsigned short *)functiondata) != 1) return(-5);
  /* skip to the first country entry */
  functiondata += 2;

  /* skip directly to the subfunctions of the first country */
  /* ddwords: size, country, codepage, reserved, reserved, offset */
  /* printf("Size = %d\n", READSHORT(functiondata)); */
  functiondata += 2; /* skip size */
  /* printf("Country = %d\n", READSHORT(functiondata[0]); */
  functiondata += 2; /* skip country */
  /* printf("Codepage = %d\n", READSHORT(functiondata)); */
  functiondata += 2; /* skip codepage */
  functiondata += 4; /* skip reserved fields */
  firstentryoffs = *((unsigned short *)functiondata); /* read offset of the subfunctions index */
  functiondata = filebuff + firstentryoffs;

  /* read all subfunctions, but no more than 15 */
  subfunctionscount = *((unsigned short *)functiondata);
  /* printf("Found %d subfunctions\n", subfunctionscount); */
  functiondata += 2;
  for (x = 0; (x < 15) && (x < subfunctionscount); x++) {
    short size = *((unsigned short *)functiondata);
    functiondata += 2;
    functiondata += 2; /* skip ID of the subfunction */
    subfunctions[x] = filebuff + *((unsigned short *)functiondata);
    /* printf("subfunction %d at 0x%p\n", x, subfunctions[x]); */
    functiondata += size - 2;
  }

  /* load every subfunction, and feed the country struct with data */
  for (x = 0; subfunctions[x] != NULL; x++) {
    struct funchdr *hdr = (void *)(subfunctions[x]);
    functiondata = subfunctions[x] + 10;
    /* */
    if ((memcmp(hdr->funcname, "\377YESNO  ", 8) == 0) && (hdr->funcsiz == 4)) {
      memcpy(&(countrydata->YESNO), functiondata, hdr->funcsiz);
    } else if ((memcmp(hdr->funcname, "\377CTYINFO", 8) == 0) && (hdr->funcsiz == 22)) {
      memcpy(&(countrydata->CTYINFO), functiondata, hdr->funcsiz);
    }
  }

  return(0);
}


#define MSB(x) (((x) >> 8) & 0xff)
#define LSB(x) ((x) & 0xff)


/* Computes a new country.sys file based on data from a country struct.
 * Returns 0 on success, non-zero otherwise. */
int country_write(const char *fname, struct country *c) {
  short filesize = 0;
  FILE *fd;
  int x;
  short subfunction_id[7] = {1,2,4,5,6,7,35};
  short subfunction_ptr[7];

  const unsigned char ucase_437[128] = {128, 154,  69,  65, 142,  65, 143, 128,
                                         69,  69,  69,  73,  73,  73, 142, 143,
                                        144, 146, 146,  79, 153,  79,  85,  85,
                                         89, 153, 154, 155, 156, 157, 158, 159,
                                         65,  73,  79,  85, 165, 165, 166, 167,
                                        168, 169, 170, 171, 172, 173, 174, 175,
                                        176, 177, 178, 179, 180, 181, 182, 183,
                                        184, 185, 186, 187, 188, 189, 190, 191,
                                        192, 193, 194, 195, 196, 197, 198, 199,
                                        200, 201, 202, 203, 204, 205, 206, 207,
                                        208, 209, 210, 211, 212, 213, 214, 215,
                                        216, 217, 218, 219, 220, 221, 222, 223,
                                        224, 225, 226, 227, 228, 229, 230, 231,
                                        232, 233, 234, 235, 236, 237, 238, 239,
                                        240, 241, 242, 243, 244, 245, 246, 247,
                                        248, 249, 250, 251, 252, 253, 254, 255};

  const unsigned char collate_437[256] = {  0,   1,   2,   3,   4,   5,   6,   7,
                                            8,   9,  10,  11,  12,  13,  14,  15,
                                           16,  17,  18,  19,  20,  21,  22,  23,
                                           24,  25,  26,  27,  28,  29,  30,  31,
                                           32,  33,  34,  35,  36,  37,  38,  39,
                                           40,  41,  42,  43,  44,  45,  46,  47,
                                           48,  49,  50,  51,  52,  53,  54,  55,
                                           56,  57,  58,  59,  60,  61,  62,  63,
                                           64,  65,  66,  67,  68,  69,  70,  71,
                                           72,  73,  74,  75,  76,  77,  78,  79,
                                           80,  81,  82,  83,  84,  85,  86,  87,
                                           88,  89,  90,  91,  92,  93,  94,  95,
                                           96,  65,  66,  67,  68,  69,  70,  71,
                                           72,  73,  74,  75,  76,  77,  78,  79,
                                           80,  81,  82,  83,  84,  85,  86,  87,
                                           88,  89,  90, 123, 124, 125, 126, 127,
                                           67,  85,  69,  65,  65,  65,  65,  67,
                                           69,  69,  69,  73,  73,  73,  65,  65,
                                           69,  65,  65,  79,  79,  79,  85,  85,
                                           89,  79,  85,  36,  36,  36,  36,  36,
                                           65,  73,  79,  85,  78,  78, 166, 167,
                                           63, 169, 170, 171, 172,  33,  34,  34,
                                          176, 177, 178, 179, 180, 181, 182, 183,
                                          184, 185, 186, 187, 188, 189, 190, 191,
                                          192, 193, 194, 195, 196, 197, 198, 199,
                                          200, 201, 202, 203, 204, 205, 206, 207,
                                          208, 209, 210, 211, 212, 213, 214, 215,
                                          216, 217, 218, 219, 220, 221, 222, 223,
                                          224,  83, 226, 227, 228, 229, 230, 231,
                                          232, 233, 234, 235, 236, 237, 238, 239,
                                          240, 241, 242, 243, 244, 245, 246, 247,
                                          248, 249, 250, 251, 252, 253, 254, 255};

  /* zero out filebuff */
  bzero(filebuff, sizeof(filebuff));

  /* compute the country.sys structures */
  memcpy(filebuff, "\377COUNTRY\0\0\0\0\0\0\0\0\1\0\1", 19); /* header */
  filesize = 19;
  /* first entry offset (always current offset+4) */
  filesize += 4;
  memcpy(filebuff + filesize - 4, &filesize, sizeof(filesize));
  /* number of entries */
  filebuff[filesize] = 1;
  filesize += 2;
  /* first (and only) entry / size, country, codepage, reserved(2), offset */
  filebuff[filesize++] = 12; /* size LSB */
  filebuff[filesize++] = 0;  /* size MSB */
  filebuff[filesize++] = LSB(c->CTYINFO.id);   /* country LSB */
  filebuff[filesize++] = MSB(c->CTYINFO.id);   /* country MSB */
  filebuff[filesize++] = LSB(c->CTYINFO.codepage); /* codepage LSB */
  filebuff[filesize++] = MSB(c->CTYINFO.codepage); /* codepage MSB */
  filesize += 4;       /* reserved bytes */

  filesize += 4;
  memcpy(filebuff + filesize - 4, &filesize, sizeof(filesize));

  /* index of subfunctions */
  filebuff[filesize] = 7; /* there are 7 subfunctions */
  filesize += 2;
  for (x = 0; x < 7; x++) { /* dump each subfunction (size, id, offset) */
    /* size is always 6 */
    filebuff[filesize] = 6;
    filesize += 2;
    /* id of the subfunction */
    filebuff[filesize++] = LSB(subfunction_id[x]);
    filebuff[filesize++] = MSB(subfunction_id[x]);
    /* remember the offset of the subfunction pointer for later */
    subfunction_ptr[x] = filesize;
    filesize += 4;
  }

  /* write the CTYINFO subfunction */
  memcpy(filebuff + subfunction_ptr[0], &filesize, sizeof(filesize));

  /* subfunction header */
  memcpy(filebuff + filesize, "\377CTYINFO", 8);
  filesize += 8;
  /* subfunction size */
  filebuff[filesize] = 22;
  filesize += 2;

  /* country preferences */
  memcpy(filebuff + filesize, &(c->CTYINFO), 22);
  filesize += 22;

  /* write the UCASE subfunction (used for LCASE, too) */
  memcpy(filebuff + subfunction_ptr[1], &filesize, sizeof(filesize));
  memcpy(filebuff + subfunction_ptr[2], &filesize, sizeof(filesize));

  /* subfunction header */
  memcpy(filebuff + filesize, "\377UCASE  ", 8);
  filesize += 8;
  /* subfunction size */
  filebuff[filesize++] = 128;
  filebuff[filesize++] = 0;
  /* UCASE table */
  memcpy(filebuff + filesize, ucase_437, 128);
  filesize += 128;

  /* write the FCHAR subfunction (filename terminator table) */
  memcpy(filebuff + subfunction_ptr[3], &filesize, sizeof(filesize));

  /* subfunction header */
  memcpy(filebuff + filesize, "\377FCHAR  ", 8);
  filesize += 8;
  /* subfunction size */
  filebuff[filesize++] = 22;
  filebuff[filesize++] = 0;
  /* values here are quite obscure, dumped from country.sys */
  filebuff[filesize++] = 142;
  filebuff[filesize++] = 0;
  filebuff[filesize++] = 255;
  filebuff[filesize++] = 65;
  filebuff[filesize++] = 0;
  filebuff[filesize++] = 32;
  filebuff[filesize++] = 238;
  /* list of characters that terminates a filename */
  filebuff[filesize++] = 14;  /* how many of them */
  filebuff[filesize++] = 46;  /* . */
  filebuff[filesize++] = 34;  /* " */
  filebuff[filesize++] = 47;  /* / */
  filebuff[filesize++] = 92;  /* \ */
  filebuff[filesize++] = 91;  /* [ */
  filebuff[filesize++] = 93;  /* ] */
  filebuff[filesize++] = 58;  /* : */
  filebuff[filesize++] = 124; /* | */
  filebuff[filesize++] = 60;  /* < */
  filebuff[filesize++] = 62;  /* > */
  filebuff[filesize++] = 43;  /* + */
  filebuff[filesize++] = 61;  /* = */
  filebuff[filesize++] = 59;  /* ; */
  filebuff[filesize++] = 44;  /* , */

  /* write the COLLATE subfunction */
  memcpy(filebuff + subfunction_ptr[4], &filesize, sizeof(filesize));

  /* subfunction header */
  memcpy(filebuff + filesize, "\377COLLATE", 8);
  filesize += 8;
  /* subfunction size */
  filebuff[filesize++] = LSB(256);
  filebuff[filesize++] = MSB(256);
  /* collation for standard CP437 */
  memcpy(filebuff + filesize, collate_437, 256);
  filesize += 256;

  /* write the DBCS subfunction */
  memcpy(filebuff + subfunction_ptr[5], &filesize, sizeof(filesize));
  /* subfunction header */
  memcpy(filebuff + filesize, "\377DBCS   ", 8);
  filesize += 8;
  /* subfunction size */
  filebuff[filesize++] = 0;
  filebuff[filesize++] = 0;
  /* table terminator (must be there even if no lenght is zero */
  filebuff[filesize++] = 0;
  filebuff[filesize++] = 0;

  /* write the YESNO subfunction */
  memcpy(filebuff + subfunction_ptr[6], &filesize, sizeof(filesize));
  memcpy(filebuff + filesize, "\377YESNO  ", 8);
  filesize += 8;
  filebuff[filesize] = 4;  /* size (LSB) */
  filesize += 2;
  memcpy(filebuff + filesize, &(c->YESNO), 4);
  filesize += 4;

  /* write the file trailer */
  memcpy(filebuff + filesize, "LOCALCFG", 8);
  filesize += 8;

  /* write the buffer to file */
  fd = fopen(fname, "wb");
  if (fd == NULL) return(-1);
  fwrite(filebuff, 1, filesize, fd);
  fclose(fd);

  return(0);
}
