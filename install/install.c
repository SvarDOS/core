/*
 * SVAROG386 INSTALL
 * COPYRIGHT (C) 2016 MATEUSZ VISTE
 */

#include <dos.h>
#include <direct.h>  /* mkdir() */
#include <stdio.h>   /* printf() and friends */
#include <stdlib.h>  /* system() */
#include <string.h>  /* memcpy() */
#include <unistd.h>
#include "input.h"
#include "video.h"

/* color scheme (color, mono) */
static unsigned short COLOR_TITLEBAR[2] = {0x7000,0x7000};
static unsigned short COLOR_BODY[2] = {0x1700,0x0700};
static unsigned short COLOR_SELECT[2] = {0x7000,0x7000};
static unsigned short COLOR_SELECTCUR[2] = {0x1F00,0x0700};

/* mono flag */
static int mono = 0;


/* reboot the computer */
static void reboot(void) {
  void ((far *bootroutine)()) = (void (far *)()) 0xFFFF0000L;
  int far *rstaddr = (int far *)0x00400072L; /* BIOS boot flag is at 0040:0072 */
  *rstaddr = 0x1234; /* 0x1234 = warm boot, 0 = cold boot */
  (*bootroutine)(); /* jump to the BIOS reboot routine at FFFF:0000 */
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
  for (x = 0; x < 80; x++) video_putchar(0, x, COLOR_TITLEBAR[mono], ' ');
  video_clear(COLOR_BODY[mono], 80);
  video_putstring(0, 29, COLOR_TITLEBAR[mono], "SVAROG386 INSTALLATION");
}


static int selectlang(char *lang) {
  int choice;
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
  video_putstring(3, 30, COLOR_BODY[mono], "Welcome to Svarog386");
  video_putstring(4, 30, COLOR_BODY[mono], "====================");
  video_putstring(6, 2, COLOR_BODY[mono], "Svarog386 is an operating system based on the FreeDOS kernel. It targets");
  video_putstring(7, 2, COLOR_BODY[mono], "386+ computers and comes with a variety of third-party applications. Before");
  video_putstring(8, 2, COLOR_BODY[mono], "we get to serious business, please select your preferred language from the");
  video_putstring(9, 2, COLOR_BODY[mono], "list below, and press the ENTER key:");
  choice = menuselect(11, -1, 12, langlist);
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
  newscreen();
  video_putstring(4, 1, COLOR_BODY[mono], "You are about to install Svarog386, a free, MSDOS-compatible operating system");
  video_putstring(5, 1, COLOR_BODY[mono], "based on the FreeDOS kernel.");
  video_putstring(7, 1, COLOR_BODY[mono], "WARNING: If your PC has another operating system installed, this other system");
  video_putstring(8, 1, COLOR_BODY[mono], "         might be unable to boot once Svarog386 is installed.");
  return(menuselect(14, -1, 4, choice));
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


/* returns total disk space of drive drv (in MiB, max 2048), or -1 if drive invalid
 * also sets emptyflag to 1 if drive is empty, or to 0 otherwise */
static int disksize(int drv, int *emptyflag) {
  long res;
  union REGS r;
  r.h.ah = 0x36; /* DOS 2+ get free disk space */
  r.h.dl = drv;
  int86(0x21, &r, &r);
  if (r.x.ax == 0xffffu) return(-1); /* AX set to FFFFh if drive invalid */
  res = r.x.ax;
  res *= r.x.bx;
  res *= r.x.cx;
  res >>= 20; /* bytes to MiB */
  if (r.x.dx != r.x.bx) { /* DX is total number of clusters, while BX is only free clusters */
    *emptyflag = 0;
  } else {
    *emptyflag = 1;
  }
  return(res);
}


static int preparedrive(void) {
  int driveremovable;
  int selecteddrive = 3; /* hardcoded to 'C:' */
  int ds, emptydriveflag;
  for (;;) {
    newscreen();
    driveremovable = isdriveremovable(selecteddrive);
    if (driveremovable < 0) {
      char *list[] = { "Create an automatic partition", "Run the FDISK partitioning tool", "Quit to DOS", NULL};
      video_putstring(4, 2, COLOR_BODY[mono], "ERROR: Drive C: could not be found. Perhaps your hard disk needs to be");
      video_putstring(5, 2, COLOR_BODY[mono], "       partitioned first. Please create at least one partition on your");
      video_putstring(6, 2, COLOR_BODY[mono], "       hard disk, so Svarog386 can be installed on it. Note, that");
      video_putstring(7, 2, COLOR_BODY[mono], "       Svarog386 requires at least 16 MiB of available disk space.");
      video_putstring(9, 2, COLOR_BODY[mono], "You can use the FDISK partitioning tool for creating the required partition");
      video_putstring(10, 2, COLOR_BODY[mono], "manually, or you can let the installer partitioning your disk");
      video_putstring(11, 2, COLOR_BODY[mono], "automatically. You can also abort the installation to use any other");
      video_putstring(12, 2, COLOR_BODY[mono], "partition manager of your choice.");
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
      video_putstring(12, 10, COLOR_BODY[mono], "Your computer will reboot now. Press any key.");
      input_getkey();
      reboot();
      return(-1);
    } else if (driveremovable > 0) {
      video_putstring(8, 2, COLOR_BODY[mono], "ERROR: Drive C: appears to be a removable device.");
      video_putstring(10, 2, COLOR_BODY[mono], "Installation aborted. Press any key.");
      return(-1);
    }
    /* if not formatted, propose to format it right away (try to create a directory) */
    if (mkdir("C:\\SVWRTEST.123") != 0) {
      char *list[] = { "Proceed with formatting", "Quit to DOS", NULL};
      video_putstring(7, 2, COLOR_BODY[mono], "ERROR: Drive C: seems to be unformated.");
      video_putstring(8, 2, COLOR_BODY[mono], "       Do you wish to format it?");
      if (menuselect(12, -1, 4, list) != 0) return(-1);
      video_clear(0x0700, 0);
      video_movecursor(0, 0);
      system("FORMAT C: /Q /U /V:SVAROG");
      continue;
    }
    rmdir("C:\\SVWRTEST.123");
    /* check total disk space */
    ds = disksize(selecteddrive, &emptydriveflag);
    if (ds < 16) {
      video_putstring(9, 2, COLOR_BODY[mono], "ERROR: Drive C: is not big enough!");
      video_putstring(10, 2, COLOR_BODY[mono], "      Svarog386 requires a disk of at least 16 MiB.");
      video_putstring(12, 2, COLOR_BODY[mono], "Press any key to return to DOS.");
      input_getkey();
      return(-1);
    }
    /* is the disk empty? */
    if (emptydriveflag != 0) {
      char *list[] = { "Proceed with formatting", "Quit to DOS", NULL};
      video_putstring(7, 2, COLOR_BODY[mono], "ERROR: Drive C: is not empty. Svarog386 must be installed on an empty disk.");
      video_putstring(8, 2, COLOR_BODY[mono], "       You can format the disk now, to make it empty. Note however, that");
      video_putstring(9, 2, COLOR_BODY[mono], "       this will ERASE ALL CURRENT DATA on your disk.");
      if (menuselect(12, -1, 4, list) != 0) return(-1);
      video_clear(0x0700, 0);
      video_movecursor(0, 0);
      system("FORMAT /Q C:");
      continue;
    } else {
      /* final confirmation */
      char *list[] = { "Install Svarog386", "Quit to DOS", NULL};
      video_putstring(8, 2, COLOR_BODY[mono], "The installation of Svarog386 to your C: disk is about to begin.");
      if (menuselect(10, -1, 4, list) != 0) return(-1);
      system("SYS A: C:");
      mkdir("C:\\TEMP");
      return(0);
    }
  }
}


static void finalreboot(void) {
  newscreen();
  video_putstring(10, 2, COLOR_BODY[mono], "Svarog386 installation is over. Please remove the");
  video_putstring(10, 2, COLOR_BODY[mono], "installation diskette and/or CD from the drive.");
  video_putstring(13, 2, COLOR_BODY[mono], "Press any key to reboot...");
  input_getkey();
  reboot();
}


static void bootfilesgen(int targetdrv, char *lang) {
  char drv = 'A' + targetdrv - 1;
  char buff[128];
  FILE *fd;
  /*** AUTOEXEC.BAT ***/
  sprintf(buff, "%c:\\AUTOEXEC.BAT", drv);
  fd = fopen(buff, "wb");
  if (fd == NULL) return;
  fprintf(fd, "@ECHO OFF\r\n");
  fprintf(fd, "SET TEMP=%c:\\TEMP\r\n", drv);
  fprintf(fd, "SET DOSDIR=%c:\\SYSTEM\\SVAROG.386\r\n", drv);
  fprintf(fd, "SET NLSPATH=%%DOSDIR%%\\NLS\r\n", drv);
  fprintf(fd, "SET LANG=%s\r\n", lang);
  fprintf(fd, "SET DIRCMD=/OGNE/P\r\n");
  fprintf(fd, "SET FDNPKG.CFG=%c:\\SYSTEM\\CFG\\FDNPKG.CFG\r\n");
  fprintf(fd, "SET WATTCP.CFG=%c:\\SYSTEM\\CFG\\WATTCP.CFG\r\n");
  fprintf(fd, "PATH %%DOSDIR%%\\BIN;%%DOSDIR%%\\LINKS\r\n");
  fprintf(fd, "PROMPT $P$G\r\n");
  fprintf(fd, "ALIAS REBOOT=FDAPM COLDBOOT\r\n");
  fprintf(fd, "ALIAS HALT=FDAPM POWEROFF\r\n");
  fprintf(fd, "\r\n\r\n");
  fprintf(fd, "MODE CON CP PREPARE=((991) %c:\\SYSTEM\\SVAROG.386\\CPI\\EGA10.CPX\r\n");
  fprintf(fd, "MODE CON CP SELECT=991\r\n");
  fprintf(fd, "\r\n");
  fprintf(fd, "SHSUCDX /d:FDCD0001\r\n");
  fclose(fd);
  /*** CONFIG.SYS ***/
  sprintf(buff, "%c:\\CONFIG.SYS", drv);
  fd = fopen(buff, "wb");
  if (fd == NULL) return;
  fprintf(fd, "DOS=UMB,HIGH\r\n");
  fprintf(fd, "FILES=50\r\n");
  fprintf(fd, "DEVICE=%c:\\SYSTEM\\SVAROG.386\\BIN\\HIMEM.EXE\r\n", drv);
  fprintf(fd, "SHELLHIGH=%c:\\SYSTEM\\SVAROG.386\\BIN\\COMMAND.COM /E:512", drv);
  fprintf(fd, "REM COUNTRY=001,437,%c:\\SYSTEM\\SVAROG.386\r\n", drv);
  fprintf(fd, "DEVICE=%c:\\SYSTEM\\SVAROG.386\\BIN\\CDROM.SYS /D:FDCD0001\r\n", drv);
  fclose(fd);
}


static void installpackages(void) {
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
  newscreen();
  /* count how long the pkg list is */
  for (pkglistlen = 0; pkglist[pkglistlen] != NULL; pkglistlen++);
  /* install packages */
  for (i = 0; pkglist[i] != NULL; i++) {
    char buff[16];
    sprintf(buff, "Installing package %d/%d: %s", i, pkglistlen, pkglist[i]);
    video_putstring(10, 2, COLOR_BODY[mono], buff);
    sprintf(buff, "FDNPKG INSTALL %s > NULL");
    system(buff);
  }
}


int main(void) {
  char lang[4];
  int targetdrv;

  /* init screen and detect mono status */
  mono = video_init();

  for (;;) { /* fake loop, it's here just to break out easily */
    if (selectlang(lang) < 0) break; /* welcome to svarog, select your language */
    /*selectkeyb();*/ /* what keyb layout should we use? */
    if (welcomescreen() != 0) break; /* what svarog386 is, ask whether to run live dos or install */
    targetdrv = preparedrive(); /* what drive should we install to? check avail. space */
    if (targetdrv < 0) break;
    /*askaboutsources();*/ /* IF sources are available, ask if installing with them */
    installpackages();   /* install packages */
    bootfilesgen(targetdrv, lang); /* generate simple boot files */
    /*localcfg();*/ /* show local params (currency, etc), and propose to change them (based on localcfg) */
    /*netcfg();*/ /* basic networking config? */
    finalreboot(); /* remove the CD and reboot */
    break;
  }
  video_clear(0x0700, 0);
  video_movecursor(0, 0);
  return(0);
}
