/*
 * SVARDOS INSTALL PROGRAM
 *
 * PUBLISHED UNDER THE TERMS OF THE MIT LICENSE
 *
 * COPYRIGHT (C) 2016-2024 MATEUSZ VISTE, ALL RIGHTS RESERVED.
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
 * http://svardos.org
 */

#include <dos.h>
#include <direct.h>  /* mkdir() */
#include <stdio.h>   /* printf() and friends */
#include <stdlib.h>  /* system() */
#include <string.h>  /* memcpy() */
#include <unistd.h>

#include "mdr\cout.h"
#include "mdr\dos.h"
#include "svarlang.lib\svarlang.h"

/* keyboard layouts and locales */
#include "keylay.h"
#include "keyoff.h"

/* prototype of the int24hdl() function defined in int24hdl.asm */
void int24hdl(void);


/* color scheme (preset for color) */
static unsigned char COLOR_TITLEBAR  = 0x70;
static unsigned char COLOR_TITLEVER  = 0x78;
static unsigned char COLOR_BODY      = 0x17;
static unsigned char COLOR_SELECT    = 0x70;
static unsigned char COLOR_SELECTCUR = 0x1F;

/* build release string, populated at startup by reading floppy's label */
static char BUILDSTRING[13];

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
  const char *keybcode;
  unsigned int codepage;
  int egafile;
  int keybfile;
  int keyboff;
  int keyblen;
  unsigned int keybid;
};


/* put a string on screen and fill it until w chars with whilte space */
static void video_putstringfix(unsigned char y, unsigned char x, unsigned char attr, const char *s, unsigned char w) {
  unsigned char i;

  /* print the string up to w characters */
  i = mdr_cout_str(y, x, s, attr, w);

  /* fill in left space (if any) with blanks */
  mdr_cout_char_rep(y, x + i, ' ', attr, w - i);
}


/* reboot the computer */
static void reboot(void) {
  void ((far *bootroutine)()) = (void (far *)()) 0xFFFF0000L;
  int far *rstaddr = (int far *)0x00400072L; /* BIOS boot flag is at 0040:0072 */
  *rstaddr = 0x1234; /* 0x1234 = warm boot, 0 = cold boot */
  (*bootroutine)(); /* jump to the BIOS reboot routine at FFFF:0000 */
}


/* returns 1 if file exists, zero otherwise */
static int fileexists(const char *fname) {
  FILE *fd;
  fd = fopen(fname, "rb");
  if (fd == NULL) return(0);
  fclose(fd);
  return(1);
}


/* outputs a string to screen with taking care of word wrapping. returns amount of lines. */
static unsigned char putstringwrap(unsigned char y, unsigned char x, unsigned char attr, const char *s) {
  unsigned char linew, lincount;
  linew = 80 - (x << 1);

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
    mdr_cout_str(y++, x, s, attr, len);
    s += len;
    if (*s == 0) break;
    s += 1; /* skip the whitespace char */
  }
  return(lincount);
}


/* an NLS wrapper around video_putstring(), also performs line wrapping when
 * needed. returns the amount of lines that were output */
static unsigned char putstringnls(unsigned char y, unsigned char x, unsigned char attr, unsigned char nlsmaj, unsigned char nlsmin) {
  const char *s = svarlang_str(nlsmaj, nlsmin);
  if (s == NULL) s = "";
  return(putstringwrap(y, x, attr, s));
}


/* copy file f1 to f2 using buff as a buffer of buffsz bytes. f2 will be overwritten if it
 * exists already! returns 0 on success. */
static int fcopy(const char *f2, const char *f1, void *buff, size_t buffsz) {
  FILE *fd1, *fd2;
  size_t sz;
  int res = -1; /* assume failure */

  /* open files */
  fd1 = fopen(f1, "rb");
  fd2 = fopen(f2, "wb");
  if ((fd1 == NULL) || (fd2 == NULL)) goto QUIT;

  /* copy data */
  for (;;) {
    sz = fread(buff, 1, buffsz, fd1);
    if (sz == 0) {
      if (feof(fd1) != 0) break;
      goto QUIT;
    }
    if (fwrite(buff, 1, sz, fd2) != sz) goto QUIT;
  }

  res = 0; /* success */

  QUIT:
  if (fd1 != NULL) fclose(fd1);
  if (fd2 != NULL) fclose(fd2);
  return(res);
}


/* display a menu with items and return user's choice.
 * ypos: starting line where the menu is drawn
 * height: number of items to display inside the menu
 * list: NULL-terminated list of items
 * maxlistlen: limit list to this many items tops */
static int menuselect(unsigned char ypos, unsigned char height, const char **list, int maxlistlen) {
  int i, offset = 0, res = 0, count;
  unsigned char y, xpos, width = 0;

  /* count how many positions there are, and check their width */
  for (count = 0; (list[count] != NULL) && (count != maxlistlen); count++) {
    int len = strlen(list[count]);
    if (len > width) width = len;
  }
  width++; /* it's nice to have a small margin to the right of the widest item */

  /* if xpos negative, means 'center out' */
  xpos = 39 - (width >> 1);

  mdr_cout_char_rep(ypos, xpos, 0xC4, COLOR_SELECT, width + 2);  /* top line */
  mdr_cout_char(ypos, xpos+width+2, 0xBF, COLOR_SELECT);         /*       \ */
  mdr_cout_char(ypos, xpos-1, 0xDA, COLOR_SELECT);               /*  /      */
  ypos++; /* from now on ypos relates to the position of the content */
  mdr_cout_char(ypos+height, xpos-1, 0xC0, COLOR_SELECT);      /*  \      */
  mdr_cout_char(ypos+height, xpos+width+2, 0xD9, COLOR_SELECT);/*      /  */
  mdr_cout_char_rep(ypos+height, xpos, 0xC4, COLOR_SELECT, width + 2);

  for (;;) {
    int key;

    /* draw side borders of the menu + the cursor */
    if (count <= height) { /* no need for a cursor, all fits on one page */
      i = 255;
    } else {
      i = offset * (height - 1) / (count - height);
    }

    for (y = ypos; y < (ypos + height); y++) {
      mdr_cout_char(y, xpos-1, 0xB3, COLOR_SELECT); /* left side */
      if (y - ypos == i) {
        mdr_cout_char(y, xpos+width+2, '=', COLOR_SELECT); /* cursor */
      } else {
        mdr_cout_char(y, xpos+width+2, 0xB3, COLOR_SELECT); /* right side */
      }
    }

    /* list of selectable items */
    for (i = 0; i < height; i++) {
      if (i + offset == res) {
        mdr_cout_char(ypos + i, xpos, 16, COLOR_SELECTCUR);
        mdr_cout_char(ypos + i, xpos+width+1, 17, COLOR_SELECTCUR);
        mdr_cout_locate(ypos + i, xpos);
        video_putstringfix(ypos + i, xpos+1, COLOR_SELECTCUR, list[i + offset], width);
      } else if (i + offset < count) {
        mdr_cout_char(ypos + i, xpos, ' ', COLOR_SELECT);
        mdr_cout_char(ypos + i, xpos+width+1, ' ', COLOR_SELECT);
        video_putstringfix(ypos + i, xpos+1, COLOR_SELECT, list[i + offset], width);
      } else {
        mdr_cout_char_rep(ypos + i, xpos, ' ', COLOR_SELECT, width+2);
      }
    }
    key = mdr_dos_getkey();
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
        if (res > offset + height - 1) offset = res - (height - 1);
      }
    } else if (key == 0x147) { /* home */
      res = 0;
      offset = 0;
    } else if (key == 0x14F) { /* end */
      res = count - 1;
      if (res > offset + height - 1) offset = res - (height - 1);
    } else if (key == 0x1B) {  /* ESC */
      return(-1);
    }/* else {
      char buf[8];
      snprintf(buf, sizeof(buf), "0x%02X ", key);
      video_putstring(1, 0, COLOR_BODY, buf, -1);
    }*/
  }
}

static void newscreen(unsigned char statusbartype) {
  const char *msg;
  mdr_cout_cls(COLOR_BODY);
  msg = svarlang_strid(0x00); /* "SVARDOS INSTALLATION" */
  mdr_cout_char_rep(0, 0, ' ', COLOR_TITLEBAR, 80);
  mdr_cout_str(0, 40 - (strlen(msg) >> 1), msg, COLOR_TITLEBAR, 80);
  mdr_cout_str(0, 80 - strlen(BUILDSTRING), BUILDSTRING, COLOR_TITLEVER, 12);

  switch (statusbartype) {
    case 1:
      msg = svarlang_strid(0x000B); /* "Up/Down = Select entry | Enter = Validate your choice | ESC = Quit to DOS" */
      break;
    case 2:
      msg = svarlang_strid(0x0005); /* "Press any key..." */
      break;
    case 3:
      msg = "";
      break;
    default:
      msg = svarlang_strid(0x000A); /* "Up/Down = Select entry | Enter = Validate your choice | ESC = Previous screen" */
      break;
  }
  mdr_cout_char(24, 0, ' ', COLOR_TITLEBAR);
  video_putstringfix(24, 1, COLOR_TITLEBAR, msg, 79);
  mdr_cout_locate(25,0);
}

/* fills a slocales struct accordingly to the value of its keyboff member */
static void kblay2slocal(struct slocales *locales) {
  const char *m;
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
  const char *msg;
  const char *langlist[] = {
    "English",
    "Brazilian",
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

  /* do not ask for language on non-multilang setups */
  if (!fileexists("INSTALL.LNG")) {
    choice = 0;
    goto SkipLangSelect;
  }

  newscreen(1);
  msg = svarlang_strid(0x0100); /* "Welcome to SvarDOS" */
  x = 40 - (strlen(msg) >> 1);
  mdr_cout_str(4, x, msg, COLOR_BODY, 80);
  mdr_cout_char_rep(5, x, '=', COLOR_BODY, strlen(msg));

  /* center out the string "Please select your language..." */
  msg = svarlang_str(1, 1); /* "Please select your language from the list below:" */
  if (strlen(msg) > 74) {
    putstringwrap(8, 1, COLOR_BODY, msg);
  } else {
    mdr_cout_str(8, 40 - (strlen(msg) / 2), msg, COLOR_BODY, 80);
  }

  choice = menuselect(11, 9, langlist, -1);
  if (choice < 0) return(MENUPREV);

  SkipLangSelect:

  /* populate locales with default values */
  memset(locales, 0, sizeof(struct slocales));
  switch (choice) {
    case 1:
      strcpy(locales->lang, "BR");
      locales->keyboff = OFFLOC_BR;
      locales->keyblen = OFFLEN_BR;
      break;
    case 2:
      strcpy(locales->lang, "FR");
      locales->keyboff = OFFLOC_FR;
      locales->keyblen = OFFLEN_FR;
      break;
    case 3:
      strcpy(locales->lang, "DE");
      locales->keyboff = OFFLOC_DE;
      locales->keyblen = OFFLEN_DE;
      break;
    case 4:
      strcpy(locales->lang, "IT");
      locales->keyboff = OFFLOC_IT;
      locales->keyblen = OFFLEN_IT;
      break;
    case 5:
      strcpy(locales->lang, "PL");
      locales->keyboff = OFFLOC_PL;
      locales->keyblen = OFFLEN_PL;
      break;
    case 6:
      strcpy(locales->lang, "RU");
      locales->keyboff = OFFLOC_RU;
      locales->keyblen = OFFLEN_RU;
      break;
    case 7:
      strcpy(locales->lang, "SI");
      locales->keyboff = OFFLOC_SI;
      locales->keyblen = OFFLEN_SI;
      break;
    case 8:
      strcpy(locales->lang, "SV");
      locales->keyboff = OFFLOC_SV;
      locales->keyblen = OFFLEN_SV;
      break;
    case 9:
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
  putstringnls(5, 1, COLOR_BODY, 1, 5); /* "SvarDOS supports different keyboard layouts */
  menuheight = locales->keyblen;
  if (menuheight > 11) menuheight = 11;
  choice = menuselect(10, menuheight, &(kblayouts[locales->keyboff]), locales->keyblen);
  if (choice < 0) return(MENUPREV);
  /* (re)load the keyboard layout & codepage setup */
  locales->keyboff += choice;
  kblay2slocal(locales);
  return(MENUNEXT);
}


/* returns 0 if installation must proceed, non-zero otherwise */
static int welcomescreen(void) {
  int c;
  const char *choice[3];
  choice[0] = svarlang_strid(0x0001);
  choice[1] = svarlang_strid(0x0002);
  choice[2] = NULL;
  newscreen(0);
  putstringnls(4, 1, COLOR_BODY, 2, 0); /* "You are about to install SvarDOS */
  c = menuselect(13, 2, choice, -1);
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
  r.h.dl = drv;  /* A=1, B=2, etc */
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

#ifdef DEADCODE
/* set new DOS "current drive" to drv ('A', 'B', etc). returns 0 on success */
static int set_cur_drive(char drv) {
  union REGS r;
  if ((drv < 'A') || (drv > 'Z')) return(-1);
  r.h.ah = 0x0E; /* DOS 1+ SELECT DEFAULT DRIVE */
  r.h.dl = drv - 'A';
  int86(0x21, &r, &r);
  if (r.h.al < drv - 'A') return(-1);
  return(0);
}
#endif


/* get the DOS "current drive" (0=A:, 1=B:, etc) */
static int get_cur_drive(void) {
  union REGS r;
  r.h.ah = 0x19; /* DOS 1+ GET CURRENT DEFAULT DRIVE */
  int86(0x21, &r, &r);
  return(r.h.al);
}


/* tries to write an empty file to drive.
 * asks DOS to inhibit the int 24h handler for this job, so erros are
 * gracefully reported - unfortunately this does not work under FreeDOS because
 * the DOS-C kernel does not implement the required flag.
 * returns 0 on success (ie. "disk exists and is writeable"). */
static unsigned short test_drive_write(char drive, char *buff) {
  unsigned short result = 0;
  sprintf(buff, "%c:\\SVWRTEST.123", drive);
  _asm {
    push ax
    push bx
    push cx
    push dx

    mov ah, 0x6c   /* extended open/create file */
    mov bx, 0x2001 /* open for write + inhibit int 24h handler */
    xor cx, cx     /* file attributes on the created file */
    mov dx, 0x0010 /* create file if does not exist, fail if it exists */
    mov si, buff   /* filename to create */
    int 0x21
    jc FAILURE
    /* close the file (handle in AX) */
    mov bx, ax
    mov ah, 0x3e /* close file */
    int 0x21
    jc FAILURE
    /* delete the file */
    mov ah, 0x41 /* delete file pointed out by DS:DX */
    mov dx, buff
    int 0x21
    jnc DONE

    FAILURE:
    mov result, ax
    DONE:

    pop dx
    pop cx
    pop bx
    pop ax
  }
  return(result);
}


static int preparedrive(char sourcedrv) {
  int driveremovable;
  int selecteddrive = 3; /* default to 'C:' */
  int cselecteddrive;
  int ds;
  int choice;
  char buff[1024];
  int driveid = 1; /* fdisk runs on first drive (unless USB boot) */
  if (selecteddrive == get_cur_drive() + 1) { /* get_cur_drive() returns 0-based values (A=0) while selecteddrive is 1-based (A=1) */
    selecteddrive = 4; /* use D: if install is run from C: (typically because it was booted from USB?) */
    driveid = 2; /* primary drive is the emulated USB storage */
  }
  cselecteddrive = 'A' + selecteddrive - 1;
  for (;;) {
    driveremovable = isdriveremovable(selecteddrive);
    if (driveremovable < 0) {
      const char *list[4];
      newscreen(0);
      list[0] = svarlang_str(0, 3); /* Create a partition automatically */
      list[1] = svarlang_str(0, 4); /* Run the FDISK tool */
      list[2] = svarlang_str(0, 2); /* Quit to DOS */
      list[3] = NULL;
      snprintf(buff, sizeof(buff), svarlang_strid(0x0300), cselecteddrive, SVARDOS_DISK_REQ); /* "ERROR: Drive %c: could not be found. Note, that SvarDOS requires at least %d MiB of available disk space */
      switch (menuselect(6 + putstringwrap(4, 1, COLOR_BODY, buff), 3, list, -1)) {
        case 0:
          sprintf(buff, "FDISK /PRI:MAX %d", driveid);
          system(buff);
          break;
        case 1:
          mdr_cout_cls(0x07);
          mdr_cout_locate(0, 0);
          sprintf(buff, "FDISK %d", driveid);
          system(buff);
          break;
        case 2:
          return(MENUQUIT);
        default:
          return(-1);
      }
      /* write a temporary MBR which only skips the drive (in case BIOS would
       * try to boot off the not-yet-ready C: disk) */
      sprintf(buff, "FDISK /LOADIPL %d", driveid);
      system(buff); /* writes BOOT.MBR into actual MBR */
      newscreen(2);
      putstringnls(10, 10, COLOR_BODY, 3, 1); /* "Your computer will reboot now." */
      putstringnls(12, 10, COLOR_BODY, 0, 5); /* "Press any key..." */
      mdr_dos_getkey();
      reboot();
      return(MENUQUIT);
    } else if (driveremovable > 0) {
      newscreen(2);
      snprintf(buff, sizeof(buff), svarlang_strid(0x0302), cselecteddrive); /* "ERROR: Drive %c: is a removable device */
      mdr_cout_str(9, 1, buff, COLOR_BODY, 80);
      putstringnls(11, 2, COLOR_BODY, 0, 5); /* "Press any key..." */
      return(MENUQUIT);
    }
    /* if not formatted, propose to format it right away (try to create a directory) */
    if (test_drive_write(cselecteddrive, buff) != 0) {
      const char *list[3];
      newscreen(0);
      snprintf(buff, sizeof(buff), svarlang_str(3, 3), cselecteddrive); /* "ERROR: Drive %c: seems to be unformated. Do you wish to format it?") */
      mdr_cout_str(7, 1, buff, COLOR_BODY, 80);

      snprintf(buff, sizeof(buff), svarlang_strid(0x0007), cselecteddrive); /* "Format drive %c:" */
      list[0] = buff;
      list[1] = svarlang_strid(0x0002); /* "Quit to DOS" */
      list[2] = NULL;

      choice = menuselect(12, 2, list, -1);
      if (choice < 0) return(MENUPREV);
      if (choice == 1) return(MENUQUIT);
      mdr_cout_cls(0x07);
      mdr_cout_locate(0, 0);
      snprintf(buff, sizeof(buff), "FORMAT %c: /Q /U /Z:seriously /V:SVARDOS", cselecteddrive);
      system(buff);
      continue;
    }
    /* check total disk space */
    ds = disksize(selecteddrive);
    if (ds < SVARDOS_DISK_REQ) {
      int y = 9;
      newscreen(2);
      snprintf(buff, sizeof(buff), svarlang_strid(0x0304), cselecteddrive, SVARDOS_DISK_REQ); /* "ERROR: Drive %c: is not big enough! SvarDOS requires a disk of at least %d MiB." */
      y += putstringwrap(y, 1, COLOR_BODY, buff);
      putstringnls(++y, 1, COLOR_BODY, 0, 5); /* "Press any key..." */
      mdr_dos_getkey();
      return(MENUQUIT);
    }
    /* is the disk empty? */
    newscreen(0);
    if (diskempty(selecteddrive) != 0) {
      const char *list[3];
      int y = 6;
      snprintf(buff, sizeof(buff), svarlang_strid(0x0305), cselecteddrive); /* "ERROR: Drive %c: not empty" */
      y += putstringwrap(y, 1, COLOR_BODY, buff);

      snprintf(buff, sizeof(buff), svarlang_strid(0x0007), cselecteddrive); /* "Format drive %c:" */
      list[0] = buff;
      list[1] = svarlang_strid(0x0002); /* "Quit to DOS" */
      list[2] = NULL;

      choice = menuselect(++y, 2, list, -1);
      if (choice < 0) return(MENUPREV);
      if (choice == 1) return(MENUQUIT);
      mdr_cout_cls(0x07);
      mdr_cout_locate(0, 0);
      snprintf(buff, sizeof(buff), "FORMAT %c: /Q /U /Z:seriously /V:SVARDOS", cselecteddrive);
      system(buff);
      continue;
    } else {
      /* final confirmation */
      const char *list[3];
      list[0] = svarlang_strid(0x0001); /* Install SvarDOS */
      list[1] = svarlang_strid(0x0002); /* Quit to DOS */
      list[2] = NULL;
      snprintf(buff, sizeof(buff), svarlang_strid(0x0306), cselecteddrive); /* "The installation of SvarDOS to %c: is about to begin." */
      mdr_cout_str(7, 40 - strlen(buff), buff, COLOR_BODY, 80);
      choice = menuselect(10, 2, list, -1);
      if (choice < 0) return(MENUPREV);
      if (choice == 1) return(MENUQUIT);
      snprintf(buff, sizeof(buff), "SYS %c: %c: > NUL", sourcedrv, cselecteddrive);
      system(buff);
      sprintf(buff, "FDISK /MBR %d", driveid);
      system(buff);
      snprintf(buff, sizeof(buff), "%c:\\TEMP", cselecteddrive);
      mkdir(buff);
      return(cselecteddrive);
    }
  }
}


/* generates locales-related configurations and writes them to file (this
 * is used to compute autoexec.bat content) */
static void genlocalesconf(FILE *fd, const struct slocales *locales) {
  if (locales == NULL) return;

  fprintf(fd, "SET LANG=%s\r\n", locales->lang);

  if (locales->egafile > 0) {
    fprintf(fd, "DISPLAY CON=(EGA,,1)\r\n");
    if (locales->egafile == 1) {
      fprintf(fd, "MODE CON CP PREPARE=((%u) %%DOSDIR%%\\CPI\\EGA.CPX)\r\n", locales->codepage);
    } else {
      fprintf(fd, "MODE CON CP PREPARE=((%u) %%DOSDIR%%\\CPI\\EGA%d.CPX)\r\n", locales->codepage, locales->egafile);
    }
    fprintf(fd, "MODE CON CP SELECT=%u\r\n", locales->codepage);
  }

  if (locales->keybfile > 0) {
    fprintf(fd, "KEYB %s,%d,%%DOSDIR%%\\", locales->keybcode, locales->codepage);
    if (locales->keybfile == 1) {
      fprintf(fd, "KEYBOARD.SYS");
    } else {
      fprintf(fd, "KEYBRD%d.SYS", locales->keybfile);
    }
    if (locales->keybid != 0) fprintf(fd, " /ID:%d", locales->keybid);
    fprintf(fd, "\r\n");
  }
}


static void bootfilesgen(char targetdrv, const struct slocales *locales) {
  char buff[128];
  FILE *fd;
  /*** CONFIG.SYS ***/
  snprintf(buff, sizeof(buff), "%c:\\TEMP\\CONFIG.SYS", targetdrv);
  fd = fopen(buff, "wb");
  if (fd == NULL) return;
  fprintf(fd, "LASTDRIVE=Z\r\n"
              "FILES=40\r\n");
  fprintf(fd, "DEVICE=C:\\SVARDOS\\HIMEMX.EXE\r\n");
  fprintf(fd, "DOS=UMB,HIGH\r\n");
  fprintf(fd, "SHELL=C:\\COMMAND.COM /E:512 /P\r\n");
  fprintf(fd, "REM COUNTRY=001,%u,C:\\SVARDOS\\CFG\\COUNTRY.SYS\r\n", locales->codepage);
  fprintf(fd, "REM DEVICE=C:\\DRIVERS\\UDVD2\\UDVD2.SYS /D:SVCD0001 /H\r\n");
  fclose(fd);
  /*** AUTOEXEC.BAT ***/
  snprintf(buff, sizeof(buff), "%c:\\TEMP\\AUTOEXEC.BAT", targetdrv);
  fd = fopen(buff, "wb");
  if (fd == NULL) return;
  fprintf(fd, "@ECHO OFF\r\n");
  fprintf(fd, "SET TEMP=C:\\TEMP\r\n");
  fprintf(fd, "SET DOSDIR=C:\\SVARDOS\r\n");
  fprintf(fd, "SET NLSPATH=%%DOSDIR%%\\NLS;.\r\n");
  fprintf(fd, "SET DIRCMD=/OGNE/P/4\r\n");
  fprintf(fd, "SET WATTCP.CFG=%%DOSDIR%%\\CFG\r\n");
  fprintf(fd, "PATH %%DOSDIR%%\r\n");
  fprintf(fd, "PROMPT $P$G\r\n");
  fprintf(fd, "FDAPM APMDOS\r\n");
  fprintf(fd, "\r\n");
  genlocalesconf(fd, locales);
  fprintf(fd, "\r\n");
  fprintf(fd, "REM Uncomment the line below for CDROM support\r\n");
  fprintf(fd, "REM SHSUCDX /d:SVCD0001\r\n");
  fprintf(fd, "\r\n");
  fprintf(fd, "ECHO.\r\n");
  fprintf(fd, "ECHO %s\r\n", svarlang_strid(0x0600)); /* "Welcome to SvarDOS!" */
  fclose(fd);
  /*** CREATE DIRECTORY FOR CONFIGURATION FILES ***/
  snprintf(buff, sizeof(buff), "%c:\\SVARDOS", targetdrv);
  mkdir(buff);
  snprintf(buff, sizeof(buff), "%c:\\SVARDOS\\CFG", targetdrv);
  mkdir(buff);
  /*** PKG.CFG ***/
  snprintf(buff, sizeof(buff), "%c:\\SVARDOS\\CFG\\PKG.CFG", targetdrv);
  fd = fopen(buff, "wb");
  if (fd == NULL) return;
  fprintf(fd, "# pkg config file - specifies locations where packages should be installed\r\n"
              "\r\n"
              "# DOS core binaries\r\n"
              "DIR BIN C:\\SVARDOS\r\n"
              "\r\n"
              "# Programs\r\n"
              "DIR PROGS C:\\\r\n"
              "\r\n"
              "# Games \r\n"
              "DIR GAMES C:\\\r\n"
              "\r\n"
              "# Drivers\r\n"
              "DIR DRIVERS C:\\DRIVERS\r\n"
              "\r\n"
              "# Development tools\r\n"
              "DIR DEVEL C:\\DEVEL\r\n");
  fclose(fd);
  /*** COUNTRY.SYS ***/
  /*** PICOTCP ***/
  /*** WATTCP ***/
  snprintf(buff, sizeof(buff), "%c:\\SVARDOS\\CFG\\WATTCP.CFG", targetdrv);
  fd = fopen(buff, "wb");
  if (fd == NULL) return;
  fprintf(fd, "my_ip = dhcp\r\n"
              "#my_ip = 192.168.0.7\r\n"
              "#netmask = 255.255.255.0\r\n"
              "#nameserver = 192.168.0.1\r\n"
              "#nameserver = 192.168.0.2\r\n"
              "#gateway = 192.168.0.1\r\n");
  fclose(fd);
}


static int installpackages(char targetdrv, char srcdrv, const struct slocales *locales) {
  char pkglist[512];
  int i, pkglistlen;
  size_t pkglistflen;
  char buff[1024]; /* must be *at least* 1 sector big for efficient file copying */
  FILE *fd = NULL;
  char *pkgptr;
  newscreen(3);
  /* load pkg list */
  fd = fopen("install.lst", "rb");
  if (fd == NULL) {
    mdr_cout_str(10, 30, "ERROR: INSTALL.LST NOT FOUND", COLOR_BODY, 80);
    mdr_dos_getkey();
    return(-1);
  }
  pkglistflen = fread(pkglist, 1, sizeof(pkglist) - 2, fd);
  fclose(fd);
  if (pkglistflen == sizeof(pkglist) - 2) {
    mdr_cout_str(10, 30, "ERROR: INSTALL.LST TOO LARGE", COLOR_BODY, 80);
    mdr_dos_getkey();
    return(-1);
  }
  /* mark the end of list */
  pkglist[pkglistflen] = 0;
  pkglist[pkglistflen + 1] = 0xff;
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
  /* copy pkg.exe to the new drive, along with all packages */
  snprintf(buff, sizeof(buff), "%c:\\TEMP\\pkg.exe", targetdrv);
  snprintf(buff + 64, sizeof(buff) - 64, "%c:\\pkg.exe", srcdrv);
  fcopy(buff, buff + 64, buff, sizeof(buff));

  /* open the post-install autoexec.bat and prepare initial instructions */
  snprintf(buff, sizeof(buff), "%c:\\temp\\postinst.bat", targetdrv);
  fd = fopen(buff, "wb");
  if (fd == NULL) return(-1);
  fprintf(fd, "@ECHO OFF\r\nECHO INSTALLING SVARDOS BUILD %s\r\n", BUILDSTRING);

  /* move COMMAND.COM so it does not clashes with the installation of the SVARCOM package */
  fprintf(fd, "COPY \\COMMAND.COM \\CMD.COM\r\n");
  fprintf(fd, "SET COMSPEC=%c:\\CMD.COM\r\n", targetdrv);
  fprintf(fd, "DEL \\COMMAND.COM\r\n");

  /* copy packages */
  for (i = 0;; i++) {
    RETRY_ENTIRE_LIST:

    /* move forward to nearest entry or end of list */
    for (pkgptr = pkglist; *pkgptr == 0; pkgptr++);
    if (*pkgptr == 0xff) break; /* end of list: means all packages have been processed */

    /* is this package present on the floppy disk? */
    TRY_NEXTPKG:
    sprintf(buff, "%s.svp", pkgptr);
    if (!fileexists(buff)) {
      while (*pkgptr != 0) pkgptr++;
      while (*pkgptr == 0) pkgptr++;
      /* end of list? ask for next floppy, there's nothing interesting left on this one */
      if (*pkgptr == 0xff) {
        putstringnls(12, 1, COLOR_BODY, 4, 1); /* "INSERT THE DISK THAT CONTAINS THE REQUIRED FILE AND PRESS ANY KEY" */
        mdr_dos_getkey();
        video_putstringfix(12, 1, COLOR_BODY, "", 80); /* erase the 'insert disk' message */
        goto RETRY_ENTIRE_LIST;
      }
      goto TRY_NEXTPKG;
    }

    /* install the package */
    snprintf(buff, sizeof(buff), svarlang_strid(0x0400), i+1, pkglistlen, pkgptr); /* "Installing package %d/%d: %s" */
    strcat(buff, "       ");
    mdr_cout_str(10, 1, buff, COLOR_BODY, 40);

    /* proceed with package copy */
    sprintf(buff, "%c:\\temp\\%s.svp", targetdrv, pkgptr);
    if (fcopy(buff, buff + 7, buff, sizeof(buff)) != 0) {
      mdr_cout_str(10, 30, "READ ERROR", COLOR_BODY, 80);
      mdr_dos_getkey();
      fclose(fd);
      return(-1);
    }
    /* write install instruction to post-install script */
    fprintf(fd, "pkg install %s.svp\r\ndel %s.svp\r\n", pkgptr, pkgptr);
    /* jump to next entry or end of list and zero out the pkg name in the process */
    while ((*pkgptr != 0) && (*pkgptr != 0xff)) {
      *pkgptr = 0;
      pkgptr++;
    }
  }
  /* set up locales so the "installation over" message is nicely displayed */
  genlocalesconf(fd, locales);
  /* replace autoexec.bat and config.sys now and write some nice message on screen */
  fprintf(fd, "DEL pkg.exe\r\n"
              "COPY CONFIG.SYS C:\\\r\n"
              "DEL CONFIG.SYS\r\n"
              "DEL C:\\AUTOEXEC.BAT\r\n"
              "COPY AUTOEXEC.BAT C:\\\r\n"
              "DEL AUTOEXEC.BAT\r\n"
              "SET COMSPEC=C:\\COMMAND.COM\r\n"
              "DEL \\CMD.COM\r\n");
  /* print out the "installation over" message */
  fprintf(fd, "ECHO.\r\n"
              "ECHO ");
  fprintf(fd, svarlang_strid(0x0501), BUILDSTRING); /* "SvarDOS installation is over. Please restart your computer now" */
  fprintf(fd, "\r\n"
              "ECHO.\r\n");
  fclose(fd);

  /* prepare a dummy autoexec.bat that will call temp\postinst.bat */
  snprintf(buff, sizeof(buff), "%c:\\autoexec.bat", targetdrv);
  fd = fopen(buff, "wb");
  if (fd == NULL) return(-1);
  fprintf(fd, "@ECHO OFF\r\n"
              "SET DOSDIR=C:\\SVARDOS\r\n"
              "SET NLSPATH=%%DOSDIR%%\\NLS\r\n"
              "PATH %%DOSDIR%%\r\n");
  fprintf(fd, "CD TEMP\r\n"
              "postinst.bat\r\n");
  fclose(fd);

  return(0);
}


static void finalreboot(void) {
  int y = 9;
  newscreen(2);
  y += putstringnls(y, 1, COLOR_BODY, 5, 0); /* "Your computer will reboot now.\nPlease remove the installation disk from your drive" */
  putstringnls(++y, 1, COLOR_BODY, 0, 5); /* "Press any key..." */
  mdr_dos_getkey();
  reboot();
}


static void loadcp(const struct slocales *locales) {
  char buff[64];
  if (locales->codepage == 437) return;
  mdr_cout_locate(1, 0);
  if (locales->egafile == 1) {
    snprintf(buff, sizeof(buff), "MODE CON CP PREP=((%u) EGA.CPX) > NUL", locales->codepage);
  } else {
    snprintf(buff, sizeof(buff), "MODE CON CP PREP=((%u) EGA%d.CPX) > NUL", locales->codepage, locales->egafile);
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


#ifdef DEADCODE
/* checks that drive drv contains SvarDOS packages
 * returns 0 if found, non-zero otherwise */
static int checkinstsrc(char drv) {
  char fname[16];
  snprintf(fname, sizeof(fname), "%c:\\ATTRIB.SVP", drv);
  return(!fileexists(fname));
}
#endif


int main(void) {
  struct slocales locales;
  int targetdrv;
  int sourcedrv;
  int action;

  /* setup the internal int 24h handler ("always fail") */
  int24hdl();

  /* read the svardos build revision (from floppy label) */
  {
    const char *fspec = "*.*";
    const char *res = (void*)0x9E; /* default DTA is at PSP:80h, field 1Eh of DTA is the ASCIZ file name */

    _asm {
      push cx
      push dx

      mov ax, 0x4e00  /* findfirst */
      mov cx, 0x08    /* file attr mask, 0x08 = volume label */
      mov dx, fspec
      int 0x21
      jnc good
      xor ah, ah
      xchg bx, dx
      mov [bx], ah
      xchg bx, dx
      good:

      pop dx
      pop cx
    }

    memcpy(BUILDSTRING, res, 12);
  }

  sourcedrv = get_cur_drive() + 'A';

  /* init screen and detect mono adapters */
  if (mdr_cout_init(NULL, NULL) == 0) {
    /* overload color scheme with mono settings */
    COLOR_TITLEBAR = 0x70;
    COLOR_TITLEVER = 0x70;
    COLOR_BODY = 0x07;
    COLOR_SELECT = 0x70;
    COLOR_SELECTCUR = 0x07;
  }

 SelectLang:
  action = selectlang(&locales); /* welcome to svardos, select your language */
  if (action != MENUNEXT) goto Quit;
  loadcp(&locales);
  svarlang_load("INSTALL.LNG", locales.lang); /* NLS support */

  action = selectkeyb(&locales);  /* what keyb layout should we use? */
  if (action == MENUQUIT) goto Quit;
  if (action == MENUPREV) {
    if (!fileexists("INSTALL.LNG")) goto Quit;
    goto SelectLang;
  }

 WelcomeScreen:
  action = welcomescreen(); /* what svardos is, ask whether to run live dos or install */
  if (action == MENUQUIT) goto Quit;
  if (action == MENUPREV) goto SelectLang;

  targetdrv = preparedrive(sourcedrv); /* what drive should we install from? check avail. space */
  if (targetdrv == MENUQUIT) goto Quit;
  if (targetdrv == MENUPREV) goto WelcomeScreen;
  bootfilesgen(targetdrv, &locales); /* generate boot files and other configurations */
  if (installpackages(targetdrv, sourcedrv, &locales) != 0) goto Quit;    /* install packages */
  /*localcfg();*/ /* show local params (currency, etc), and propose to change them (based on localcfg) */
  /*netcfg();*/ /* basic networking config */
  finalreboot(); /* remove the CD and reboot */

 Quit:
  mdr_cout_locate(0, 0);
  mdr_cout_close();
  return(0);
}
