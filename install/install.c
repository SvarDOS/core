/*
 * SVARDOS INSTALL PROGRAM
 * PUBLISHED UNDER THE TERMS OF THE MIT LICENSE
 *
 * COPYRIGHT (C) 2016-2021 MATEUSZ VISTE, ALL RIGHTS RESERVED.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * http://svardos.osdn.io
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

/* how much disk space does SvarDOS require (in MiB) */
#define SVARDOS_DISK_REQ 8

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
  msg = kittengets(0, 0, "SVARDOS INSTALLATION");
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
    "German",
    "Italian",
    "Polish",
    "Russian",
    "Slovene",
    "Swedish",
    "Turkish",
    NULL
  };

  newscreen(1);
  msg = kittengets(1, 0, "Welcome to SvarDOS");
  x = 40 - (strlen(msg) >> 1);
  video_putstring(4, x, COLOR_BODY[mono], msg, -1);
  video_putcharmulti(5, x, COLOR_BODY[mono], '=', strlen(msg), 1);
  putstringnls(8, -1, COLOR_BODY[mono], 1, 1, "Please select your language from the list below:");
  choice = menuselect(11, -1, 11, langlist, -1);
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
      strcpy(locales->lang, "DE");
      locales->keyboff = OFFLOC_DE;
      locales->keyblen = OFFLEN_DE;
      break;
    case 3:
      strcpy(locales->lang, "IT");
      locales->keyboff = OFFLOC_IT;
      locales->keyblen = OFFLEN_IT;
      break;
    case 4:
      strcpy(locales->lang, "PL");
      locales->keyboff = OFFLOC_PL;
      locales->keyblen = OFFLEN_PL;
      break;
    case 5:
      strcpy(locales->lang, "RU");
      locales->keyboff = OFFLOC_RU;
      locales->keyblen = OFFLEN_RU;
      break;
    case 6:
      strcpy(locales->lang, "SI");
      locales->keyboff = OFFLOC_SI;
      locales->keyblen = OFFLEN_SI;
      break;
    case 7:
      strcpy(locales->lang, "SV");
      locales->keyboff = OFFLOC_SV;
      locales->keyblen = OFFLEN_SV;
      break;
    case 8:
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
  putstringnls(5, 1, COLOR_BODY[mono], 1, 5, "SvarDOS supports different keyboard layouts. Choose the keyboard layout that you want.");
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
  char *choice[] = {"Install SvarDOS to disk", "Quit to DOS", NULL};
  choice[0] = kittengets(0, 1, choice[0]);
  choice[1] = kittengets(0, 2, choice[1]);
  newscreen(0);
  putstringnls(4, 1, COLOR_BODY[mono], 2, 0, "You are about to install SvarDOS: a free, MSDOS-compatible operating system based on the FreeDOS kernel. SvarDOS targets 386+ computers and comes with a variety of third-party applications.\n\nWARNING: If your PC has another operating system installed, this other system might be unable to boot once SvarDOS is installed.");
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
      snprintf(buff, sizeof(buff), kittengets(3, 0, "ERROR: Drive %c: could not be found. Perhaps your hard disk needs to be partitioned first. Please create at least one partition on your hard disk, so SvarDOS can be installed on it. Note, that SvarDOS requires at least %d MiB of available disk space.\n\nYou can use the FDISK partitioning tool for creating the required partition manually, or you can let the installer partitioning your disk automatically. You can also abort the installation to use any other partition manager of your choice."), cselecteddrive, SVARDOS_DISK_REQ);
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
      snprintf(buff, sizeof(buff), "FORMAT %c: /Q /U /Z:seriously /V:SVARDOS", cselecteddrive);
      system(buff);
      continue;
    }
    /* check total disk space */
    ds = disksize(selecteddrive);
    if (ds < SVARDOS_DISK_REQ) {
      int y = 9;
      newscreen(2);
      snprintf(buff, sizeof(buff), kittengets(3, 4, "ERROR: Drive %c: is not big enough! SvarDOS requires a disk of at least %d MiB."), cselecteddrive);
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
      snprintf(buff, sizeof(buff), kittengets(3, 5, "ERROR: Drive %c: is not empty. SvarDOS must be installed on an empty disk.\n\nYou can format the disk now, to make it empty. Note however, that this will ERASE ALL CURRENT DATA on your disk."), cselecteddrive);
      y += putstringwrap(y, 1, COLOR_BODY[mono], buff);
      choice = menuselect(++y, -1, 4, list, -1);
      if (choice < 0) return(MENUPREV);
      if (choice == 1) return(MENUQUIT);
      video_clear(0x0700, 0, 0);
      video_movecursor(0, 0);
      snprintf(buff, sizeof(buff), "FORMAT %c: /Q /U /Z:seriously /V:SVARDOS", cselecteddrive);
      system(buff);
      continue;
    } else {
      /* final confirmation */
      char *list[] = { "Install SvarDOS", "Quit to DOS", NULL};
      list[0] = kittengets(0, 1, list[0]);
      list[1] = kittengets(0, 2, list[1]);
      snprintf(buff, sizeof(buff), kittengets(3, 6, "The installation of SvarDOS to %c: is about to begin."), cselecteddrive);
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
static void fcopysub(const char *dst, const char *src, char c1, char c2) {
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


static void bootfilesgen(char targetdrv, const struct slocales *locales, char cdromdrv) {
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
  fprintf(fd, "DEVICE=%c:\\SYSTEM\\SVARDOS\\BIN\\HIMEMX.EXE\r\n", targetdrv);
  if (strcmp(locales->lang, "EN") == 0) {
    strcpy(buff, "COMMAND");
  } else {
    snprintf(buff, sizeof(buff), "CMD-%s", locales->lang);
  }
  fprintf(fd, "SHELLHIGH=%c:\\SYSTEM\\SVARDOS\\BIN\\%s.COM /E:512 /P\r\n", targetdrv, buff);
  fprintf(fd, "REM COUNTRY=001,437,%c:\\SYSTEM\\CONF\\COUNTRY.SYS\r\n", targetdrv);
  fprintf(fd, "DEVICE=%c:\\SYSTEM\\DRIVERS\\UDVD2\\UDVD2.SYS /D:SVCD0001 /H\r\n", targetdrv);
  fclose(fd);
  /*** AUTOEXEC.BAT ***/
  snprintf(buff, sizeof(buff), "%c:\\AUTOEXEC.BAT", targetdrv);
  fd = fopen(buff, "wb");
  if (fd == NULL) return;
  fprintf(fd, "@ECHO OFF\r\n");
  fprintf(fd, "SET TEMP=%c:\\TEMP\r\n", targetdrv);
  fprintf(fd, "SET DOSDIR=%c:\\SYSTEM\\SVARDOS\r\n", targetdrv);
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
      fprintf(fd, "MODE CON CP PREPARE=((%u) %c:\\SYSTEM\\SVARDOS\\CPI\\EGA.CPX)\r\n", locales->codepage, targetdrv);
    } else {
      fprintf(fd, "MODE CON CP PREPARE=((%u) %c:\\SYSTEM\\SVARDOS\\CPI\\EGA%d.CPX)\r\n", locales->codepage, targetdrv, locales->egafile);
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
    fprintf(fd, "KEYB %s,%d,%c:\\SYSTEM\\SVARDOS\\BIN\\%s%s\r\n", locales->keybcode, locales->codepage, targetdrv, buff2, buff3);
    fprintf(fd, "\r\n");
  }
  fprintf(fd, "SHSUCDX /d:SVCD0001\r\n");
  fprintf(fd, "\r\n");
  fprintf(fd, "REM Uncomment the line below for automatic mouse support\r\n");
  fprintf(fd, "REM CTMOUSE\r\n");
  fprintf(fd, "\r\n");
  fprintf(fd, "ECHO.\r\n");
  fprintf(fd, "ECHO %s\r\n", kittengets(6, 0, "Welcome to SvarDOS! Type 'HELP' if you need help."));
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


static int installpackages(char targetdrv, char cdromdrv) {
  char pkglist[512];
  int i, pkglistlen;
  size_t pkglistflen;
  char buff[64];
  FILE *fd;
  char *pkgptr;
  newscreen(3);
  /* load pkg list */
  fd = fopen("install.lst", "rb");
  if (fd == NULL) {
    video_putstring(10, 30, COLOR_BODY[mono], "ERROR: INSTALL.LST NOT FOUND", -1);
    input_getkey();
    return(-1);
  }
  pkglistflen = fread(pkglist, 1, sizeof(pkglist), fd);
  fclose(fd);
  if (pkglistflen == sizeof(pkglist)) {
    video_putstring(10, 30, COLOR_BODY[mono], "ERROR: INSTALL.LST TOO LARGE", -1);
    input_getkey();
    return(-1);
  }
  pkglist[pkglistflen] = 0xff; /* mark the end of list */
  /* replace all \r and \n chars by 0 bytes, and count the number of packages */
  pkglistlen = 0;
  for (i = 0; i < pkglistflen; i++) {
    switch (pkglist[i]) {
      case '\n':
        pkglistlen++;
        /* FALLTHRU */
      case '\r':
        pkglist[i] = 0;
        break;
    }
  }
  /* set DOSDIR */
  snprintf(buff, sizeof(buff), "%c:\\SYSTEM\\SVARDOS", targetdrv);
  setenv("DOSDIR", buff, 1);
  snprintf(buff, sizeof(buff), "%c:\\TEMP", targetdrv);
  setenv("TEMP", buff, 1);
  /* install packages */
  pkgptr = pkglist;
  for (i = 0;; i++) {
    char buff[64];
    /* move forward to nearest entry or end of list */
    while (*pkgptr == 0) pkgptr++;
    if (*pkgptr == 0xff) break;
    /* install the package */
    snprintf(buff, sizeof(buff), kittengets(4, 0, "Installing package %d/%d: %s"), i+1, pkglistlen, pkgptr);
    strcat(buff, "       ");
    video_putstringfix(10, 1, COLOR_BODY[mono], buff, sizeof(buff));
    snprintf(buff, sizeof(buff), "FDINST INSTALL %c:\\%s.ZIP > NUL", cdromdrv, pkgptr);
    if (system(buff) != 0) {
      video_putstring(10, 30, COLOR_BODY[mono], "ERROR: PKG INSTALL FAILED", -1);
      input_getkey();
      return(-1);
    }
    /* jump to next entry or end of list */
    while ((*pkgptr != 0) && (*pkgptr != 0xff)) pkgptr++;
    if (*pkgptr == 0xff) break;
  }
  return(0);
}


static void finalreboot(void) {
  int y = 9;
  newscreen(2);
  y += putstringnls(y, 1, COLOR_BODY[mono], 5, 0, "SvarDOS installation is over. Your computer will reboot now.\nPlease remove the installation disk from your drive.");
  putstringnls(++y, 1, COLOR_BODY[mono], 0, 5, "Press any key...");
  input_getkey();
  reboot();
}


static void loadcp(const struct slocales *locales) {
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

/* checks that drive drv contains SvarDOS packages
 * returns 0 if found, non-zero otherwise */
static int checkinstsrc(char drv) {
  FILE *fd;
  char fname[16];
  snprintf(fname, sizeof(fname), "%c:\\ATTRIB.ZIP", drv);
  fd = fopen(fname, "rb");
  if (fd == NULL) return(-1);
  fclose(fd);
  return(0);
}


int main(void) {
  struct slocales locales;
  int targetdrv;
  int sourcedrv;
  int action;

  /* am I running in install-from-floppy mode? */
  if (checkinstsrc('A') == 0) {
    sourcedrv = 'A';
  } else { /* otherwise find where the cdrom drive is */
    sourcedrv = cdrom_findfirst();
    if (sourcedrv < 0) {
      printf("ERROR: CD-ROM DRIVE NOT FOUND\r\n");
      return(1);
    }
    sourcedrv += 'A'; /* convert the source drive 'id' (A=0) to an actual drive letter */
    if (checkinstsrc(sourcedrv) != 0) {
      printf("ERROR: SVARDOS INSTALLATION CD NOT FOUND IN THE DRIVE.\r\n");
      return(1);
    }
  }

  /* init screen and detect mono status */
  mono = video_init();

  kittenopen("INSTALL"); /* load initial NLS support */

 SelectLang:
  action = selectlang(&locales); /* welcome to svardos, select your language */
  if (action != MENUNEXT) goto Quit;
  setenv("LANG", locales.lang, 1);
  loadcp(&locales);
  kittenclose(); /* reload NLS with new language */
  kittenopen("INSTALL"); /* NLS support */
  action = selectkeyb(&locales);  /* what keyb layout should we use? */
  if (action == MENUQUIT) goto Quit;
  if (action == MENUPREV) goto SelectLang;

 WelcomeScreen:
  action = welcomescreen(); /* what svardos is, ask whether to run live dos or install */
  if (action == MENUQUIT) goto Quit;
  if (action == MENUPREV) goto SelectLang;
  targetdrv = preparedrive(); /* what drive should we install to? check avail. space */
  if (targetdrv == MENUQUIT) goto Quit;
  if (targetdrv == MENUPREV) goto WelcomeScreen;
  /*askaboutsources();*/ /* IF sources are available, ask if installing with them */
  if (installpackages(targetdrv, sourcedrv) != 0) goto Quit;    /* install packages */
  bootfilesgen(targetdrv, &locales, sourcedrv); /* generate boot files and other configurations */
  /*localcfg();*/ /* show local params (currency, etc), and propose to change them (based on localcfg) */
  /*netcfg();*/ /* basic networking config */
  finalreboot(); /* remove the CD and reboot */

 Quit:
  kittenclose(); /* close NLS support */
  video_clear(0x0700, 0, 0);
  video_movecursor(0, 0);
  return(0);
}
