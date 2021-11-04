#ifndef HELPERS_H
#define HELPERS_H

/* case-insensitive comparison of strings, returns non-zero on equality */
int imatch(const char *s1, const char *s2);

/* returns zero if s1 starts with s2 */
int strstartswith(const char *s1, const char *s2);

/* outputs a NULL-terminated string to stdout */
void output_internal(const char *s, unsigned short nl);

#define output(x) output_internal(x, 0)
#define outputnl(x) output_internal(x, 1)

/*
 * FileInfoRec (DTA) format:
 * offset size desc
 *    +0   21  reserved
 *  +15h    1  file attr (1=RO 2=Hidden 4=System 8=VOL 16=DIR 32=Archive
 *  +16h    2  time: bits 0-4=bi-seconds (0-30), bits 5-10=minutes (0-59), bits 11-15=hour (0-23)
 *  +18h    2  date: bits 0-4=day(0-31), bits 5-8=month (1-12), bits 9-15=years since 1980
 *  +1ah    4  DWORD file size, in bytes
 *  +1eh   13  13-bytes max ASCIIZ filename
 */
_Packed struct DTA {
  char reserved[21];
  unsigned char attr;
  unsigned short time;
  unsigned short date;
  unsigned long size;
  char fname[13];
};

#define DOS_ATTR_RO   1
#define DOS_ATTR_HID  2
#define DOS_ATTR_SYS  4
#define DOS_ATTR_VOL  8
#define DOS_ATTR_DIR 16
#define DOS_ATTR_ARC 32

/* find first matching files using a FindFirst DOS call
 * attr contains DOS attributes that files MUST have (ie attr=0 will match all
 * files)
 * returns 0 on success or a DOS err code on failure */
unsigned short findfirst(struct DTA *dta, const char *pattern, unsigned short attr);

/* find next matching, ie. continues an action intiated by findfirst() */
unsigned short findnext(struct DTA *dta);

/* print s string and wait for a single key press from stdin. accepts only
 * key presses defined in the c ASCIIZ string. returns offset of pressed key
 * in string. keys in c MUST BE UPPERCASE! */
unsigned short askchoice(const char *s, const char *c);

/* converts a path to its canonic representation, returns 0 on success
 * or DOS err on failure (invalid drive) */
unsigned short file_truename(const char *src, char *dst);

/* returns DOS attributes of file, or -1 on error */
int file_getattr(const char *fname);

/* returns screen's width (in columns) */
unsigned short screen_getwidth(void);

/* returns screen's height (in rows) */
unsigned short screen_getheight(void);

/* displays the "Press any key to continue" msg and waits for a keypress */
void press_any_key(void);

/* validate a drive (A=0, B=1, etc). returns 1 if valid, 0 otherwise */
int isdrivevalid(unsigned char drv);

/* converts a filename into FCB format (FILENAMEEXT) */
void file_fname2fcb(char *dst, const char *src);

/* converts a FCB filename (FILENAMEEXT) into normal format (FILENAME.EXT) */
void file_fcb2fname(char *dst, const char *src);

#endif
