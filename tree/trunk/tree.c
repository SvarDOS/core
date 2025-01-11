/****************************************************************************

  TREE - Graphically displays the directory structure of a drive or path

****************************************************************************/

#define VERSION "20250111"

/****************************************************************************

  Written by: Kenneth J. Davis
  Date:       August, 2000
  Updated:    September, 2000; October, 2000; November, 2000; January, 2001;
              May, 2004; Sept, 2005

  2024-2025:  Lots of changes by Mateusz Viste, became SvarDOS TREE.
              See CHANGES.TXT for details.

Copyright (c): Public Domain [United States Definition]

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR AUTHORS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.

****************************************************************************/


#include <dos.h>
#include <stdio.h>   /* only included for the PATH_MAX definition */
#include <stdlib.h>
#include <string.h>

#include "stack.h"
#include "svarlang/svarlang.h"


/* The default extended forms of the lines used. */
#define VERTBAR_STR  "\xB3   "                 /* |    */
#define TBAR_HORZBAR_STR "\xC3\xC4\xC4\xC4"    /* +--- */
#define CBAR_HORZBAR_STR "\xC0\xC4\xC4\xC4"    /* \--- */


/* Global flags */
static unsigned char showFiles = 0;  /* Display names of files in directories       */
static unsigned char asciiOnly = 0;  /* Use ASCII [7bit] characters                 */
static unsigned char pause = 0;      /* Wait for keypress after each page           */

static unsigned char dspAll = 0;     /* if nonzero includes HIDDEN & SYSTEM files in output */
static unsigned char dspSize = 0;    /* if nonzero displays filesizes                       */
static unsigned char dspAttr = 0;    /* if nonzero displays file attributes [DACESHRBP]     */
static unsigned char dspSumDirs = 0; /* show count of subdirectories  (per dir and total)*/


/* maintains total count, for > 4billion dirs, use a __int64 */
static unsigned long totalSubDirCnt = 0;


/* text window size, used to determine when to pause */
static unsigned char cols = 80, rows = 25;  /* determined on startup (when possible) */



/* Global constants */
#define SERIALLEN 16      /* Defines max size of volume & serial number   */
#define VOLLEN 16

#define MAXPADLEN (PATH_MAX*2) /* Must be large enough to hold the maximum padding */
/* (PATH_MAX/2)*4 == (max path len / min 2chars dirs "?\") * 4chars per padding    */


/* Procedures */


/* returns the current drive (A=0, B=1, etc) */
static unsigned char getdrive(void);
#pragma aux getdrive = \
"mov ah, 0x19" \
"int 0x21" \
modify [ah] \
value [al]


/* waits for a keypress, flushes keyb buffer, returns nothing */
static void waitkey(void);
#pragma aux waitkey = \
"mov ah, 0x08" \
"int 0x21" \
/* flush keyb buffer in case it was an extended key */ \
"mov ax, 0x0C0B" \
"int 0x21" \
modify [ax]


/* checks if stdout appears to be redirected. returns 0 if not, non-zero otherwise. */
static unsigned char is_stdout_redirected(void);
#pragma aux is_stdout_redirected = \
"mov ax, 0x4400"    /* DOS 2+, IOCTL Get Device Info */            \
"mov bx, 0x0001"    /* file handle (stdout) */                     \
"int 0x21" \
"jc DONE"           /* on error AL contains a non-zero err code */ \
"and dl, 0x80"      /* bit 7 of DL is the "CHAR" flag */           \
"xor dl, 0x80"      /* return 0 if CHAR bit is set */              \
"mov al, dl" \
"DONE:" \
modify [ax bx dx] \
value [al]


/* outputs a single character to console */
static void outch(char s);
#pragma aux outch = \
"mov ah, 0x02" \
"int 0x21" \
modify [ah] \
parm [dl]


/* display nul-terminated string on screen, no new line */
static void outstr(const char *s) {
  for (; *s != 0; s++) outch(*s);
}


/* display nul-terminated string on screen and append a new line (CR LF) */
static void outstrnl(const char *s) {
  outstr(s);
  outstr("\r\n");
}


/* drv is 1-based (A=1, B=2, ...) */
static unsigned char getdrvserial(unsigned char drv, void far *drv_info_ptr);
#pragma aux getdrvserial = \
"mov ax, 0x6900"   /* DOS 4+ - get disk serial number */ \
"push ds" \
"xor bh, bh"       /* "info level" (OS/2 only, must be 0 for DOS) */ \
"push es" \
"pop ds"           /* ptr is expected in DS:DX */ \
"int 0x21" \
"lahf" \
"and ah, 1" \
"pop ds" \
parm [bl] [es dx] \
modify [ax bh] \
value [ah]


static unsigned char truename(char far *path, const char far *origpath);
#pragma aux truename = \
"push ds" \
"mov ds, dx" \
"mov ah, 0x60" \
"int 0x21"    /* DOS 5+ - TRUENAME */ \
"lahf"        /* mov flags to ah */ \
"and ah, 1"   /* isolate CF so AH != 0 on error */ \
"pop ds" \
parm [es di] [dx si] \
modify [si di] \
value [ah]


/* sets rows & cols to size of actual console window
 * force NO PAUSE if appears output redirected to a file or
 * piped to another program
 * Uses hard coded defaults and leaves pause flag unchanged
 * if unable to obtain information.
 */
static void getConsoleSize(void) {
  unsigned short far *bios_cols = (unsigned short far *)MK_FP(0x40,0x4A);
  unsigned short far *bios_size = (unsigned short far *)MK_FP(0x40,0x4C);

  if (is_stdout_redirected() != 0) {
    /* e.g. redirected to a file, tree > filelist.txt */
    /* Output to a file or program, so no screen to fill (no max cols or rows) */
      pause = 0;   /* so ignore request to pause */
  } else { /* e.g. the console */
    if ((*bios_cols == 0) || (*bios_size == 0)) { /* MDA does not report size */
      cols = 80;
      rows = 25;
    } else {
      cols = *bios_cols;
      rows = *bios_size / cols / 2;
    }
  }
}


/* when pause == 0 then identical to puts, otherwise counts lines printed and
 * pauses as needed. Should be used for all messages printed that do not
 * immediately exit afterwards (else puts may be used). May display N too many
 * lines before pause if line is printed that exceeds cols [N=linelen%cols] and
 * lacks any newlines (but this should not occur in tree).
 */
static void pputs(const char *s) {
  static unsigned short count;
  outstrnl(s);
  if ((pause) && (++count + 1 >= rows)) {
    outstr(svarlang_strid(0x0106));
    waitkey();
    outstrnl("");
    count = 0;
  }
}


/* prints two strings, one embedded in another */
static void print_strinstr(const char *s1, const char *s2) {
  /* output s1 char after char, if '%' is spotted then output s2 */
  for (; *s1 != 0; s1++) {
    if (*s1 == '%') {
      outstr(s2);
    } else {
      outch(*s1);
    }
  }
}


static void showShortVer(void) {
  outstrnl("SvarDOS TREE " VERSION "\r\n");
}


/* Displays to user valid options then exits program indicating no error */
static void showUsage(void) {
  unsigned short i;
  showShortVer();
  for (i = 0x0200; i < 0x021F; i++) {
    const char *s = svarlang_strid(i);
    if (s[0] == 0) continue;
    outstrnl(s);
  }
  exit(1);
}


/* Displays error message then exits indicating error */
static void showInvalidUsage(char * badOption) {
  print_strinstr(svarlang_strid(0x0301), badOption); /* invalid switch - ... */
  outstrnl("");
  outstrnl(svarlang_strid(0x0302)); /* use TREE /? for usage info */
  exit(1);
}


/* Displays author, copyright, etc info, then exits indicating no error. */
static void showVersionInfo(void) {
  unsigned short i;
  showShortVer();
  for (i = 0x0400; i < 0x0409; i++) {
    if (svarlang_strid(i)[0] == 0) continue;
    outstrnl(svarlang_strid(i));
  }
  exit(1);
}


/* Displays error messge for invalid drives and exits */
static void showInvalidDrive(void) {
  outstrnl(svarlang_strid(0x0501)); /* invalid drive spec */
  exit(1);
}


/**
 * Takes a given path, strips any \ or / that may appear on the end.
 * Returns a pointer to its static buffer containing path
 * without trailing slash and any necessary display conversions.
 */
static char *fixPathForDisplay(char *path);

/* Displays error message for invalid path; Does NOT exit */
static void showInvalidPath(const char *badpath) {
  print_strinstr(svarlang_strid(0x0601), badpath); /* invalid path - ... */
  pputs("");
}

/* Displays error message for out of memory; Does NOT exit */
static void showOutOfMemory(const char *path) {
  print_strinstr(svarlang_strid(0x0702), path); /* out of memory on subdir ... */
  pputs("");
}


/* Parses the command line and sets global variables. */
static void parseArguments(char *path, int argc, char **argv) {
  int i;

  /* if no drive specified on command line, use current */
  if (truename(path, ".") != 0) showInvalidDrive();

  for (i = 1; i < argc; i++) {

    /* Check if user is giving an option or drive/path */
    if ((argv[i][0] != '/') && (argv[i][0] != '-') ) {
      if (truename(path, argv[i]) != 0) {
        showInvalidPath(argv[i]);
        exit(1);
      }
      continue;
    }

    /* must be an option then */
    /* check multi character options 1st */
    if ((argv[i][1] & 0xDF) == 'D') {
      switch(argv[i][2] & 0xDF) {
        case 'A' :       /*  /DA  display attributes */
          dspAttr = 1;
          break;
        case 'F' :       /*  /DF  display filesizes  */
          dspSize = 1;
          break;
        case 'H' :       /*  /DH  display hidden & system files (normally not shown) */
          dspAll = 1;
          break;
        case 'R' :       /*  /DR  display results at end */
          dspSumDirs = 1;
          break;
        default:
          showInvalidUsage(argv[i]);
      }
      continue;
    }

    /* a 1 character option (or invalid) */
    if (argv[i][2] != 0) showInvalidUsage(argv[i]);

    switch(argv[i][1] & 0xDF) { /* upcase */
      case 'F': /* show files */
        showFiles = 1; /* set file display flag appropriately */
        break;
      case 'A': /* use ASCII only (7-bit) */
        asciiOnly = 1;    /* set charset flag appropriately      */
        break;
      case 'V': /* Version information */
        showVersionInfo();       /* show version info and exit          */
        break;
      case 'P': /* wait for keypress after each page (pause) */
        pause = 1;
        break;
      case '?' & 0xDF:
        showUsage();             /* show usage info and exit            */
        break;
      default: /* Invalid or unknown option */
        showInvalidUsage(argv[i]);
    }
  }
}


/**
 * Fills in the serial and volume variables with the serial #
 * and volume found using path.
 */
static void GetVolumeAndSerial(char *volume, char *serial, const char *path) {
  char buff[8] = "@:\\*.*";
  struct find_t findData;
  _Packed struct {
    unsigned short infolevel;
    unsigned short serial2;
    unsigned short serial1;
    char label[11];
    char fstype[8];
  } drv_info;

  /* do not rely on the EBPB for the volume label - look for a volume file at
   * the root of the drive, as all applications do */
  buff[0] = path[0];
  if (_dos_findfirst(buff, _A_VOLID, &findData) != 0) {
    *volume = 0;
  } else {
    char *s;
    /* copy the filename to volume name, skip the dot */
    for (s = findData.name;; s++) {
      if (*s == '.') continue;
      *volume = *s;
      volume++;
      if (*s == 0) break;
    }
  }
  _dos_findclose(&findData);

  /* now let's ask DOS for the serial */
  if (getdrvserial((path[0] & 0xDF) - '@', &drv_info) != 0) {
    *serial = 0;
  } else {
    /* build the "1234:5678" serial number string */
    strcpy(serial, "0000");
    utoa(drv_info.serial1, buff, 16);
    strcpy(serial + 4 - strlen(buff), buff);
    strcat(serial, ":0000");
    utoa(drv_info.serial2, buff, 16);
    strcpy(serial + 9 - strlen(buff), buff);
  }
}


/**
 * Stores directory information obtained from FindFirst/Next that
 * we may wish to make use of when displaying directory entry.
 * e.g. attribute, dates, etc.
 */
typedef struct DIRDATA {
  unsigned long subdirCnt;  /* how many subdirectories we have */
  unsigned long fileCnt;    /* how many [normal] files we have */
  unsigned int attrib;      /* Directory attributes            */
} DIRDATA;

/**
 * Contains the information stored in a Stack necessary to allow
 * non-recursive function to display directory tree.
 */
struct SUBDIRINFO {
  struct SUBDIRINFO *parent; /* points to parent subdirectory                */
  char *currentpath;    /* Stores the full path this structure represents     */
  char *subdir;         /* points to last subdir within currentpath           */
  char *dsubdir;        /* Stores a display ready directory name              */
  long subdircnt;       /* Initially a count of how many subdirs in this dir  */
  struct find_t *findnexthnd; /* The handle returned by findfirst, used in findnext */
  struct DIRDATA ddata; /* Maintain directory information, eg attributes      */
};


/**
 * Returns 0 if no subdirectories, count if has subdirs.
 * Path must end in slash \ or /
 * On error (invalid path) displays message and returns -1L.
 * Stores additional directory data in ddata if non-NULL
 * and path is valid.
 */
static long hasSubdirectories(char *path, DIRDATA *ddata) {
  struct find_t findData;
  char buffer[PATH_MAX + 4];
  unsigned short hasSubdirs = 0;

  /* get the handle to start with (using wildcard spec) */
  strcpy(buffer, path);
  strcat(buffer, "*.*");

  if (_dos_findfirst(buffer, 0x37, &findData) != 0) {
    showInvalidPath(path); /* Display error message */
    return(-1);
  }

  /*  cycle through entries counting directories found until no more entries */
  do {
    if ((findData.attrib & _A_SUBDIR) == 0) continue; /* not a DIR */
      /* filter out system and hidden files, unless dspAll is on */
    if (dspAll == 0) {
      if (findData.attrib & _A_HIDDEN) continue;
      if (findData.attrib & _A_SYSTEM) continue;
    }
    if (findData.name[0] != '.') { /* ignore '.' and '..' */
      hasSubdirs++;      /* subdir of initial path found, so increment counter */
    }
  } while(_dos_findnext(&findData) == 0);

  /* prevent resource leaks, close the handle. */
  _dos_findclose(&findData);

  if (ddata != NULL) { // don't bother if user doesn't want them
    /* The root directory of a volume (including non root paths
       corresponding to mount points) may not have a current (.) and
       parent (..) entry.  So we can't get attributes for initial
       path in above loop from the FindFile call as it may not show up
       (no . entry).  So instead we explicitly get them here.
    */
    if (_dos_getfileattr(path, &(ddata->attrib)) != 0) {
      //printf("ERROR: unable to get file attr, %i\n", GetLastError());
      ddata->attrib = 0;
    }

    /* a curiosity, for showing sum of directories process */
    ddata->subdirCnt = hasSubdirs;
  }
  totalSubDirCnt += hasSubdirs;

  return hasSubdirs;
}


/**
 * Allocates memory and stores the necessary stuff to allow us to
 * come back to this subdirectory after handling its subdirectories.
 * parentpath must end in \ or / or be NULL, however
 * parent should only be NULL for initialpath
 * if subdir does not end in slash, one is added to stored subdir
 * dsubdir is subdir already modified so ready to display to user
 */
static struct SUBDIRINFO *newSubdirInfo(struct SUBDIRINFO *parent, char *subdir, char *dsubdir) {
  int parentLen, subdirLen;
  struct SUBDIRINFO *temp;

  /* Get length of parent directory */
  if (parent == NULL) {
    parentLen = 0;
  } else {
    parentLen = strlen(parent->currentpath);
  }

  /* Get length of subdir, add 1 if does not end in slash */
  subdirLen = strlen(subdir);
  if ((subdirLen < 1) || ( (*(subdir+subdirLen-1) != '\\') && (*(subdir+subdirLen-1) != '/') ) )
    subdirLen++;

  temp = malloc(sizeof(struct SUBDIRINFO));
  if (temp == NULL) {
    showOutOfMemory(subdir);
    return NULL;
  }
  if ( ((temp->currentpath = (char *)malloc(parentLen+subdirLen+1)) == NULL) ||
       ((temp->dsubdir = (char *)malloc(strlen(dsubdir)+1)) == NULL) )
  {
    showOutOfMemory(subdir);
    if (temp->currentpath != NULL) free(temp->currentpath);
    free(temp);
    return NULL;
  }
  temp->parent = parent;
  if (parent == NULL) {
    strcpy(temp->currentpath, "");
  } else {
    strcpy(temp->currentpath, parent->currentpath);
  }
  strcat(temp->currentpath, subdir);

  /* if subdir[subdirLen-1] == '\0' then we must append a slash */
  if (*(subdir+subdirLen-1) == '\0') strcat(temp->currentpath, "\\");

  temp->subdir = temp->currentpath+parentLen;
  strcpy(temp->dsubdir, dsubdir);
  if ((temp->subdircnt = hasSubdirectories(temp->currentpath, &(temp->ddata))) == -1L) {
    free (temp->currentpath);
    free (temp->dsubdir);
    free(temp);
    return NULL;
  }
  temp->findnexthnd = NULL;

  return temp;
}


/**
 * Extends the padding with the necessary 4 characters.
 * Returns the pointer to the padding.
 * padding should be large enough to hold the additional
 * characters and '\0', moreSubdirsFollow specifies if
 * this is the last subdirectory in a given directory
 * or if more follow (hence if a | is needed).
 * padding must not be NULL
 */
static char * addPadding(char *padding, int moreSubdirsFollow) {
  if (moreSubdirsFollow) {
    /* 1st char is | or a vertical bar */
    if (asciiOnly) {
      strcat(padding, "|   ");
    } else {
      strcat(padding, VERTBAR_STR);
    }
  } else {
    strcat(padding, "    ");
  }

  return(padding);
}

/**
 * Removes the last padding added (last 4 characters added).
 * Does nothing if less than 4 characters in string.
 * padding must not be NULL
 * Returns the pointer to padding.
 */
static char *removePadding(char *padding) {
  size_t len = strlen(padding);

  if (len < 4) return padding;
  *(padding + len - 4) = '\0';

  return padding;
}


/**
 * Displays the current path, with necessary padding before it.
 * A \ or / on end of currentpath is not shown.
 * moreSubdirsFollow should be nonzero if this is not the last
 * subdirectory to be displayed in current directory, else 0.
 * Also displays additional information, such as attributes or
 * sum of size of included files.
 * currentpath is an ASCIIZ string of path to display
 *             assumed to be a displayable path (ie. OEM or UTF-8)
 * padding is an ASCIIZ string to display prior to entry.
 * moreSubdirsFollow is -1 for initial path else >= 0.
 */
static void showCurrentPath(char *currentpath, char *padding, int moreSubdirsFollow, DIRDATA *ddata) {
  if (padding != NULL) {
    outstr(padding);
  }

  /* print lead padding except for initial directory */
  if (moreSubdirsFollow >= 0) {
    if (!asciiOnly) {
      if (moreSubdirsFollow) {
        outstr(TBAR_HORZBAR_STR);
      } else {
        outstr(CBAR_HORZBAR_STR);
      }
    } else {
      if (moreSubdirsFollow) {
        outstr("+---");
      } else {
        outstr("\\---");
      }
    }
  }

  /* optional display data */
  if (dspAttr) { /* attributes */
    char attrstr[] = "[     ] ";
    if (ddata->attrib & _A_SUBDIR) attrstr[1] = 'D'; /* keep this one? its always true */
    if (ddata->attrib & _A_ARCH) attrstr[2] = 'A';
    if (ddata->attrib & _A_SYSTEM) attrstr[3] = 'S';
    if (ddata->attrib & _A_HIDDEN) attrstr[4] = 'H';
    if (ddata->attrib & _A_RDONLY) attrstr[5] = 'R';
    outstr(attrstr);
  }

  /* display directory name */
  pputs(currentpath);
}


/**
 * Displays summary information about directory.
 * Expects to be called after displayFiles (optionally called)
 */
static void displaySummary(char *padding, int hasMoreSubdirs, DIRDATA *ddata) {
  char buff[16];
  addPadding(padding, hasMoreSubdirs);

  if (dspSumDirs) {
    if (showFiles) {
      /* print File summary with lead padding, add filesize to it */
      outstr(padding);
      ultoa(ddata->fileCnt, buff, 10);
      outstr(buff);
      pputs(" files");
    }

    /* print Directory summary with lead padding */
    outstr(padding);
    ultoa(ddata->subdirCnt, buff, 10);
    outstr(buff);
    pputs(" subdirectories");

    /* show [nearly] blank line after summary */
    pputs(padding);
  }

  removePadding(padding);
}

/**
 * Displays files in directory specified by path.
 * Path must end in slash \ or /
 * Returns -1 on error,
 *          0 if no files, but no errors either,
 *      or  1 if files displayed, no errors.
 */
static int displayFiles(const char *path, char *padding, int hasMoreSubdirs, DIRDATA *ddata) {
  char buffer[PATH_MAX + 4];
  struct find_t entry;   /* current directory entry info    */
  unsigned long filesShown = 0;

  /* get handle for files in current directory (using wildcard spec) */
  strcpy(buffer, path);
  strcat(buffer, "*.*");
  if (_dos_findfirst(buffer, 0x37, &entry) != 0) return(-1);

  addPadding(padding, hasMoreSubdirs);

  /* cycle through directory printing out files. */
  do {
    /* print padding followed by filename */
    if ( ((entry.attrib & _A_SUBDIR) == 0) &&
         ( ((entry.attrib & (_A_HIDDEN | _A_SYSTEM)) == 0)  || dspAll) )
    {
      /* print lead padding */
      outstr(padding);

      /* optional display data */
      if (dspAttr) { /* file attributes */
        char attrstr[] = "[    ] ";
        if (entry.attrib & _A_ARCH) attrstr[1] = 'A';
        if (entry.attrib & _A_SYSTEM) attrstr[2] = 'S';
        if (entry.attrib & _A_HIDDEN) attrstr[3] = 'H';
        if (entry.attrib & _A_RDONLY) attrstr[4] = 'R';
        outstr(attrstr);
      }

      if (dspSize) { /* file size */
        char buff[32] = "          ";
        if (entry.size < 1048576ul) { /* if less than a MB, display in bytes */
          //sprintf(buff, "%10lu ", entry.size);
          ultoa(entry.size, buff + 10, 10);
          outstr(buff + strlen(buff + 10));
          outstr(" ");
        } else {                      /* otherwise display in KB */
          //sprintf(buff, "%8luKB ", entry.size / 1024ul);
          ultoa(entry.size / 1024, buff + 8, 10);
          outstr(buff + strlen(buff + 8));
          outstr("KB ");
        }
      }

      /* print filename */
      pputs(entry.name);

      filesShown++;
    }
  } while(_dos_findnext(&entry) == 0);

  if (filesShown) {
    pputs(padding);
  }

  removePadding(padding);

  /* store for summary display */
  if (ddata != NULL) ddata->fileCnt = filesShown;

  return (filesShown)? 1 : 0;
}


/**
 * Common portion of findFirstSubdir and findNextSubdir
 * Checks current FindFile results to determine if a valid directory
 * was found, and if so copies appropriate data into subdir and dsubdir.
 * It will repeat until a valid subdirectory is found or no more
 * are found, at which point it closes the FindFile search handle and
 * return NULL.  If successful, returns FindFile handle.
 */
static struct find_t *cycleFindResults(struct find_t *entry, char *subdir, char *dsubdir) {
  /* cycle through directory until 1st non . or .. directory is found. */
  for (;;) {
    /* skip files & hidden or system directories */
    if ((((entry->attrib & _A_SUBDIR) == 0) ||
         ((entry->attrib &
          (_A_HIDDEN | _A_SYSTEM)) != 0  && !dspAll) ) ||
        (entry->name[0] == '.')) {
      if (_dos_findnext(entry) != 0) {
        _dos_findclose(entry);      // prevent resource leaks
        return(NULL); // no subdirs found
      }
    } else {
      /* set display name */
      strcpy(dsubdir, entry->name);

      strcpy(subdir, entry->name);
      strcat(subdir, "\\");
      return(entry);
    }
  }

  return entry;
}


/**
 * Given the current path, find the 1st subdirectory.
 * The subdirectory found is stored in subdir.
 * subdir is cleared on error or no subdirectories.
 * Returns the findfirst search HANDLE, which should be passed to
 * findclose when directory has finished processing, and can be
 * passed to findnextsubdir to find subsequent subdirectories.
 * Returns NULL on error.
 * currentpath must end in \
 */
static struct find_t *findFirstSubdir(char *currentpath, char *subdir, char *dsubdir) {
  char buffer[PATH_MAX + 4];
  struct find_t *dir;         /* Current directory entry working with      */

  dir = malloc(sizeof(struct find_t));
  if (dir == NULL) return(NULL);

  /* get handle for files in current directory (using wildcard spec) */
  strcpy(buffer, currentpath);
  strcat(buffer, "*.*");

  if (_dos_findfirst(buffer, 0x37, dir) != 0) {
    showInvalidPath(currentpath);
    return(NULL);
  }

  /* clear result path */
  strcpy(subdir, "");

  return cycleFindResults(dir, subdir, dsubdir);
}

/**
 * Given a search HANDLE, will find the next subdirectory,
 * setting subdir to the found directory name.
 * dsubdir is the name to display
 * currentpath must end in \
 * If a subdirectory is found, returns 0, otherwise returns 1
 * (either error or no more files).
 */
static int findNextSubdir(struct find_t *findnexthnd, char *subdir, char *dsubdir) {
  /* clear result path */
  subdir[0] = 0;

  if (_dos_findnext(findnexthnd) != 0) return(1); // no subdirs found

  if (cycleFindResults(findnexthnd, subdir, dsubdir) == NULL) {
    return 1;
  }
  return 0;
}

/**
 * Given an initial path, displays the directory tree with
 * a non-recursive function using a Stack.
 * initialpath must be large enough to hold an added slash \ or /
 * if it does not already end in one.
 * Returns the count of subdirs in initialpath.
 */
static long traverseTree(char *initialpath) {
  long subdirsInInitialpath;
  char padding[MAXPADLEN] = "";
  char subdir[PATH_MAX];
  char dsubdir[PATH_MAX];
  struct SUBDIRINFO *sdi;

  STACK s;
  stackDefaults(&s);
  stackInit(&s);

  if ( (sdi = newSubdirInfo(NULL, initialpath, initialpath)) == NULL) {
    return(0);
  }
  stackPushItem(&s, sdi);

  /* Store count of subdirs in initial path so can display message if none. */
  subdirsInInitialpath = sdi->subdircnt;

  do {
    sdi = (struct SUBDIRINFO *)stackPopItem(&s);

    if (sdi->findnexthnd == NULL) { // findfirst not called yet
      // 1st time this subdirectory processed, so display its name & possibly files
      if (sdi->parent == NULL) { // if initial path
        // display initial path
        showCurrentPath(/*sdi->dsubdir*/initialpath, NULL, -1, &(sdi->ddata));
      } else { // normal processing (display path, add necessary padding)
        showCurrentPath(sdi->dsubdir, padding, (sdi->parent->subdircnt > 0L)?1 : 0, &(sdi->ddata));
        addPadding(padding, (sdi->parent->subdircnt > 0L)?1 : 0);
      }

      if (showFiles) {
        displayFiles(sdi->currentpath, padding, (sdi->subdircnt > 0L)?1 : 0, &(sdi->ddata));
      }
      displaySummary(padding, (sdi->subdircnt > 0L)?1 : 0, &(sdi->ddata));
    }

    if (sdi->subdircnt > 0) { /* if (there are more subdirectories to process) */
      int flgErr;
      if (sdi->findnexthnd == NULL) {
        sdi->findnexthnd = findFirstSubdir(sdi->currentpath, subdir, dsubdir);
        flgErr = (sdi->findnexthnd == NULL);
      } else {
        flgErr = findNextSubdir(sdi->findnexthnd, subdir, dsubdir);
      }

      if (flgErr) { // don't add invalid paths to stack
        //printf("INTERNAL ERROR: subdir count changed, expecting %li more!", sdi->subdircnt+1L);
        outstrnl("INTERNAL ERROR: subdir count changed!");

        sdi->subdircnt = 0; /* force subdir counter to 0, none left */
        stackPushItem(&s, sdi);
      } else {
        sdi->subdircnt = sdi->subdircnt - 1L; /* decrement subdirs left count */
        stackPushItem(&s, sdi);

        /* store necessary information, validate subdir, and if no error store it. */
        if ((sdi = newSubdirInfo(sdi, subdir, dsubdir)) != NULL)
          stackPushItem(&s, sdi);
      }
    } else { /* this directory finished processing, so free resources */
      /* Remove the padding for this directory, all but initial path. */
      if (sdi->parent != NULL)
        removePadding(padding);

      /* Prevent resource leaks, by ending findsearch and freeing memory. */
      _dos_findclose(sdi->findnexthnd);
      if (sdi != NULL)
      {
        if (sdi->currentpath != NULL)
          free(sdi->currentpath);
        free(sdi);
      }
    }
  } while (stackTotalItems(&s)); /* while (stack is not empty) */

  stackTerm(&s);

  return subdirsInInitialpath;
}


int main(int argc, char **argv) {
  static char path[PATH_MAX]; /* path to begin search from, default=current */
  char serial[SERIALLEN]; /* volume serial #  0000:0000 */
  char volume[VOLLEN];    /* volume name (label), possibly none */

  /* load translation strings */
  svarlang_autoload_exepath(argv[0], getenv("LANG"));

  /* Parse any command line arguments, obtain path */
  parseArguments(path, argc, argv);

  /* Initialize screen size, may reset pause to NOPAUSE if redirected */
  getConsoleSize();

  /* Get Volume & Serial Number */
  GetVolumeAndSerial(volume, serial, path);
  if (volume[0] == 0) {
    pputs(svarlang_strid(0x0102)); /* Dir PATH listing */
  } else {
    print_strinstr(svarlang_strid(0x0103), volume); /* Dir PATH listing for volume ... */
    pputs("");
  }
  if (serial[0] != '\0') {  /* Don't print anything if no serial# found */
    print_strinstr(svarlang_strid(0x0104), serial); /* vol serial num is ... */
    pputs("");
  }

  /* now traverse & print tree, returns nonzero if has subdirectories */
  if (traverseTree(path) == 0) {
    pputs(svarlang_strid(0x0105)); /* no subdirs exist */
  } else if (dspSumDirs) { /* show count of directories processed */
    char buff[16];
    pputs("");
    outstr("    ");
    ultoa(totalSubDirCnt+1, buff, 10);
    outstr(buff);
    pputs(" total directories");
  }

  return(0);
}
