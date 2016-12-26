/*
 * SVAROG386 INSTALL
 * COPYRIGHT (C) 2016 MATEUSZ VISTE, ALL RIGHTS RESERVED.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
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

#include "cdrom.h"
#include "input.h"
#include "video.h"

/* keyboard layouts and locales */
#include "keylay.h"
#include "keyoff.h"

/* color scheme (color, mono) */
static unsigned short COLOR_TITLEBAR[2] = {0x7000,0x7000};
static unsigned short COLOR_BODY[2] = {0x1700,0x0700};
static unsigned short COLOR_SELECT[2] = {0x7000,0x7000};
static unsigned short COLOR_SELECTCUR[2] = {0x1F00,0x0700};

/* mono flag */
static int mono = 0;

/* how much disk space does Svarog386 require (in MiB) */
#define SVAROG_DISK_REQ 8

/* menu screens can output only one of these: */
#define MENUNEXT 0
#define MENUPREV -1
#define MENUQUIT -2

/* a convenience 'function' used for debugging */
#define DBG(x) { video_putstringfix(24, 0, 0x4F00u, x, 80); }

struct slocales {
  char lang[4];
  char *keybcode;
  unsigned int codepage;
  int egafile;
  int keybfile;
  int keyboff;
  int keyblen;
  unsigned int keybid;
};


/* reboot the computer */
static void reboot(void) {
  void ((far *bootroutine)()) = (void (far *)()) 0xFFFF0000L;
  int far *rstaddr = (int far *)0x00400072L; /* BIOS boot flag is at 0040:0072 */
  *rstaddr = 0x1234; /* 0x1234 = warm boot, 0 = cold boot */
  (*bootroutine)(); /* jump to the BIOS reboot routine at FFFF:0000 */
}


/* outputs a string to screen with taking care of word wrapping. returns amount of lines. */
static int putstringwrap(int y, int x, unsigned short attr, char *s) {
  int linew, lincount;
  linew = 80;
  if (x >= 0) linew -= (x << 1);

  for (lincount = 1; y+lincount < 25; lincount++) {
    int i, len = linew;
    for (i = 0; i <= linew; i++) {
      if (s[i] == ' ') len = i;
      if (s[i] == '\n') {
        len = i;
        break;
      }
      if (s[i] == 0) {
        len = i;
        break;
      }
    }
    video_putstring(y++, x, attr, s, len);
    s += len;
    if (*s == 0) break;
    s += 1; /* skip the whitespace char */
  }
  return(lincount);
}


/* an NLS wrapper around video_putstring(), also performs line wrapping when
 * needed. returns the amount of lines that were output */
static int putstringnls(int y, int x, unsigned short attr, int nlsmaj, int nlsmin, char *s) {
  s = kittengets(nlsmaj, nlsmin, s);
  return(putstringwrap(y, x, attr, s));
}


static int menuselect(int ypos, int xpos, int height, char **list, int listlen) {
  int i, offset = 0, res = 0, count, width = 0;
  /* count how many positions there is, and check their width */
  for (count = 0; (list[count] != NULL) && (count != listlen); count++) {
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
    }/* else {
      char buf[8];
      snprintf(buf, sizeof(buf), "0x%02X ", key);
      video_putstring(1, 0, COLOR_BODY[mono], buf, -1);
    }*/
  }
}

static void newscreen(int statusbartype) {
  char *msg;
  msg = kittengets(0, 0, "SVAROG386 INSTALLATION");
  video_putcharmulti(0, 0, COLOR_TITLEBAR[mono], ' ', 80, 1);
  video_putstring(0, 40 - (strlen(msg) >> 1), COLOR_TITLEBAR[mono], msg, -1);
  video_clear(COLOR_BODY[mono], 80, -80);
  switch (statusbartype) {
    case 1:
      msg = kittengets(0, 11, "Up/Down = Select entry | Enter = Validate your choice | ESC = Quit to DOS");
      break;
    case 2:
      msg = kittengets(0, 5, "Press any key...");
      break;
    case 3:
      msg = "";
      break;
    default:
      msg = kittengets(0, 10, "Up/Down = Select entry | Enter = Validate your choice | ESC = Previous screen");
      break;
  }
  video_putchar(24, 0, COLOR_TITLEBAR[mono], ' ');
  video_putstringfix(24, 1, COLOR_TITLEBAR[mono], msg, 79);
  video_movecursor(25,0);
}

/* fills a slocales struct accordingly to the value of its keyboff member */
static void kblay2slocal(struct slocales *locales) {
  char *m;
  for (m = kblayouts[locales->keyboff]; *m != 0; m++); /* skip layout name */
  m++;
  /* skip keyb code and copy it to locales.keybcode */
  locales->keybcode = m;
  for (; *m != 0; m++);
  /* */
  locales->codepage = ((unsigned short)m[1] << 8) | m[2];
  locales->egafile = m[3];
  locales->keybfile = m[4];
  locales->keybid = ((unsigned short)m[5] << 8) | m[6];
}

static int selectlang(struct slocales *locales) {
  int choice, x;
  char *msg;
  char *langlist[] = {
    "English",
    "French",
    "Italian",
    "Polish",
    "Russian",
    "Slovene",
    "Turkish",
    NULL
  };

  newscreen(1);
  msg = kittengets(1, 0, "Welcome to Svarog386");
  x = 40 - (strlen(msg) >> 1);
  video_putstring(4, x, COLOR_BODY[mono], msg, -1);
  video_putcharmulti(5, x, COLOR_BODY[mono], '=', strlen(msg), 1);
  putstringnls(8, -1, COLOR_BODY[mono], 1, 1, "Please select your language from the list below:");
  choice = menuselect(11, -1, 9, langlist, -1);
  if (choice < 0) return(MENUPREV);
  /* populate locales with default values */
  memset(locales, 0, sizeof(struct slocales));
  switch (choice) {
    case 1:
      strcpy(locales->lang, "FR");
      locales->keyboff = OFFLOC_FR;
      locales->keyblen = OFFLEN_FR;
      break;
    case 2:
      strcpy(locales->lang, "IT");
      locales->keyboff = OFFLOC_IT;
      locales->keyblen = OFFLEN_IT;
      break;
    case 3:
      strcpy(locales->lang, "PL");
      locales->keyboff = OFFLOC_PL;
      locales->keyblen = OFFLEN_PL;
      break;
    case 4:
      strcpy(locales->lang, "RU");
      locales->keyboff = OFFLOC_RU;
      locales->keyblen = OFFLEN_RU;
      break;
    case 5:
      strcpy(locales->lang, "SI");
      locales->keyboff = OFFLOC_SI;
      locales->keyblen = OFFLEN_SI;
      break;
    case 6:
      strcpy(locales->lang, "TR");
      locales->keyboff = OFFLOC_TR;
      locales->keyblen = OFFLEN_TR;
      break;
    default:
      strcpy(locales->lang, "EN");
      locales->keyboff = 0;
      locales->keyblen = OFFCOUNT;
      break;
  }
  /* populate the slocales struct accordingly to the keyboff member */
  kblay2slocal(locales);
  /* */
  return(MENUNEXT);
}


static int selectkeyb(struct slocales *locales) {
  int menuheight, choice;
  if (locales->keyblen == 1) return(MENUNEXT); /* do not ask for keyboard layout if only one is available for given language */
  newscreen(0);
  putstringnls(5, 1, COLOR_BODY[mono], 1, 5, "Svarog386 supports the keyboard layouts used in different countries. Choose the keyboard layout you want.");
  menuheight = locales->keyblen + 2;
  if (menuheight > 13) menuheight = 13;
  choice = menuselect(10, -1, menuheight, &(kblayouts[locales->keyboff]), locales->keyblen);
  if (choice < 0) return(MENUPREV);
  /* (re)load the keyboard layout & codepage setup */
  locales->keyboff += choice;
  kblay2slocal(locales);
  return(MENUNEXT);
}


/* returns 0 if installation must proceed, non-zero otherwise */
static int welcomescreen(void) {
  int c;
  char *choice[] = {"Install Svarog386 to disk", "Quit to DOS", NULL};
  choice[0] = kittengets(0, 1, choice[0]);
  choice[1] = kittengets(0, 2, choice[1]);
  newscreen(0);
  putstringnls(4, 1, COLOR_BODY[mono], 2, 0, "You are about to install Svarog386: a free, MSDOS-compatible operating system based on the FreeDOS kernel. Svarog386 targets 386+ computers and comes with a variety of third-party applications.\n\nWARNING: If your PC has another operating system installed, this other system might be unable to boot once Svarog386 is installed.");
  c = menuselect(13, -1, 4, choice, -1);
  if (c < 0) return(MENUPREV);
  if (c == 0) return(MENUNEXT);
  return(MENUQUIT);
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
  res = r.x.ax;  /* sectors per cluster */
  res *= r.x.dx; /* dx contains total clusters, bx contains free clusters */
  res *= r.x.cx; /* bytes per sector */
  res >>= 20;    /* convert bytes to MiB */
  return(res);
}


/* returns 0 if disk is empty, non-zero otherwise */
static int diskempty(int drv) {
  unsigned int rc;
  int res;
  char buff[8];
  struct find_t fileinfo;
  snprintf(buff, sizeof(buff), "%c:\\*.*", 'A' + drv - 1);
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
  int choice;
  char buff[1024];
  cselecteddrive = 'A' + selecteddrive - 1;
  for (;;) {
    driveremovable = isdriveremovable(selecteddrive);
    if (driveremovable < 0) {
      char *list[] = { "Create a partition automatically", "Run the FDISK partitioning tool", "Quit to DOS", NULL};
      newscreen(0);
      list[0] = kittengets(0, 3, list[0]);
      list[1] = kittengets(0, 4, list[1]);
      list[2] = kittengets(0, 2, list[2]);
      snprintf(buff, sizeof(buff), kittengets(3, 0, "ERROR: Drive %c: could not be found. Perhaps your hard disk needs to be partitioned first. Please create at least one partition on your hard disk, so Svarog386 can be installed on it. Note, that Svarog386 requires at least %d MiB of available disk space.\n\nYou can use the FDISK partitioning tool for creating the required partition manually, or you can let the installer partitioning your disk automatically. You can also abort the installation to use any other partition manager of your choice."), cselecteddrive, SVAROG_DISK_REQ);
      putstringwrap(4, 1, COLOR_BODY[mono], buff);
      switch (menuselect(14, -1, 5, list, -1)) {
        case 0:
          system("FDISK /AUTO");
          break;
        case 1:
          video_clear(0x0700, 0, 0);
          video_movecursor(0, 0);
          system("FDISK");
          break;
        case 2:
          return(MENUQUIT);
        default:
          return(-1);
      }
      /* write a temporary MBR which only skips the drive (in case BIOS would
       * try to boot off the not-yet-ready C: disk) */
      system("FDISK /AMBR"); /* writes BOOT.MBR into actual MBR */
      newscreen(2);
      putstringnls(10, 10, COLOR_BODY[mono], 3, 1, "Your computer will reboot now.");
      putstringnls(12, 10, COLOR_BODY[mono], 0, 5, "Press any key...");
      input_getkey();
      reboot();
      return(MENUQUIT);
    } else if (driveremovable > 0) {
      newscreen(2);
      snprintf(buff, sizeof(buff), kittengets(3, 2, "ERROR: Drive %c: is a removable device. Installation aborted."), cselecteddrive);
      video_putstring(9, 1, COLOR_BODY[mono], buff, -1);
      putstringnls(11, 2, COLOR_BODY[mono], 0, 5, "Press any key...");
      return(MENUQUIT);
    }
    /* if not formatted, propose to format it right away (try to create a directory) */
    snprintf(buff, sizeof(buff), "%c:\\SVWRTEST.123", cselecteddrive);
    if (mkdir(buff) == 0) {
      rmdir(buff);
    } else {
      char *list[] = { "Proceed with formatting", "Quit to DOS", NULL};
      newscreen(0);
      list[0] = kittengets(0, 6, list[0]);
      list[1] = kittengets(0, 2, list[1]);
      snprintf(buff, sizeof(buff), kittengets(3, 3, "ERROR: Drive %c: seems to be unformated. Do you wish to format it?"), cselecteddrive);
      video_putstring(7, 1, COLOR_BODY[mono], buff, -1);
      choice = menuselect(12, -1, 4, list, -1);
      if (choice < 0) return(MENUPREV);
      if (choice == 1) return(MENUQUIT);
      video_clear(0x0700, 0, 0);
      video_movecursor(0, 0);
      snprintf(buff, sizeof(buff), "FORMAT %c: /Q /U /Z:seriously /V:SVAROG386", cselecteddrive);
      system(buff);
      continue;
    }
    /* check total disk space */
    ds = disksize(selecteddrive);
    if (ds < SVAROG_DISK_REQ) {
      int y = 9;
      newscreen(2);
      snprintf(buff, sizeof(buff), kittengets(3, 4, "ERROR: Drive %c: is not big enough! Svarog386 requires a disk of at least %d MiB."), cselecteddrive);
      y += putstringwrap(y, 1, COLOR_BODY[mono], buff);
      putstringnls(++y, 1, COLOR_BODY[mono], 0, 5, "Press any key...");
      input_getkey();
      return(MENUQUIT);
    }
    /* is the disk empty? */
    newscreen(0);
    if (diskempty(selecteddrive) != 0) {
      char *list[] = { "Proceed with formatting", "Quit to DOS", NULL};
      int y = 6;
      list[0] = kittengets(0, 6, list[0]);
      list[1] = kittengets(0, 2, list[1]);
      snprintf(buff, sizeof(buff), kittengets(3, 5, "ERROR: Drive %c: is not empty. Svarog386 must be installed on an empty disk.\n\nYou can format the disk now, to make it empty. Note however, that this will ERASE ALL CURRENT DATA on your disk."), cselecteddrive);
      y += putstringwrap(y, 1, COLOR_BODY[mono], buff);
      choice = menuselect(++y, -1, 4, list, -1);
      if (choice < 0) return(MENUPREV);
      if (choice == 1) return(MENUQUIT);
      video_clear(0x0700, 0, 0);
      video_movecursor(0, 0);
      snprintf(buff, sizeof(buff), "FORMAT %c: /Q /U /Z:seriously /V:SVAROG386", cselecteddrive);
      system(buff);
      continue;
    } else {
      /* final confirmation */
      char *list[] = { "Install Svarog386", "Quit to DOS", NULL};
      list[0] = kittengets(0, 1, list[0]);
      list[1] = kittengets(0, 2, list[1]);
      snprintf(buff, sizeof(buff), kittengets(3, 6, "The installation of Svarog386 to %c: is about to begin."), cselecteddrive);
      video_putstring(7, -1, COLOR_BODY[mono], buff, -1);
      choice = menuselect(10, -1, 4, list, -1);
      if (choice < 0) return(MENUPREV);
      if (choice == 1) return(MENUQUIT);
      snprintf(buff, sizeof(buff), "SYS A: %c: > NUL", cselecteddrive);
      system(buff);
      system("FDISK /MBR");
      snprintf(buff, sizeof(buff), "%c:\\TEMP", cselecteddrive);
      mkdir(buff);
      return(cselecteddrive);
    }
  }
}


/* copy file src into dst, substituting all characters c1 by c2 */
static void fcopysub(char *dst, char *src, char c1, char c2) {
  FILE *fdd, *fds;
  int buff;
  fds = fopen(src, "rb");
  if (fds == NULL) return;
  fdd = fopen(dst, "wb");
  if (fdd == NULL) {
    fclose(fds);
    return;
  }
  /* */
  for (;;) {
    buff = fgetc(fds);
    if (buff == EOF) break;
    if (buff == c1) buff = c2;
    fprintf(fdd, "%c", buff);
  }
  /* close files and return */
  fclose(fdd);
  fclose(fds);
}


static void bootfilesgen(int targetdrv, struct slocales *locales, int cdromdrv) {
  char buff[128];
  char buff2[16];
  char buff3[16];
  FILE *fd;
  /*** CONFIG.SYS ***/
  snprintf(buff, sizeof(buff), "%c:\\CONFIG.SYS", targetdrv);
  fd = fopen(buff, "wb");
  if (fd == NULL) return;
  fprintf(fd, "DOS=UMB,HIGH\r\n"
              "LASTDRIVE=Z\r\n"
              "FILES=50\r\n");
  fprintf(fd, "DEVICE=%c:\\SYSTEM\\SVAROG.386\\BIN\\HIMEMX.EXE\r\n", targetdrv);
  if (strcmp(locales->lang, "EN") == 0) {
    strcpy(buff, "COMMAND");
  } else {
    snprintf(buff, sizeof(buff), "CMD-%s", locales->lang);
  }
  fprintf(fd, "SHELLHIGH=%c:\\SYSTEM\\SVAROG.386\\BIN\\%s.COM /E:512 /P\r\n", targetdrv, buff);
  fprintf(fd, "REM COUNTRY=001,437,%c:\\SYSTEM\\CONF\\COUNTRY.SYS\r\n", targetdrv);
  fprintf(fd, "DEVICE=%c:\\SYSTEM\\DRIVERS\\UDVD2\\UDVD2.SYS /D:SVCD0001 /H\r\n", targetdrv);
  fclose(fd);
  /*** AUTOEXEC.BAT ***/
  snprintf(buff, sizeof(buff), "%c:\\AUTOEXEC.BAT", targetdrv);
  fd = fopen(buff, "wb");
  if (fd == NULL) return;
  fprintf(fd, "@ECHO OFF\r\n");
  fprintf(fd, "SET TEMP=%c:\\TEMP\r\n", targetdrv);
  fprintf(fd, "SET DOSDIR=%c:\\SYSTEM\\SVAROG.386\r\n", targetdrv);
  fprintf(fd, "SET NLSPATH=%%DOSDIR%%\\NLS\r\n");
  fprintf(fd, "SET LANG=%s\r\n", locales->lang);
  fprintf(fd, "SET DIRCMD=/OGNE/P/4\r\n");
  fprintf(fd, "SET FDNPKG.CFG=%c:\\SYSTEM\\CFG\\FDNPKG.CFG\r\n", targetdrv);
  fprintf(fd, "SET WATTCP.CFG=%c:\\SYSTEM\\CFG\\WATTCP.CFG\r\n", targetdrv);
  fprintf(fd, "PATH %%DOSDIR%%\\BIN;%c:\\SYSTEM\\LINKS\r\n", targetdrv);
  fprintf(fd, "PROMPT $P$G\r\n");
  fprintf(fd, "ALIAS REBOOT=FDAPM COLDBOOT\r\n");
  fprintf(fd, "ALIAS HALT=FDAPM POWEROFF\r\n");
  fprintf(fd, "FDAPM APMDOS\r\n");
  fprintf(fd, "\r\n");
  if (locales->egafile > 0) {
    fprintf(fd, "DISPLAY CON=(EGA,,1)\r\n");
    if (locales->egafile == 1) {
      fprintf(fd, "MODE CON CP PREPARE=((%u) %c:\\SYSTEM\\SVAROG.386\\CPI\\EGA.CPX)\r\n", locales->codepage, targetdrv);
    } else {
      fprintf(fd, "MODE CON CP PREPARE=((%u) %c:\\SYSTEM\\SVAROG.386\\CPI\\EGA%d.CPX)\r\n", locales->codepage, targetdrv, locales->egafile);
    }
    fprintf(fd, "MODE CON CP SELECT=%u\r\n", locales->codepage);
  }
  if (locales->keybfile > 0) {
    if (locales->keybfile == 1) {
      snprintf(buff2, sizeof(buff2), "KEYBOARD.SYS");
    } else {
      snprintf(buff2, sizeof(buff2), "KEYBRD%d.SYS", locales->keybfile);
    }
    if (locales->keybid == 0) {
      buff3[0] = 0;
    } else {
      snprintf(buff3, sizeof(buff3), " /ID:%d", locales->keybid);
    }
    fprintf(fd, "KEYB %s,%d,%c:\\SYSTEM\\SVAROG.386\\BIN\\%s%s\r\n", locales->keybcode, locales->codepage, targetdrv, buff2, buff3);
    fprintf(fd, "\r\n");
  }
  fprintf(fd, "SHSUCDX /d:SVCD0001\r\n");
  fprintf(fd, "\r\n");
  fprintf(fd, "REM Uncomment the line below for automatic mouse support\r\n");
  fprintf(fd, "REM CTMOUSE\r\n");
  fprintf(fd, "\r\n");
  fprintf(fd, "ECHO.\r\n");
  fprintf(fd, "ECHO %s\r\n", kittengets(6, 0, "Welcome to Svarog386! Type 'HELP' if you need help."));
  fclose(fd);
  /*** CREATE DIRECTORY FOR OTHER CONFIGURATION FILES ***/
  snprintf(buff, sizeof(buff), "%c:\\SYSTEM\\CFG", targetdrv);
  mkdir(buff);
  /*** FDNPKG.CFG ***/
  snprintf(buff, sizeof(buff), "%c:\\SYSTEM\\CFG\\FDNPKG.CFG", targetdrv);
  fcopysub(buff, "A:\\DAT\\FDNPKG.CFG", '$', cdromdrv);
  /*** COUNTRY.SYS ***/
  /*** PICOTCP ***/
  /*** WATTCP ***/
}


static void installpackages(int targetdrv, int cdromdrv) {
  char *pkglist[] = {
    "A:\\UDVD2", /* this one's not part of CORE, hence it's stored right on the floppy */
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
    "KEYB_LAY",
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
  newscreen(3);
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
    video_putstring(10, 1, COLOR_BODY[mono], buff, -1);
    if (pkglist[i][1] == ':') {
      snprintf(buff, sizeof(buff), "FDINST INSTALL %s.ZIP > NUL", pkglist[i]);
    } else {
      snprintf(buff, sizeof(buff), "FDINST INSTALL %c:\\CORE\\%s.ZIP > NUL", cdromdrv, pkglist[i]);
    }
    system(buff);
  }
}


static void finalreboot(void) {
  int y = 9;
  newscreen(2);
  y += putstringnls(y, 1, COLOR_BODY[mono], 5, 0, "Svarog386 installation is over. Your computer will reboot now.\nPlease remove the installation disk from your drive.");
  putstringnls(++y, 1, COLOR_BODY[mono], 0, 5, "Press any key...");
  input_getkey();
  reboot();
}


static void loadcp(struct slocales *locales) {
  char buff[64];
  if (locales->codepage == 437) return;
  video_movecursor(1, 0);
  if (locales->egafile == 1) {
    snprintf(buff, sizeof(buff), "MODE CON CP PREP=((%u) A:\\EGA.CPX) > NUL", locales->codepage);
  } else {
    snprintf(buff, sizeof(buff), "MODE CON CP PREP=((%u) A:\\EGA%d.CPX) > NUL", locales->codepage, locales->egafile);
  }
  system(buff);
  snprintf(buff, sizeof(buff), "MODE CON CP SEL=%u > NUL", locales->codepage);
  system(buff);
  /* below I re-init the video controller - apparently this is required if
   * I want the new glyph symbols to be actually applied, at least some
   * (broken?) BIOSes, like VBox, apply glyphs only at next video mode change */
  {
  union REGS r;
  r.h.ah = 0x0F; /* get current video mode */
  int86(0x10, &r, &r); /* r.h.al contains the current video mode now */
  r.h.al |= 128; /* set the high bit of AL to instruct BIOS not to flush VRAM's content (EGA+) */
  r.h.ah = 0; /* re-set video mode (to whatever is set in AL) */
  int86(0x10, &r, &r);
  }
}

/* checks CD drive drv for the presence of the Svarog386 install CD
 * returns 0 if found, non-zero otherwise */
static int checkcd(char drv) {
  FILE *fd;
  char fname[32];
  snprintf(fname, sizeof(fname), "%c:\\CORE\\MEM.ZIP", drv);
  fd = fopen(fname, "rb");
  if (fd == NULL) return(-1);
  fclose(fd);
  return(0);
}


int main(void) {
  struct slocales locales;
  int targetdrv;
  int cdromdrv;
  int action;

  /* find where the cdrom drive is */
  cdromdrv = cdrom_findfirst();
  if (cdromdrv < 0) {
    printf("ERROR: CD-ROM DRIVE NOT FOUND\r\n");
    return(1);
  }
  cdromdrv += 'A'; /* convert the cdrom 'id' (A=0) to an actual drive letter */
  if (checkcd(cdromdrv) != 0) {
    printf("ERROR: SVAROG386 INSTALLATION CD NOT FOUND IN THE DRIVE.\r\n");
    return(1);
  }

  /* init screen and detect mono status */
  mono = video_init();

  kittenopen("INSTALL"); /* load initial NLS support */

 SelectLang:
  action = selectlang(&locales); /* welcome to svarog, select your language */
  if (action != MENUNEXT) goto Quit;
  setenv("LANG", locales.lang, 1);
  loadcp(&locales);
  kittenclose(); /* reload NLS with new language */
  kittenopen("INSTALL"); /* NLS support */
  action = selectkeyb(&locales);  /* what keyb layout should we use? */
  if (action == MENUQUIT) goto Quit;
  if (action == MENUPREV) goto SelectLang;

 WelcomeScreen:
  action = welcomescreen(); /* what svarog386 is, ask whether to run live dos or install */
  if (action == MENUQUIT) goto Quit;
  if (action == MENUPREV) goto SelectLang;
  targetdrv = preparedrive(); /* what drive should we install to? check avail. space */
  if (targetdrv == MENUQUIT) goto Quit;
  if (targetdrv == MENUPREV) goto WelcomeScreen;
  /*askaboutsources();*/ /* IF sources are available, ask if installing with them */
  installpackages(targetdrv, cdromdrv);    /* install packages */
  bootfilesgen(targetdrv, &locales, cdromdrv); /* generate boot files and other configurations */
  /*localcfg();*/ /* show local params (currency, etc), and propose to change them (based on localcfg) */
  /*netcfg();*/ /* basic networking config */
  finalreboot(); /* remove the CD and reboot */

 Quit:
  kittenclose(); /* close NLS support */
  video_clear(0x0700, 0, 0);
  video_movecursor(0, 0);
  return(0);
}
