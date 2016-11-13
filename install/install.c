/*
 * SVAROG386 INSTALL
 * COPYRIGHT (C) 2016 MATEUSZ VISTE
 *
 * http://svarog386.sf.net
 */

#include <dos.h>
#include <direct.h>  /* mkdir() */
#include <stdio.h>   /* printf() and friends */
#include <stdlib.h>  /* system() */
#include <string.h>  /* memcpy() */
#include <unistd.h>

#include "kitten\kitten.h"

#include "input.h"
#include "video.h"


/* color scheme (color, mono) */
static unsigned short COLOR_TITLEBAR[2] = {0x7000,0x7000};
static unsigned short COLOR_BODY[2] = {0x1700,0x0700};
static unsigned short COLOR_SELECT[2] = {0x7000,0x7000};
static unsigned short COLOR_SELECTCUR[2] = {0x1F00,0x0700};

/* mono flag */
static int mono = 0;

/* how much disk space does Svarog386 require (in MiB) */
#define SVAROG_DISK_REQ 8


/* reboot the computer */
static void reboot(void) {
  void ((far *bootroutine)()) = (void (far *)()) 0xFFFF0000L;
  int far *rstaddr = (int far *)0x00400072L; /* BIOS boot flag is at 0040:0072 */
  *rstaddr = 0x1234; /* 0x1234 = warm boot, 0 = cold boot */
  (*bootroutine)(); /* jump to the BIOS reboot routine at FFFF:0000 */
}


/* an NLS wrapper around video_putstring() */
static void putstringnls(int y, int x, unsigned short attr, int nlsmaj, int nlsmin, char *s) {
  s = kittengets(nlsmaj, nlsmin, s);
  video_putstring(y, x, attr, s);
}


#define LDEC(x,y) (((unsigned short)x << 8) | (unsigned short)y)
/* provides codepage and country files required by lang */
static int getnlscp(char *lang, int *egafile) {
  unsigned short l;
  l = lang[0];
  l <<= 8;
  l |= lang[1];
  switch (l) {
    case LDEC('E','N'):
      *egafile = 0;
      return(437);
    case LDEC('P','L'):
      *egafile = 10;
      return(991);
  }
  *egafile = 0;
  return(437);
}


static int menuselect(int ypos, int xpos, int height, char **list) {
  int i, offset = 0, res = 0, count, width = 0;
  /* count how many languages there is */
  for (count = 0; list[count] != NULL; count++) {
    int len = strlen(list[count]);
    if (len > width) width = len;
  }

  /* if xpos negative, means 'center out' */
  if (xpos < 0) xpos = 39 - (width >> 1);

  video_putchar(ypos, xpos+width+2, COLOR_SELECT[mono], 0xBF);         /*       \ */
  video_putchar(ypos, xpos-1, COLOR_SELECT[mono], 0xDA);               /*  /      */
  video_putchar(ypos+height-1, xpos-1, COLOR_SELECT[mono], 0xC0);      /*  \      */
  video_putchar(ypos+height-1, xpos+width+2, COLOR_SELECT[mono], 0xD9);/*      /  */
  video_putcharmulti(ypos, xpos, COLOR_SELECT[mono], 0xC4, width + 2, 1);
  video_putcharmulti(ypos+height-1, xpos, COLOR_SELECT[mono], 0xC4, width + 2, 1);
  video_putcharmulti(ypos+1, xpos-1, COLOR_SELECT[mono], 0xB3, height - 2, 80);
  video_putcharmulti(ypos+1, xpos+width+2, COLOR_SELECT[mono], 0xB3, height - 2, 80);

  for (;;) {
    int key;
    /* list of selectable items */
    for (i = 0; i < height - 2; i++) {
      if (i + offset == res) {
        video_putchar(ypos + 1 + i, xpos, COLOR_SELECTCUR[mono], 16);
        video_putchar(ypos + 1 + i, xpos+width+1, COLOR_SELECTCUR[mono], 17);
        video_movecursor(ypos + 1 + i, xpos);
        video_putstringfix(ypos + 1 + i, xpos+1, COLOR_SELECTCUR[mono], list[i + offset], width);
      } else if (i + offset < count) {
        video_putchar(ypos + 1 + i, xpos, COLOR_SELECT[mono], ' ');
        video_putchar(ypos + 1 + i, xpos+width+1, COLOR_SELECT[mono], ' ');
        video_putstringfix(ypos + 1 + i, xpos+1, COLOR_SELECT[mono], list[i + offset], width);
      } else {
        video_putcharmulti(ypos + 1 + i, xpos, COLOR_SELECT[mono], ' ', width+2, 1);
      }
    }
    key = input_getkey();
    if (key == 0x0D) { /* ENTER */
      return(res);
    } else if (key == 0x148) { /* up */
      if (res > 0) {
        res--;
        if (res < offset) offset = res;
      }
    } else if (key == 0x150) { /* down */
      if (res+1 < count) {
        res++;
        if (res > offset + height - 3) offset = res - (height - 3);
      }
    } else if (key == 0x147) { /* home */
      res = 0;
      offset = 0;
    } else if (key == 0x14F) { /* end */
      res = count - 1;
      if (res > offset + height - 3) offset = res - (height - 3);
    } else if (key == 0x1B) {  /* ESC */
      return(-1);
    } else {
      char buf[8];
      sprintf(buf, "0x%02X ", key);
      video_putstring(1, 0, COLOR_BODY[mono], buf);
    }
  }
}

static void newscreen(void) {
  int x;
  char *title;
  title = kittengets(0, 0, "SVAROG386 INSTALLATION");
  for (x = 0; x < 80; x++) video_putchar(0, x, COLOR_TITLEBAR[mono], ' ');
  video_putstring(0, 40 - (strlen(title) >> 1), COLOR_TITLEBAR[mono], title);
  video_clear(COLOR_BODY[mono], 80);
  video_movecursor(25,0);
}

static int selectlang(char *lang) {
  int choice;
  int x;
  char *msg;
  char *code;
  char *langlist[] = {
    "English\0EN",
    "French\0FR",
    "German\0DE",
    "Italian\0IT",
    "Polish\0PL",
    "Russian\0RU",
    "Slovenian\0SL",
    "Spanish\0ES",
    "Turkish\0TR",
    NULL
  };

  newscreen();
  msg = kittengets(1, 0, "Welcome to Svarog386");
  x = 40 - (strlen(msg) >> 1);
  video_putstring(4, x, COLOR_BODY[mono], msg);
  video_putcharmulti(5, x, COLOR_BODY[mono], '=', strlen(msg), 1);
  putstringnls(8, 2, COLOR_BODY[mono], 1, 1, "Please select your language from the list below:");
  choice = menuselect(10, -1, 12, langlist);
  if (choice < 0) return(-1);
  /* write short language code into lang */
  for (code = langlist[choice]; *code != 0; code++);
  memcpy(lang, code + 1, 2);
  lang[2] = 0;
  return(0);
}


/* returns 0 if installation must proceed, non-zero otherwise */
static int welcomescreen(void) {
  char *choice[] = {"Install Svarog386 to disk", "Quit to DOS", NULL};
  choice[0] = kittengets(0, 1, choice[0]);
  choice[1] = kittengets(0, 2, choice[1]);
  newscreen();
  putstringnls(4, 1, COLOR_BODY[mono], 2, 0, "You are about to install Svarog386: a free, MSDOS-compatible operating system");
  putstringnls(5, 1, COLOR_BODY[mono], 2, 1, "based on the FreeDOS kernel. Svarog386 targets 386+ computers and comes with a");
  putstringnls(6, 1, COLOR_BODY[mono], 2, 2, "variety of third-party applications.");
  putstringnls(8, 1, COLOR_BODY[mono], 2, 3, "WARNING: If your PC has another operating system installed, this other system");
  putstringnls(9, 1, COLOR_BODY[mono], 2, 4, "         might be unable to boot once Svarog386 is installed.");
  return(menuselect(13, -1, 4, choice));
}


/* returns 1 if drive is removable, 0 if not, -1 on error */
static int isdriveremovable(int drv) {
  union REGS r;
  r.x.ax = 0x4408;
  r.h.bl = drv;
  int86(0x21, &r, &r);
  /* CF set on error, AX set to 0 if removable, 1 if fixed */
  if (r.x.cflag != 0) return(-1);
  if (r.x.ax == 0) return(1);
  return(0);
}


/* returns total disk space of drive drv (in MiB, max 2048), or -1 if drive invalid */
static int disksize(int drv) {
  long res;
  union REGS r;
  r.h.ah = 0x36; /* DOS 2+ get free disk space */
  r.h.dl = drv;
  int86(0x21, &r, &r);
  if (r.x.ax == 0xffffu) return(-1); /* AX set to FFFFh if drive invalid */
  res = r.x.ax;
  res *= r.x.bx;
  res *= r.x.cx;
  return(res >> 20); /* return result after converting bytes to MiB */
}


/* returns 0 if disk is empty, non-zero otherwise */
static int diskempty(int drv) {
  unsigned int rc;
  int res;
  char buff[8];
  struct find_t fileinfo;
  sprintf(buff, "%c:\\*.*", 'A' + drv - 1);
  rc = _dos_findfirst(buff, _A_NORMAL | _A_SUBDIR | _A_HIDDEN | _A_SYSTEM, &fileinfo);
  if (rc == 0) {
    res = 1; /* call successfull means disk is not empty */
  } else {
    res = 0;
  }
  /* _dos_findclose(&fileinfo); */ /* apparently required only on OS/2 */
  return(res);
}


static int preparedrive(void) {
  int driveremovable;
  int selecteddrive = 3; /* hardcoded to 'C:' for now */
  int cselecteddrive;
  int ds;
  char buff[128];
  cselecteddrive = 'A' + selecteddrive - 1;
  for (;;) {
    newscreen();
    driveremovable = isdriveremovable(selecteddrive);
    if (driveremovable < 0) {
      char *list[] = { "Create an automatic partition", "Run the FDISK partitioning tool", "Quit to DOS", NULL};
      list[0] = kittengets(0, 3, list[0]);
      list[1] = kittengets(0, 4, list[1]);
      list[2] = kittengets(0, 2, list[2]);
      sprintf(buff, kittengets(3, 0, "ERROR: Drive %c: could not be found. Perhaps your hard disk needs to be"), cselecteddrive);
      video_putstring(4, 2, COLOR_BODY[mono], buff);
      putstringnls(5, 2, COLOR_BODY[mono], 3, 1, "       partitioned first. Please create at least one partition on your");
      putstringnls(6, 2, COLOR_BODY[mono], 3, 2, "       hard disk, so Svarog386 can be installed on it. Note, that");
      sprintf(buff, kittengets(3, 3, "       Svarog386 requires at least %d MiB of available disk space."), SVAROG_DISK_REQ);
      video_putstring(7, 2, COLOR_BODY[mono], buff);
      putstringnls(9, 2, COLOR_BODY[mono], 3, 4, "You can use the FDISK partitioning tool for creating the required partition");
      putstringnls(10, 2, COLOR_BODY[mono], 3, 5, "manually, or you can let the installer partitioning your disk");
      putstringnls(11, 2, COLOR_BODY[mono], 3, 6, "automatically. You can also abort the installation to use any other");
      putstringnls(12, 2, COLOR_BODY[mono], 3, 7, "partition manager of your choice.");
      switch (menuselect(14, -1, 5, list)) {
        case 0:
          system("FDISK /AUTO");
          break;
        case 1:
          video_clear(0x0700, 0);
          video_movecursor(0, 0);
          system("FDISK");
          break;
        case 2:
          return(-1);
      }
      newscreen();
      putstringnls(11, 10, COLOR_BODY[mono], 3, 8, "Your computer will reboot now.");
      putstringnls(12, 10, COLOR_BODY[mono], 0, 5, "Press any key...");
      input_getkey();
      reboot();
      return(-1);
    } else if (driveremovable > 0) {
      sprintf(buff, kittengets(3, 9, "ERROR: Drive %c: is a removable device. Installation aborted."), cselecteddrive);
      video_putstring(9, 2, COLOR_BODY[mono], buff);
      putstringnls(11, 2, COLOR_BODY[mono], 0, 5, "Press any key...");
      return(-1);
    }
    /* if not formatted, propose to format it right away (try to create a directory) */
    sprintf(buff, "%c:\\SVWRTEST.123", cselecteddrive);
    if (mkdir(buff) != 0) {
      char *list[] = { "Proceed with formatting", "Quit to DOS", NULL};
      list[0] = kittengets(0, 6, list[0]);
      list[1] = kittengets(0, 2, list[1]);
      sprintf(buff, kittengets(3, 10, "ERROR: Drive %c: seems to be unformated."), cselecteddrive);
      video_putstring(7, 2, COLOR_BODY[mono], buff);
      putstringnls(8, 2, COLOR_BODY[mono], 3, 11, "       Do you wish to format it?");
      if (menuselect(12, -1, 4, list) != 0) return(-1);
      video_clear(0x0700, 0);
      video_movecursor(0, 0);
      sprintf(buff, "FORMAT %c: /Q /U /Z:seriously /V:SVAROG386", cselecteddrive);
      system(buff);
      continue;
    }
    sprintf(buff, "%c:\\SVWRTEST.123", cselecteddrive);
    rmdir(buff);
    /* check total disk space */
    ds = disksize(selecteddrive);
    if (ds < SVAROG_DISK_REQ) {
      sprintf(buff, kittengets(3, 12, "ERROR: Drive %c: is not big enough!"), cselecteddrive);
      video_putstring(9, 2, COLOR_BODY[mono], buff);
      sprintf(buff, kittengets(3, 13, "      Svarog386 requires a disk of at least %d MiB."), SVAROG_DISK_REQ);
      video_putstring(10, 2, COLOR_BODY[mono], buff);
      putstringnls(12, 2, COLOR_BODY[mono], 0, 5, "Press any key...");
      input_getkey();
      return(-1);
    }
    /* is the disk empty? */
    if (diskempty(selecteddrive) != 0) {
      char *list[] = { "Proceed with formatting", "Quit to DOS", NULL};
      list[0] = kittengets(0, 6, list[0]);
      list[1] = kittengets(0, 2, list[1]);
      sprintf(buff, kittengets(3, 14, "ERROR: Drive %c: is not empty. Svarog386 must be installed on an empty disk."), cselecteddrive);
      video_putstring(7, 2, COLOR_BODY[mono], buff);
      putstringnls(8, 2, COLOR_BODY[mono], 3, 15, "       You can format the disk now, to make it empty. Note however, that");
      putstringnls(9, 2, COLOR_BODY[mono], 3, 16, "       this will ERASE ALL CURRENT DATA on your disk.");
      if (menuselect(12, -1, 4, list) != 0) return(-1);
      video_clear(0x0700, 0);
      video_movecursor(0, 0);
      sprintf(buff, "FORMAT %c: /Q /U /Z:seriously /V:SVAROG386", cselecteddrive);
      system(buff);
      continue;
    } else {
      /* final confirmation */
      char *list[] = { "Install Svarog386", "Quit to DOS", NULL};
      list[0] = kittengets(0, 1, list[0]);
      list[1] = kittengets(0, 2, list[1]);
      sprintf(buff, kittengets(3, 17, "The installation of Svarog386 to %c: is about to begin."), cselecteddrive);
      video_putstring(7, 2, COLOR_BODY[mono], buff);
      if (menuselect(10, -1, 4, list) != 0) return(-1);
      sprintf(buff, "SYS A: %c:", cselecteddrive);
      system(buff);
      sprintf(buff, "%c:\\TEMP", cselecteddrive);
      mkdir(buff);
      return(cselecteddrive);
    }
  }
}


static void bootfilesgen(int targetdrv, char *lang) {
  char buff[128];
  FILE *fd;
  /*** AUTOEXEC.BAT ***/
  sprintf(buff, "%c:\\AUTOEXEC.BAT", targetdrv);
  fd = fopen(buff, "wb");
  if (fd == NULL) return;
  fprintf(fd, "@ECHO OFF\r\n");
  fprintf(fd, "SET TEMP=%c:\\TEMP\r\n", targetdrv);
  fprintf(fd, "SET DOSDIR=%c:\\SYSTEM\\SVAROG.386\r\n", targetdrv);
  fprintf(fd, "SET NLSPATH=%%DOSDIR%%\\NLS\r\n", targetdrv);
  fprintf(fd, "SET LANG=%s\r\n", lang);
  fprintf(fd, "SET DIRCMD=/OGNE/P\r\n");
  fprintf(fd, "SET FDNPKG.CFG=%c:\\SYSTEM\\CFG\\FDNPKG.CFG\r\n");
  fprintf(fd, "SET WATTCP.CFG=%c:\\SYSTEM\\CFG\\WATTCP.CFG\r\n");
  fprintf(fd, "PATH %%DOSDIR%%\\BIN;%%DOSDIR%%\\LINKS\r\n");
  fprintf(fd, "PROMPT $P$G\r\n");
  fprintf(fd, "ALIAS REBOOT=FDAPM COLDBOOT\r\n");
  fprintf(fd, "ALIAS HALT=FDAPM POWEROFF\r\n");
  fprintf(fd, "\r\n\r\n");
  fprintf(fd, "MODE CON CP PREPARE=((991) %c:\\SYSTEM\\SVAROG.386\\CPI\\EGA10.CPX)\r\n");
  fprintf(fd, "MODE CON CP SELECT=991\r\n");
  fprintf(fd, "\r\n");
  fprintf(fd, "SHSUCDX /d:FDCD0001\r\n");
  fclose(fd);
  /*** CONFIG.SYS ***/
  sprintf(buff, "%c:\\CONFIG.SYS", targetdrv);
  fd = fopen(buff, "wb");
  if (fd == NULL) return;
  fprintf(fd, "DOS=UMB,HIGH\r\n");
  fprintf(fd, "FILES=50\r\n");
  fprintf(fd, "DEVICE=%c:\\SYSTEM\\SVAROG.386\\BIN\\HIMEM.EXE\r\n", targetdrv);
  fprintf(fd, "SHELLHIGH=%c:\\SYSTEM\\SVAROG.386\\BIN\\COMMAND.COM /E:512\r\n", targetdrv);
  fprintf(fd, "REM COUNTRY=001,437,%c:\\SYSTEM\\SVAROG.386\r\n", targetdrv);
  fprintf(fd, "DEVICE=%c:\\SYSTEM\\SVAROG.386\\BIN\\CDROM.SYS /D:FDCD0001\r\n", targetdrv);
  fclose(fd);
}


static void installpackages(int targetdrv) {
  char *pkglist[] = {
    "APPEND",
    "ASSIGN",
    "ATTRIB",
    "CHKDSK",
    "CHOICE",
    "COMMAND",
    "COMP",
    "CPIDOS",
    "CTMOUSE",
    "DEBUG",
    "DEFRAG",
    "DELTREE",
    "DEVLOAD",
    "DISKCOMP",
    "DISKCOPY",
    "DISPLAY",
    "DOSFSCK",
    "EDIT",
    "EDLIN",
    "EXE2BIN",
    "FC",
    "FDAPM",
    "FDISK",
    "FDNPKG",
    "FIND",
    "FORMAT",
    "HELP",
    "HIMEMX",
    "KERNEL",
    "KEYB",
    "LABEL",
    "LBACACHE",
    "MEM",
    "MIRROR",
    "MODE",
    "MORE",
    "MOVE",
    "NANSI",
    "NLSFUNC",
    "PRINT",
    "RDISK",
    "RECOVER",
    "REPLACE",
    "SHARE",
    "SHSUCDX",
    "SORT",
    "SWSUBST",
    "TREE",
    "UNDELETE",
    "XCOPY",
    NULL
  };
  int i, pkglistlen;
  char buff[64];
  newscreen();
  /* count how long the pkg list is */
  for (pkglistlen = 0; pkglist[pkglistlen] != NULL; pkglistlen++);
  /* set DOSDIR and friends */
  snprintf(buff, sizeof(buff), "%c:\\SYSTEM\\SVAROG.386", targetdrv);
  setenv("DOSDIR", buff, 1);
  snprintf(buff, sizeof(buff), "%c:\\TEMP", targetdrv);
  setenv("TEMP", buff, 1);
  /* install packages */
  for (i = 0; pkglist[i] != NULL; i++) {
    char buff[128];
    snprintf(buff, sizeof(buff), kittengets(4, 0, "Installing package %d/%d: %s"), i+1, pkglistlen, pkglist[i]);
    strcat(buff, "       ");
    video_putstring(10, 2, COLOR_BODY[mono], buff);
    sprintf(buff, "FDINST INSTALL X:\\BASE\\%s.ZIP > NUL", pkglist[i]);
    system(buff);
  }
}


static void finalreboot(void) {
  newscreen();
  putstringnls(10, 2, COLOR_BODY[mono], 5, 0, "Svarog386 installation is over. Your computer will reboot now.");
  putstringnls(11, 2, COLOR_BODY[mono], 5, 1, "Please remove the installation disk from your drive.");
  putstringnls(13, 2, COLOR_BODY[mono], 0, 5, "Press any key...");
  input_getkey();
  reboot();
}


static void loadcp(char *lang) {
  int cp, egafile;
  char buff[64];
  cp = getnlscp(lang, &egafile);
  if (cp == 437) return;
  video_movecursor(1, 0);
  if (egafile == 1) {
    sprintf(buff, "MODE CON CP PREP=((%d) A:\\EGA.CPX)", cp);
  } else {
    sprintf(buff, "MODE CON CP PREP=((%d) A:\\EGA%d.CPX)", cp, egafile);
  }
  system(buff);
  sprintf(buff, "MODE CON CP SEL=%d", cp);
  system(buff);
  /* below I re-init the video controller - apparently this is required if
   * I want the new glyph symbols to be actually applied */
  {
  union REGS r;
  r.h.ah = 0x0F; /* get current video mode */
  int86(0x10, &r, &r); /* r.h.al contains the current video mode now */
  r.h.ah = 0; /* set video mode (to whatever is set in AL) */
  int86(0x10, &r, &r);
  }
}


int main(void) {
  char lang[4];
  int targetdrv;

  /* init screen and detect mono status */
  mono = video_init();

  for (;;) { /* fake loop, it's here just to break out easily */
    if (selectlang(lang) < 0) break; /* welcome to svarog, select your language */
    setenv("LANG", lang, 1);
    loadcp(lang);
    kittenopen("INSTALL"); /* NLS support */
    /*selectkeyb();*/ /* what keyb layout should we use? */
    if (welcomescreen() != 0) break; /* what svarog386 is, ask whether to run live dos or install */
    targetdrv = preparedrive(); /* what drive should we install to? check avail. space */
    if (targetdrv < 0) break;
    /*askaboutsources();*/ /* IF sources are available, ask if installing with them */
    installpackages(targetdrv);   /* install packages */
    bootfilesgen(targetdrv, lang); /* generate simple boot files */
    /*localcfg();*/ /* show local params (currency, etc), and propose to change them (based on localcfg) */
    /*netcfg();*/ /* basic networking config */
    /* TODO compute a customized FDNPKG file that looks either to local CD or net */
    finalreboot(); /* remove the CD and reboot */
    break;
  }
  kittenclose(); /* close NLS support */
  video_clear(0x0700, 0);
  video_movecursor(0, 0);
  return(0);
}
