/****************************************************************************

  TREE - Graphically displays the directory structure of a drive or path

****************************************************************************/

#define VERSION "1.04"

/****************************************************************************

  Written by: Kenneth J. Davis
  Date:       August, 2000
  Updated:    September, 2000; October, 2000; November, 2000; January, 2001;
              May, 2004; Sept, 2005
  Contact:    jeremyd@computer.org


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


/* Include files */
#include <dos.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <direct.h>
#include <ctype.h>
#include <limits.h>

#include "stack.h"

/* DOS disk accesses */
#include "dosdisk.h"


/* Define getdrive so it returns current drive, 0=A,1=B,...           */
#define getdrive() getdisk()

#include <conio.h>  /* for getch()   */


/* The default extended forms of the lines used. */
#define VERTBAR_STR  "\xB3   "                 /* |    */
#define TBAR_HORZBAR_STR "\xC3\xC4\xC4\xC4"    /* +--- */
#define CBAR_HORZBAR_STR "\xC0\xC4\xC4\xC4"    /* \--- */

/* Global flags */
#define SHOWFILESON    1  /* Display names of files in directories       */
#define SHOWFILESOFF   0  /* Don't display names of files in directories */

#define ASCIICHARS     1  /* Use ASCII [7bit] characters                 */
#define EXTENDEDCHARS  0  /* Use extended ASCII [8bit] characters        */

#define NOPAUSE        0  /* Default, don't pause after screenfull       */
#define PAUSE          1  /* Wait for keypress after each page           */


/* Global variables */
short showFiles = SHOWFILESOFF;
short charSet = EXTENDEDCHARS;
short pause = NOPAUSE;

short dspAll = 0;  /* if nonzero includes HIDDEN & SYSTEM files in output */
short dspSize = 0; /* if nonzero displays filesizes                       */
short dspAttr = 0; /* if nonzero displays file attributes [DACESHRBP]     */
short dspSumDirs = 0; /* show count of subdirectories  (per dir and total)*/


/* maintains total count, for > 4billion dirs, use a __int64 */
unsigned long totalSubDirCnt = 0;


/* text window size, used to determine when to pause,
   Note: rows is total rows available - 2
   1 is for pause message and then one to prevent auto scroll up
*/
short cols=80, rows=23;   /* determined these on startup (when possible)  */



/* Global constants */
#define SERIALLEN 16      /* Defines max size of volume & serial number   */
#define VOLLEN 128

#define MAXBUF 1024       /* Must be larger than max file path length     */
char path[MAXBUF];        /* Path to begin search from, default=current   */

#define MAXPADLEN (MAXBUF*2) /* Must be large enough to hold the maximum padding */
/* (MAXBUF/2)*4 == (max path len / min 2chars dirs "?\") * 4chars per padding    */

/* The maximum size any line of text output can be, including room for '\0'*/
#define MAXLINE 160        /* Increased to fit two lines for translations  */


/* The hard coded strings used by the following show functions.            */

/* common to many functions [Set 1] */
char newLine[MAXLINE] = "\n";

/* showUsage [Set 2] - Each %c will be replaced with proper switch/option */
char treeDescription[MAXLINE] = "Graphically displays the directory structure of a drive or path.\n";
char treeUsage[MAXLINE] =       "TREE [drive:][path] [%c%c] [%c%c]\n";
char treeFOption[MAXLINE] =     "   %c%c   Display the names of the files in each directory.\n";
char treeAOption[MAXLINE] =     "   %c%c   Use ASCII instead of extended characters.\n";

/* showInvalidUsage [Set 3] */
char invalidOption[MAXLINE] = "Invalid switch - %s\n";  /* Must include the %s for option given. */
char useTreeHelp[MAXLINE] =   "Use TREE %c? for usage information.\n"; /* %c replaced with switch */

/* showVersionInfo [Set 4] */
/* also uses treeDescription */
char treeGoal[MAXLINE] =      "Written to work with FreeDOS\n";
char treePlatforms[MAXLINE] = "Win32(c) console and DOS with LFN support.\n";
char version[MAXLINE] =       "Version %s\n"; /* Must include the %s for version string. */
char writtenBy[MAXLINE] =     "Written by: Kenneth J. Davis\n";
char writtenDate[MAXLINE] =   "Date:       2000, 2001, 2004\n";
char contact[MAXLINE] =       "Contact:    jeremyd@computer.org\n";
char copyright[MAXLINE] =     "Copyright (c): Public Domain [United States Definition]\n";

/* showInvalidDrive [Set 5] */
char invalidDrive[MAXLINE] = "Invalid drive specification\n";

/* showInvalidPath [Set 6] */
char invalidPath[MAXLINE] = "Invalid path - %s\n"; /* Must include %s for the invalid path given. */

/* Misc Error messages [Set 7] */
/* showBufferOverrun */
/* %u required to show what the buffer's current size is. */
char bufferToSmall[MAXLINE] = "Error: File path specified exceeds maximum buffer = %u bytes\n";
/* showOutOfMemory */
/* %s required to display what directory we were processing when ran out of memory. */
char outOfMemory[MAXLINE] = "Out of memory on subdirectory: %s\n";

/* main [Set 1] */
char pathListingNoLabel[MAXLINE] = "Directory PATH listing\n";
char pathListingWithLabel[MAXLINE] = "Directory PATH listing for Volume %s\n"; /* %s for label */
char serialNumber[MAXLINE] = "Volume serial number is %s\n"; /* Must include %s for serial #   */
char noSubDirs[MAXLINE] = "No subdirectories exist\n\n";
char pauseMsg[MAXLINE]  = " --- Press any key to continue ---\n";

/* Option Processing - parseArguments [Set 8]      */
char optionchar1 = '/';  /* Primary character used to determine option follows  */
char optionchar2 = '-';  /* Secondary character used to determine option follows  */
const char OptShowFiles[2] = { 'F', 'f' };  /* Show files */
const char OptUseASCII[2]  = { 'A', 'a' };  /* Use ASCII only */
const char OptVersion[2]   = { 'V', 'v' };  /* Version information */
const char OptSFNs[2]      = { 'S', 's' };  /* Shortnames only (disable LFN support) */
const char OptPause[2]     = { 'P', 'p' };  /* Pause after each page (screenfull) */
const char OptDisplay[2]   = { 'D', 'd' };  /* modify Display settings */


/* Procedures */


#define FILE_TYPE_UNKNOWN 0x00
#define FILE_TYPE_DISK    0x01
#define FILE_TYPE_CHAR    0x02
#define FILE_TYPE_PIPE    0x03
#define FILE_TYPE_REMOTE  0x80

/* Returns file type of stdout.
 * Output, one of predefined values above indicating if
 *         handle refers to file (FILE_TYPE_DISK), a
 *         device such as CON (FILE_TYPE_CHAR), a
 *         pipe (FILE_TYPE_PIPE), or unknown.
 * On errors or unspecified input, FILE_TYPE_UNKNOWN
 * is returned. */
static unsigned char GetStdoutType(void) {
  union REGS r;

  r.x.ax = 0x4400;                 /* DOS 2+, IOCTL Get Device Info */
  r.x.bx = 0x0001;                 /* file handle (stdout) */

  /* We assume hFile is an opened DOS handle, & if invalid call should fail. */
  intdos(&r, &r);     /* Clib function to invoke DOS int21h call   */

  /* error? */
  if (r.x.cflag != 0) return(FILE_TYPE_UNKNOWN);

  /* if bit 7 is set it is a char dev */
  if (r.x.dx & 0x80) return(FILE_TYPE_CHAR);

  /* file is remote */
  if (r.x.dx & 0x8000) return(FILE_TYPE_REMOTE);

  /* assume valid file handle */
  return(FILE_TYPE_DISK);
}


/* sets rows & cols to size of actual console window
 * force NOPAUSE if appears output redirected to a file or
 * piped to another program
 * Uses hard coded defaults and leaves pause flag unchanged
 * if unable to obtain information.
 */
static void getConsoleSize(void) {
  unsigned short far *bios_cols = (unsigned short far *)MK_FP(0x40,0x4A);
  unsigned short far *bios_size = (unsigned short far *)MK_FP(0x40,0x4C);

  switch (GetStdoutType()) {
    case FILE_TYPE_DISK: /* e.g. redirected to a file, tree > filelist.txt */
      /* Output to a file or program, so no screen to fill (no max cols or rows) */
      pause = NOPAUSE;   /* so ignore request to pause */
      break;
    case FILE_TYPE_CHAR:  /* e.g. the console */
    case FILE_TYPE_UNKNOWN:  /* else at least attempt to get proper limits */
    case FILE_TYPE_REMOTE:
    default:
      if ((*bios_cols == 0) || (*bios_size == 0)) { /* MDA does not report size */
        cols = 80;
        rows = 23;
      } else {
        cols = *bios_cols;
        rows = *bios_size / cols / 2;
        if (rows > 2) rows -= 2; /* necessary to keep screen from scrolling */
      }
      break;
  }
}


/* when pause == NOPAUSE then identical to printf,
   otherwise counts lines printed and pauses as needed.
   Should be used for all messages printed that do not
   immediately exit afterwards (else printf may be used).
   May display N too many lines before pause if line is
   printed that exceeds cols [N=linelen%cols] and lacks
   any newlines (but this should not occur in tree).
*/
#include <stdarg.h>  /* va_list, va_start, va_end */
static int pprintf(const char *msg, ...) {
  static int lineCnt = -1;
  static int lineCol = 0;
  va_list argptr;
  int cnt;
  char buffer[MAXBUF];

  if (lineCnt == -1) lineCnt = rows;

  va_start(argptr, msg);
  cnt = vsprintf(buffer, msg, argptr);
  va_end(argptr);

  if (pause == PAUSE)
  {
    char *l = buffer;
    char *t;
    /* cycle through counting newlines and lines > cols */
    for (t = strchr(l, '\n'); t != NULL; t = strchr(l, '\n'))
    {
      char c;
      t++;             /* point to character after newline */
      c = *t;          /* store current value */
      *t = '\0';       /* mark as end of string */

      /* print all but last line of a string that wraps across rows */
      /* adjusting for partial lines printed without the newlines   */
      while (strlen(l)+lineCol > cols)
      {
        char c = l[cols-lineCol];
        l[cols-lineCol] = '\0';
        printf("%s", l);
        l[cols-lineCol] = c;
        l += cols-lineCol;

        lineCnt--;  lineCol = 0;
        if (!lineCnt) { lineCnt= rows;  fflush(NULL);  fprintf(stderr, "%s", pauseMsg);  getch(); }
      }

      printf("%s", l); /* print out this line */
      *t = c;          /* restore value */
      l = t;           /* mark beginning of next line */

      lineCnt--;  lineCol = 0;
      if (!lineCnt) { lineCnt= rows;  fflush(NULL);  fprintf(stderr, "%s", pauseMsg);  getch(); }
    }
    printf("%s", l);   /* print rest of string that lacks newline */
    lineCol = strlen(l);
  }
  else  /* NOPAUSE */
    printf("%s", buffer);

  return cnt;
}


/* Displays to user valid options then exits program indicating no error */
static void showUsage(void) {
  printf("%s%s%s%s", treeDescription, newLine, treeUsage, newLine);
  printf("%s%s%s", treeFOption, treeAOption, newLine);
  exit(1);
}


/* Displays error message then exits indicating error */
static void showInvalidUsage(char * badOption) {
  printf(invalidOption, badOption);
  printf("%s%s", useTreeHelp, newLine);
  exit(1);
}


/* Displays author, copyright, etc info, then exits indicating no error. */
static void showVersionInfo(void) {
  printf("%s%s%s%s%s", treeDescription, newLine, treeGoal, treePlatforms, newLine);
  printf(version, VERSION);
  printf("%s%s%s%s%s", writtenBy, writtenDate, contact, newLine, newLine);
  printf("%s%s", copyright, newLine);
  exit(1);
}


/* Displays error messge for invalid drives and exits */
static void showInvalidDrive(void) {
  printf(invalidDrive);
  exit(1);
}


/* Takes a fullpath, splits into drive (C:, or \\server\share) and path */
static void splitpath(char *fullpath, char *drive, char *path);

/**
 * Takes a given path, strips any \ or / that may appear on the end.
 * Returns a pointer to its static buffer containing path
 * without trailing slash and any necessary display conversions.
 */
static char *fixPathForDisplay(char *path);

/* Displays error message for invalid path; Does NOT exit */
static void showInvalidPath(char *path) {
  char partialPath[MAXBUF], dummy[MAXBUF];

  pprintf("%s\n", path);
  splitpath(path, dummy, partialPath);
  pprintf(invalidPath, fixPathForDisplay(partialPath));
}

/* Displays error message for out of memory; Does NOT exit */
static void showOutOfMemory(char *path) {
  pprintf(outOfMemory, path);
}

/* Displays buffer exceeded message and exits */
static void showBufferOverrun(WORD maxSize) {
  printf(bufferToSmall, maxSize);
  exit(1);
}


/**
 * Takes a fullpath, splits into drive (C:, or \\server\share) and path
 * It assumes a colon as the 2nd character means drive specified,
 * a double slash \\ (\\, //, \/, or /\) specifies network share.
 * If neither drive nor network share, then assumes whole fullpath
 * is path, and sets drive to "".
 * If drive specified, then set drive to it and colon, eg "C:", with
 * the rest of fullpath being set in path.
 * If network share, the slash slash followed by the server name,
 * another slash and either the rest of fullpath or up to, but not
 * including, the next slash are placed in drive, eg "\\KJD\myshare";
 * the rest of the fullpath including the slash are placed in
 * path, eg "\mysubdir"; where fullpath is "\\KJD\myshare\mysubdir".
 * None of these may be NULL, and drive and path must be large
 * enough to hold fullpath.
 */
static void splitpath(char *fullpath, char *drive, char *path) {
  char *src = fullpath;
  char oldchar;

  /* If either network share or path only starting at root directory */
  if ( (*src == '\\') || (*src == '/') )
  {
    src++;

    if ( (*src == '\\') || (*src == '/') ) /* network share */
    {
      src++;

      /* skip past server name */
      while ( (*src != '\\') && (*src != '/') && (*src != '\0') )
        src++;

      /* skip past slash (\ or /) separating  server from share */
      if (*src != '\0') src++;

      /* skip past share name */
      while ( (*src != '\\') && (*src != '/') && (*src != '\0') )
        src++;

      /* src points to start of path, either a slash or '\0' */
      oldchar = *src;
      *src = '\0';

      /* copy server name to drive */
      strcpy(drive, fullpath);

      /* restore character used to mark end of server name */
      *src = oldchar;

      /* copy path */
      strcpy(path, src);
    }
    else /* path only starting at root directory */
    {
      /* no drive, so set path to same as fullpath */
      strcpy(drive, "");
      strcpy(path, fullpath);
    }
  }
  else
  {
    if (*src != '\0') src++;

    /* Either drive and path or path only */
    if (*src == ':')
    {
      /* copy drive specified */
      *drive = *fullpath;  drive++;
      *drive = ':';        drive++;
      *drive = '\0';

      /* copy path */
      src++;
      strcpy(path, src);
    }
    else
    {
      /* no drive, so set path to same as fullpath */
      strcpy(drive, "");
      strcpy(path, fullpath);
    }
  }
}


/* Converts given path to full path */
static void getProperPath(char *fullpath) {
  char drive[MAXBUF];
  char path[MAXBUF];

  splitpath(fullpath, drive, path);

  /* if no drive specified use current */
  if (drive[0] == '\0')
  {
    sprintf(fullpath, "%c:%s", 'A'+ getdrive(), path);
  }
  else if (path[0] == '\0') /* else if drive but no path specified */
  {
    if ((drive[0] == '\\') || (drive[0] == '/'))
    {
      /* if no path specified and network share, use root   */
      sprintf(fullpath, "%s%s", drive, "\\");
    }
    else
    {
      /* if no path specified and drive letter, use current path */
      sprintf(fullpath, "%s%s", drive, ".");
    }
  }
  /* else leave alone, it has both a drive and path specified */
}


/* Parses the command line and sets global variables. */
static void parseArguments(int argc, char *argv[]) {
  int i;     /* temp loop variable */

  /* if no drive specified on command line, use current */
  sprintf(path, "%c:.", 'A'+ getdrive());

  for (i = 1; i < argc; i++)
  {
    /* Check if user is giving an option or drive/path */
    if ((argv[i][0] == optionchar1) || (argv[i][0] == optionchar2) )
    {
      /* check multi character options 1st */
      if ((argv[i][1] == OptDisplay[0]) || (argv[i][1] == OptDisplay[1]))
      {
        switch (argv[i][2] & 0xDF)
        {
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
      }
      else /* a 1 character option (or invalid) */
      {
        if (argv[i][2] != '\0')
          showInvalidUsage(argv[i]);

        /* Must check both uppercase and lowercase                        */
        if ((argv[i][1] == OptShowFiles[0]) || (argv[i][1] == OptShowFiles[1]))
          showFiles = SHOWFILESON; /* set file display flag appropriately */
        else if ((argv[i][1] == OptUseASCII[0]) || (argv[i][1] == OptUseASCII[1]))
          charSet = ASCIICHARS;    /* set charset flag appropriately      */
        else if (argv[i][1] == '?')
          showUsage();             /* show usage info and exit            */
        else if ((argv[i][1] == OptVersion[0]) || (argv[i][1] == OptVersion[1]))
          showVersionInfo();       /* show version info and exit          */
        else if ((argv[i][1] == OptSFNs[0]) || (argv[i][1] == OptSFNs[1]))
          LFN_Enable_Flag = LFN_DISABLE;         /* force shortnames only */
        else if ((argv[i][1] == OptPause[0]) || (argv[i][1] == OptPause[1]))
          pause = PAUSE;     /* wait for keypress after each page (pause) */
        else /* Invalid or unknown option */
          showInvalidUsage(argv[i]);
      }
    }
    else /* should be a drive/path */
    {
      char *dptr = path;
      char *cptr;

      if (strlen(argv[i]) > MAXBUF) showBufferOverrun(MAXBUF);

      /* copy path over, making all caps to look prettier, can be strcpy */
      for (cptr = argv[i]; *cptr != '\0'; cptr++, dptr++) {
        *dptr = toupper(*cptr);
      }
      *dptr = '\0';

      /* Converts given path to full path */
      getProperPath(path);
    }
  }
}


/**
 * Fills in the serial and volume variables with the serial #
 * and volume found using path.
 * If there is an error getting the volume & serial#, then an
 * error message is displayed and the program exits.
 * Volume and/or serial # returned may be blank if the path specified
 * does not contain them, or an error retrieving
 * (ie UNC paths under DOS), but path is valid.
 */
static void GetVolumeAndSerial(char *volume, char *serial, char *path) {
  char rootPath[MAXBUF];
  char dummy[MAXBUF];
  union serialNumber {
    DWORD serialFull;
    struct {
      WORD a;
      WORD b;
    } serialParts;
  } serialNum;

  /* get drive letter or share server\name */
  splitpath(path, rootPath, dummy);
  strcat(rootPath, "\\");

  if (GetVolumeInformation(rootPath, volume, VOLLEN, &serialNum.serialFull) == 0) {
    showInvalidDrive();
  }

  if (serialNum.serialFull == 0)
    serial[0] = '\0';
  else
    sprintf(serial, "%04X:%04X",
      serialNum.serialParts.b, serialNum.serialParts.a);
}


/**
 * Stores directory information obtained from FindFirst/Next that
 * we may wish to make use of when displaying directory entry.
 * e.g. attribute, dates, etc.
 */
typedef struct DIRDATA
{
  DWORD subdirCnt;          /* how many subdirectories we have */
  DWORD fileCnt;            /* how many [normal] files we have */
  DWORD dwDirAttributes;    /* Directory attributes            */
} DIRDATA;

/**
 * Contains the information stored in a Stack necessary to allow
 * non-recursive function to display directory tree.
 */
typedef struct SUBDIRINFO
{
  struct SUBDIRINFO * parent; /* points to parent subdirectory                */
  char *currentpath;    /* Stores the full path this structure represents     */
  char *subdir;         /* points to last subdir within currentpath           */
  char *dsubdir;        /* Stores a display ready directory name              */
  long subdircnt;       /* Initially a count of how many subdirs in this dir  */
  HANDLE findnexthnd;   /* The handle returned by findfirst, used in findnext */
  struct DIRDATA ddata; /* Maintain directory information, eg attributes      */
} SUBDIRINFO;


/**
 * Returns 0 if no subdirectories, count if has subdirs.
 * Path must end in slash \ or /
 * On error (invalid path) displays message and returns -1L.
 * Stores additional directory data in ddata if non-NULL
 * and path is valid.
 */
static long hasSubdirectories(char *path, DIRDATA *ddata) {
  static struct WIN32_FIND_DATA findData;
  HANDLE hnd;
  static char buffer[MAXBUF];
  int hasSubdirs = 0;

  /* get the handle to start with (using wildcard spec) */
  strcpy(buffer, path);
  strcat(buffer, "*");

  /* Use FindFirstFileEx when available (falls back to FindFirstFile).
   * Allows us to limit returned results to just directories
   * if supported by underlying filesystem.
   */
  hnd = FindFirstFile(buffer, &findData);
  if (hnd == INVALID_HANDLE_VALUE)
  {
    showInvalidPath(path); /* Display error message */
    return -1L;
  }


  /*  cycle through entries counting directories found until no more entries */
  do {
    if (((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) &&
	((findData.dwFileAttributes &
	 (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM)) == 0 || dspAll) )
    {
      if ( (strcmp(findData.cFileName, ".") != 0) && /* ignore initial [current] path */
           (strcmp(findData.cFileName, "..") != 0) ) /* and ignore parent path */
        hasSubdirs++;      /* subdir of initial path found, so increment counter */
    }
  } while(FindNextFile(hnd, &findData) != 0);

  /* prevent resource leaks, close the handle. */
  FindClose(hnd);

  if (ddata != NULL)  // don't bother if user doesn't want them
  {
    /* The root directory of a volume (including non root paths
       corresponding to mount points) may not have a current (.) and
       parent (..) entry.  So we can't get attributes for initial
       path in above loop from the FindFile call as it may not show up
       (no . entry).  So instead we explicitly get them here.
    */
    if ((ddata->dwDirAttributes = GetFileAttributes(path)) == (DWORD)-1)
    {
      //printf("ERROR: unable to get file attr, %i\n", GetLastError());
      ddata->dwDirAttributes = 0;
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
static SUBDIRINFO *newSubdirInfo(SUBDIRINFO *parent, char *subdir, char *dsubdir) {
  int parentLen, subdirLen;
  SUBDIRINFO *temp;

  /* Get length of parent directory */
  if (parent == NULL)
    parentLen = 0;
  else
    parentLen = strlen(parent->currentpath);

  /* Get length of subdir, add 1 if does not end in slash */
  subdirLen = strlen(subdir);
  if ((subdirLen < 1) || ( (*(subdir+subdirLen-1) != '\\') && (*(subdir+subdirLen-1) != '/') ) )
    subdirLen++;

  temp = (SUBDIRINFO *)malloc(sizeof(SUBDIRINFO));
  if (temp == NULL)
  {
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
  if (parent == NULL)
    strcpy(temp->currentpath, "");
  else
    strcpy(temp->currentpath, parent->currentpath);
  strcat(temp->currentpath, subdir);
  /* if subdir[subdirLen-1] == '\0' then we must append a slash */
  if (*(subdir+subdirLen-1) == '\0')
    strcat(temp->currentpath, "\\");
  temp->subdir = temp->currentpath+parentLen;
  strcpy(temp->dsubdir, dsubdir);
  if ((temp->subdircnt = hasSubdirectories(temp->currentpath, &(temp->ddata))) == -1L)
  {
    free (temp->currentpath);
    free (temp->dsubdir);
    free(temp);
    return NULL;
  }
  temp->findnexthnd = INVALID_HANDLE_VALUE;

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
    if (charSet == EXTENDEDCHARS) {
      strcat(padding, VERTBAR_STR);
    } else {
      strcat(padding, "|   ");
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
 * Takes a given path, strips any \ or / that may appear on the end.
 * Returns a pointer to its static buffer containing path
 * without trailing slash and any necessary display conversions.
 */
static char *fixPathForDisplay(char *path) {
  static char buffer[MAXBUF];
  int pathlen;

  strcpy(buffer, path);
  pathlen = strlen(buffer);
  if (pathlen > 1) {
    pathlen--;
    if ((buffer[pathlen] == '\\') || (buffer[pathlen] == '/')) {
      buffer[pathlen] = '\0'; // strip off trailing slash on end
    }
  }

  return buffer;
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
  if (padding != NULL)
    pprintf("%s", padding);

  /* print lead padding except for initial directory */
  if (moreSubdirsFollow >= 0)
  {
    if (charSet == EXTENDEDCHARS)
    {
      if (moreSubdirsFollow)
        pprintf("%s", TBAR_HORZBAR_STR);
      else
        pprintf("%s", CBAR_HORZBAR_STR);
    } else {
      if (moreSubdirsFollow)
        pprintf("+---");
      else
        pprintf("\\---");
    }
  }

  /* optional display data */
  if (dspAttr)  /* attributes */
    pprintf("[%c%c%c%c%c] ",
      (ddata->dwDirAttributes & FILE_ATTRIBUTE_DIRECTORY)?'D':' ',  /* keep this one? its always true */
      (ddata->dwDirAttributes & FILE_ATTRIBUTE_ARCHIVE)?'A':' ',
      (ddata->dwDirAttributes & FILE_ATTRIBUTE_SYSTEM)?'S':' ',
      (ddata->dwDirAttributes & FILE_ATTRIBUTE_HIDDEN)?'H':' ',
      (ddata->dwDirAttributes & FILE_ATTRIBUTE_READONLY)?'R':' '
    );

  /* display directory name */
  pprintf("%s\n", currentpath);
}


/**
 * Displays summary information about directory.
 * Expects to be called after displayFiles (optionally called)
 */
static void displaySummary(char *padding, int hasMoreSubdirs, DIRDATA *ddata) {
  addPadding(padding, hasMoreSubdirs);

  if (dspSumDirs) {
    if (showFiles == SHOWFILESON) {
      /* print File summary with lead padding, add filesize to it */
      pprintf("%s%lu files\n", padding, ddata->fileCnt);
    }

    /* print Directory summary with lead padding */
    pprintf("%s%lu subdirectories\n", padding, ddata->subdirCnt);

    /* show [nearly] blank line after summary */
    pprintf("%s\n", padding);
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
static int displayFiles(char *path, char *padding, int hasMoreSubdirs, DIRDATA *ddata) {
  static char buffer[MAXBUF];
  struct WIN32_FIND_DATA entry; /* current directory entry info    */
  HANDLE dir;         /* Current directory entry working with      */
  unsigned long filesShown = 0;

  /* get handle for files in current directory (using wildcard spec) */
  strcpy(buffer, path);
  strcat(buffer, "*");
  dir = FindFirstFile(buffer, &entry);
  if (dir == INVALID_HANDLE_VALUE)
    return -1;

  addPadding(padding, hasMoreSubdirs);

  /* cycle through directory printing out files. */
  do
  {
    /* print padding followed by filename */
    if ( ((entry.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) &&
         ( ((entry.dwFileAttributes & (FILE_ATTRIBUTE_HIDDEN |
         FILE_ATTRIBUTE_SYSTEM)) == 0)  || dspAll) )
    {
      /* print lead padding */
      pprintf("%s", padding);

      /* optional display data */
      if (dspAttr)  /* file attributes */
        pprintf("[%c%c%c%c] ",
          (entry.dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE)?'A':' ',
          (entry.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM)?'S':' ',
          (entry.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)?'H':' ',
          (entry.dwFileAttributes & FILE_ATTRIBUTE_READONLY)?'R':' '
        );

      if (dspSize)  /* file size */
      {
        if (entry.nFileSizeHigh)
        {
          pprintf("******** ");  /* error exceed max value we can display, > 4GB */
        }
        else
        {
          if (entry.nFileSizeLow < 1048576l)  /* if less than a MB, display in bytes */
            pprintf("%10lu ", entry.nFileSizeLow);
          else                               /* otherwise display in KB */
            pprintf("%8luKB ", entry.nFileSizeLow/1024UL);
        }
      }

      /* print filename */
      pprintf("%s\n", entry.cFileName);

      filesShown++;
    }
  } while(FindNextFile(dir, &entry) != 0);

  if (filesShown)
  {
    pprintf("%s\n", padding);
  }

  /* cleanup directory search */
  FindClose(dir);
  /* dir = NULL; */

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
 * return INVALID_HANDLE_VALUE.  If successful, returns FindFile handle.
 */
static HANDLE cycleFindResults(HANDLE findnexthnd, struct WIN32_FIND_DATA *entry, char *subdir, char *dsubdir) {
  /* cycle through directory until 1st non . or .. directory is found. */
  do
  {
    /* skip files & hidden or system directories */
    if ((((entry->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) ||
         ((entry->dwFileAttributes &
          (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM)) != 0  && !dspAll) ) ||
        ((strcmp(entry->cFileName, ".") == 0) ||
         (strcmp(entry->cFileName, "..") == 0)) )
    {
      if (FindNextFile(findnexthnd, entry) == 0)
      {
        FindClose(findnexthnd);      // prevent resource leaks
        return INVALID_HANDLE_VALUE; // no subdirs found
      }
    }
    else
    {
      /* set display name */
      strcpy(dsubdir, entry->cFileName);

      /* set canical name to use for further FindFile calls */
      /* use short file name if exists as lfn may contain unicode values converted
       * to default character (eg. ?) and so not a valid path.
       */
      strcpy(subdir, entry->cFileName);
      strcat(subdir, "\\");
    }
  } while (!*subdir); // while (subdir is still blank)

  return findnexthnd;
}


/* FindFile buffer used by findFirstSubdir and findNextSubdir only */
static struct WIN32_FIND_DATA findSubdir_entry; /* current directory entry info    */

/**
 * Given the current path, find the 1st subdirectory.
 * The subdirectory found is stored in subdir.
 * subdir is cleared on error or no subdirectories.
 * Returns the findfirst search HANDLE, which should be passed to
 * findclose when directory has finished processing, and can be
 * passed to findnextsubdir to find subsequent subdirectories.
 * Returns INVALID_HANDLE_VALUE on error.
 * currentpath must end in \
 */
static HANDLE findFirstSubdir(char *currentpath, char *subdir, char *dsubdir) {
  static char buffer[MAXBUF];
  HANDLE dir;         /* Current directory entry working with      */

  /* get handle for files in current directory (using wildcard spec) */
  strcpy(buffer, currentpath);
  strcat(buffer, "*");

  dir = FindFirstFile(buffer, &findSubdir_entry);
  if (dir == INVALID_HANDLE_VALUE)
  {
    showInvalidPath(currentpath);
    return INVALID_HANDLE_VALUE;
  }

  /* clear result path */
  strcpy(subdir, "");

  return cycleFindResults(dir, &findSubdir_entry, subdir, dsubdir);
}

/**
 * Given a search HANDLE, will find the next subdirectory,
 * setting subdir to the found directory name.
 * dsubdir is the name to display (lfn or sfn as appropriate)
 * currentpath must end in \
 * If a subdirectory is found, returns 0, otherwise returns 1
 * (either error or no more files).
 */
static int findNextSubdir(HANDLE findnexthnd, char *subdir, char *dsubdir) {
  /* clear result path */
  strcpy(subdir, "");

  if (FindNextFile(findnexthnd, &findSubdir_entry) == 0) return 1; // no subdirs found

  if (cycleFindResults(findnexthnd, &findSubdir_entry, subdir, dsubdir) == INVALID_HANDLE_VALUE)
    return 1;
  else
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
  char subdir[MAXBUF];
  char dsubdir[MAXBUF];
  SUBDIRINFO *sdi;

  STACK s;
  stackDefaults(&s);
  stackInit(&s);

  if ( (sdi = newSubdirInfo(NULL, initialpath, initialpath)) == NULL)
    return 0L;
  stackPushItem(&s, sdi);

  /* Store count of subdirs in initial path so can display message if none. */
  subdirsInInitialpath = sdi->subdircnt;

  do
  {
    sdi = (SUBDIRINFO *)stackPopItem(&s);

    if (sdi->findnexthnd == INVALID_HANDLE_VALUE)  // findfirst not called yet
    {
      // 1st time this subdirectory processed, so display its name & possibly files
      if (sdi->parent == NULL) // if initial path
      {
        // display initial path
        showCurrentPath(/*sdi->dsubdir*/initialpath, NULL, -1, &(sdi->ddata));
      }
      else // normal processing (display path, add necessary padding)
      {
        showCurrentPath(sdi->dsubdir, padding, (sdi->parent->subdircnt > 0L)?1 : 0, &(sdi->ddata));
        addPadding(padding, (sdi->parent->subdircnt > 0L)?1 : 0);
      }

      if (showFiles == SHOWFILESON)  displayFiles(sdi->currentpath, padding, (sdi->subdircnt > 0L)?1 : 0, &(sdi->ddata));
      displaySummary(padding, (sdi->subdircnt > 0L)?1 : 0, &(sdi->ddata));
    }

    if (sdi->subdircnt > 0) /* if (there are more subdirectories to process) */
    {
      int flgErr;
      if (sdi->findnexthnd == INVALID_HANDLE_VALUE)
      {
        sdi->findnexthnd = findFirstSubdir(sdi->currentpath, subdir, dsubdir);
        flgErr = (sdi->findnexthnd == INVALID_HANDLE_VALUE);
      }
      else
      {
        flgErr = findNextSubdir(sdi->findnexthnd, subdir, dsubdir);
      }

      if (flgErr) // don't add invalid paths to stack
      {
        printf("INTERNAL ERROR: subdir count changed, expecting %li more!\n", sdi->subdircnt+1L);

        sdi->subdircnt = 0; /* force subdir counter to 0, none left */
        stackPushItem(&s, sdi);
      }
      else
      {
        sdi->subdircnt = sdi->subdircnt - 1L; /* decrement subdirs left count */
        stackPushItem(&s, sdi);

        /* store necessary information, validate subdir, and if no error store it. */
        if ((sdi = newSubdirInfo(sdi, subdir, dsubdir)) != NULL)
          stackPushItem(&s, sdi);
      }
    }
    else /* this directory finished processing, so free resources */
    {
      /* Remove the padding for this directory, all but initial path. */
      if (sdi->parent != NULL)
        removePadding(padding);

      /* Prevent resource leaks, by ending findsearch and freeing memory. */
      FindClose(sdi->findnexthnd);
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


static void FixOptionText(void) {
  char buffer[MAXLINE];  /* sprintf can have problems with src==dest */

  /* Handle %c for options within messages using Set 8 */
  strcpy(buffer, treeUsage);
  sprintf(treeUsage, buffer, optionchar1, OptShowFiles[0], optionchar1, OptUseASCII[0]);
  strcpy(buffer, treeFOption);
  sprintf(treeFOption, buffer, optionchar1, OptShowFiles[0]);
  strcpy(buffer, treeAOption);
  sprintf(treeAOption, buffer, optionchar1, OptUseASCII[0]);
  strcpy(buffer, useTreeHelp);
  sprintf(useTreeHelp, buffer, optionchar1);
}


/* Loads all messages from the message catalog. */
static void loadAllMessages(void) {
  /* Changes %c in certain lines with proper option characters. */
  FixOptionText();
}


int main(int argc, char **argv) {
  char serial[SERIALLEN]; /* volume serial #  0000:0000 */
  char volume[VOLLEN];    /* volume name (label), possibly none */

  /* Load all text from message catalog (or uses hard coded text) */
  loadAllMessages();

  /* Parse any command line arguments, obtain path */
  parseArguments(argc, argv);

  /* Initialize screen size, may reset pause to NOPAUSE if redirected */
  getConsoleSize();

  /* Get Volume & Serial Number */
  GetVolumeAndSerial(volume, serial, path);
  if (strlen(volume) == 0)
    pprintf(pathListingNoLabel);
  else
    pprintf(pathListingWithLabel, volume);
  if (serial[0] != '\0')  /* Don't print anything if no serial# found */
    pprintf(serialNumber, serial);

  /* now traverse & print tree, returns nonzero if has subdirectories */
  if (traverseTree(path) == 0)
    pprintf(noSubDirs);
  else if (dspSumDirs) /* show count of directories processed */
    pprintf("\n    %lu total directories\n", totalSubDirCnt+1);

  return 0;
}
