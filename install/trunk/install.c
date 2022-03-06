/*
 * SVARDOS INSTALL PROGRAM
 *
 * PUBLISHED UNDER THE TERMS OF THE MIT LICENSE
 *
 * COPYRIGHT (C) 2016-2022 MATEUSZ VISTE, ALL RIGHTS RESERVED.
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

#include "svarlang.lib\svarlang.h"

#include "input.h"
#include "video.h"

/* keyboard layouts and locales */
#include "keylay.h"
#include "keyoff.h"

/* prototype of the int24hdl() function defined in int24hdl.asm */
void int24hdl(void);


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
  const char *keybcode;
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
static int putstringwrap(int y, int x, unsigned short attr, const char *s) {
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
static int putstringnls(int y, int x, unsigned short attr, int nlsmaj, int nlsmin) {
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


static int menuselect(int ypos, int xpos, int height, const char **list, int listlen) {
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
  const char *msg;
  msg = svarlang_strid(0x00); /* "SVARDOS INSTALLATION" */
  video_putcharmulti(0, 0, COLOR_TITLEBAR[mono], ' ', 80, 1);
  video_putstring(0, 40 - (strlen(msg) >> 1), COLOR_TITLEBAR[mono], msg, -1);
  video_clear(COLOR_BODY[mono], 80, -80);
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
  video_putchar(24, 0, COLOR_TITLEBAR[mono], ' ');
  video_putstringfix(24, 1, COLOR_TITLEBAR[mono], msg, 79);
  video_movecursor(25,0);
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
  msg = svarlang_strid(0x0100); /* "Welcome to SvarDOS" */
  x = 40 - (strlen(msg) >> 1);
  video_putstring(4, x, COLOR_BODY[mono], msg, -1);
  video_putcharmulti(5, x, COLOR_BODY[mono], '=', strlen(msg), 1);
  putstringnls(8, -1, COLOR_BODY[mono], 1, 1); /* "Please select your language from the list below:" */
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
  putstringnls(5, 1, COLOR_BODY[mono], 1, 5); /* "SvarDOS supports different keyboard layouts */
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
  const char *choice[3];
  choice[0] = svarlang_strid(0x0001);
  choice[1] = svarlang_strid(0x0002);
  choice[2] = NULL;
  newscreen(0);
  putstringnls(4, 1, COLOR_BODY[mono], 2, 0); /* "You are about to install SvarDOS */
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


/* returns 0 if file exists, non-zero otherwise */
static int fileexists(const char *fname) {
  FILE *fd;
  fd = fopen(fname, "rb");
  if (fd == NULL) return(-1);
  fclose(fd);
  return(0);
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
      switch (menuselect(6 + putstringwrap(4, 1, COLOR_BODY[mono], buff), -1, 5, list, -1)) {
        case 0:
          sprintf(buff, "FDISK /AUTO %d", driveid);
          system(buff);
          break;
        case 1:
          video_clear(0x0700, 0, 0);
          video_movecursor(0, 0);
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
      sprintf(buff, "FDISK /AMBR %d", driveid);
      system(buff); /* writes BOOT.MBR into actual MBR */
      newscreen(2);
      putstringnls(10, 10, COLOR_BODY[mono], 3, 1); /* "Your computer will reboot now." */
      putstringnls(12, 10, COLOR_BODY[mono], 0, 5); /* "Press any key..." */
      input_getkey();
      reboot();
      return(MENUQUIT);
    } else if (driveremovable > 0) {
      newscreen(2);
      snprintf(buff, sizeof(buff), svarlang_strid(0x0302), cselecteddrive); /* "ERROR: Drive %c: is a removable device */
      video_putstring(9, 1, COLOR_BODY[mono], buff, -1);
      putstringnls(11, 2, COLOR_BODY[mono], 0, 5); /* "Press any key..." */
      return(MENUQUIT);
    }
    /* if not formatted, propose to format it right away (try to create a directory) */
    if (test_drive_write(cselecteddrive, buff) != 0) {
      const char *list[3];
      newscreen(0);
      snprintf(buff, sizeof(buff), svarlang_str(3, 3), cselecteddrive); /* "ERROR: Drive %c: seems to be unformated. Do you wish to format it?") */
      video_putstring(7, 1, COLOR_BODY[mono], buff, -1);

      snprintf(buff, sizeof(buff), svarlang_strid(0x0007), cselecteddrive); /* "Format drive %c:" */
      list[0] = buff;
      list[1] = svarlang_strid(0x0002); /* "Quit to DOS" */
      list[2] = NULL;

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
      snprintf(buff, sizeof(buff), svarlang_strid(0x0304), cselecteddrive, SVARDOS_DISK_REQ); /* "ERROR: Drive %c: is not big enough! SvarDOS requires a disk of at least %d MiB." */
      y += putstringwrap(y, 1, COLOR_BODY[mono], buff);
      putstringnls(++y, 1, COLOR_BODY[mono], 0, 5); /* "Press any key..." */
      input_getkey();
      return(MENUQUIT);
    }
    /* is the disk empty? */
    newscreen(0);
    if (diskempty(selecteddrive) != 0) {
      const char *list[3];
      int y = 6;
      snprintf(buff, sizeof(buff), svarlang_strid(0x0305), cselecteddrive); /* "ERROR: Drive %c: not empty" */
      y += putstringwrap(y, 1, COLOR_BODY[mono], buff);

      snprintf(buff, sizeof(buff), svarlang_strid(0x0007), cselecteddrive); /* "Format drive %c:" */
      list[0] = buff;
      list[1] = svarlang_strid(0x0002); /* "Quit to DOS" */
      list[2] = NULL;

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
      const char *list[3];
      list[0] = svarlang_strid(0x0001); /* Install SvarDOS */
      list[1] = svarlang_strid(0x0002); /* Quit to DOS */
      list[2] = NULL;
      snprintf(buff, sizeof(buff), svarlang_strid(0x0306), cselecteddrive); /* "The installation of SvarDOS to %c: is about to begin." */
      video_putstring(7, -1, COLOR_BODY[mono], buff, -1);
      choice = menuselect(10, -1, 4, list, -1);
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
    fprintf(fd, "KEYB %s,%d,%%DOSDIR%%\\BIN\\", locales->keybcode, locales->codepage);
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
  fprintf(fd, "DOS=UMB,HIGH\r\n"
              "LASTDRIVE=Z\r\n"
              "FILES=50\r\n");
  fprintf(fd, "DEVICE=C:\\SVARDOS\\BIN\\HIMEMX.EXE\r\n");
  if (strcmp(locales->lang, "EN") == 0) {
    strcpy(buff, "COMMAND");
  } else {
    snprintf(buff, sizeof(buff), "CMD-%s", locales->lang);
  }
  fprintf(fd, "SHELLHIGH=C:\\SVARDOS\\BIN\\%s.COM /E:512 /P\r\n", buff);
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
  fprintf(fd, "PATH %%DOSDIR%%\\BIN\r\n");
  fprintf(fd, "PROMPT $P$G\r\n");
  fprintf(fd, "ALIAS REBOOT=FDAPM COLDBOOT\r\n");
  fprintf(fd, "ALIAS HALT=FDAPM POWEROFF\r\n");
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


static int installpackages(char targetdrv, char srcdrv, const struct slocales *locales, const char *buildstring) {
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
  /* copy pkg.exe to the new drive, along with all packages */
  snprintf(buff, sizeof(buff), "%c:\\TEMP\\pkg.exe", targetdrv);
  snprintf(buff + 64, sizeof(buff) - 64, "%c:\\pkg.exe", srcdrv);
  fcopy(buff, buff + 64, buff, sizeof(buff));

  /* open the post-install autoexec.bat and prepare initial instructions */
  snprintf(buff, sizeof(buff), "%c:\\temp\\postinst.bat", targetdrv);
  fd = fopen(buff, "wb");
  if (fd == NULL) return(-1);
  fprintf(fd, "@ECHO OFF\r\nECHO INSTALLING SVARDOS BUILD %s\r\n", buildstring);

  /* copy packages */
  pkgptr = pkglist;
  for (i = 0;; i++) {
    /* move forward to nearest entry or end of list */
    while (*pkgptr == 0) pkgptr++;
    if (*pkgptr == 0xff) break;
    /* install the package */
    snprintf(buff, sizeof(buff), svarlang_strid(0x0400), i+1, pkglistlen, pkgptr); /* "Installing package %d/%d: %s" */
    strcat(buff, "       ");
    video_putstringfix(10, 1, COLOR_BODY[mono], buff, sizeof(buff));
    /* wait for new diskette if package not found */
    snprintf(buff, sizeof(buff), "%c:\\%s.svp", srcdrv, pkgptr);
    while (fileexists(buff) != 0) {
      putstringnls(12, 1, COLOR_BODY[mono], 4, 1); /* "INSERT THE DISK THAT CONTAINS THE REQUIRED FILE AND PRESS ANY KEY" */
      input_getkey();
      video_putstringfix(12, 1, COLOR_BODY[mono], "", 80); /* erase the 'insert disk' message */
    }
    /* proceed with package copy (buff contains the src filename already) */
    snprintf(buff + 32, sizeof(buff) - 32, "%c:\\temp\\%s.svp", targetdrv, pkgptr);
    if (fcopy(buff + 32, buff, buff, sizeof(buff)) != 0) {
      video_putstring(10, 30, COLOR_BODY[mono], "READ ERROR", -1);
      input_getkey();
      fclose(fd);
      return(-1);
    }
    /* write install instruction to post-install script */
    fprintf(fd, "pkg install %s.svp\r\ndel %s.svp\r\n", pkgptr, pkgptr);
    /* jump to next entry or end of list */
    while ((*pkgptr != 0) && (*pkgptr != 0xff)) pkgptr++;
    if (*pkgptr == 0xff) break;
  }
  /* set up locales so the "installation over" message is nicely displayed */
  genlocalesconf(fd, locales);
  /* replace autoexec.bat and config.sys now and write some nice message on screen */
  fprintf(fd, "DEL pkg.exe\r\n"
              "COPY CONFIG.SYS C:\\\r\n"
              "DEL CONFIG.SYS\r\n"
              "DEL C:\\AUTOEXEC.BAT\r\n"
              "COPY AUTOEXEC.BAT C:\\\r\n"
              "DEL AUTOEXEC.BAT\r\n");
  /* print out the "installation over" message */
  fprintf(fd, "ECHO.\r\n"
              "ECHO ");
  fprintf(fd, svarlang_strid(0x0501), buildstring); /* "SvarDOS installation is over. Please restart your computer now" */
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
              "PATH %%DOSDIR%%\\BIN\r\n");
  fprintf(fd, "CD TEMP\r\n"
              "postinst.bat\r\n");
  fclose(fd);

  return(0);
}


static void finalreboot(void) {
  int y = 9;
  newscreen(2);
  y += putstringnls(y, 1, COLOR_BODY[mono], 5, 0); /* "Your computer will reboot now.\nPlease remove the installation disk from your drive" */
  putstringnls(++y, 1, COLOR_BODY[mono], 0, 5); /* "Press any key..." */
  input_getkey();
  reboot();
}


static void loadcp(const struct slocales *locales) {
  char buff[64];
  if (locales->codepage == 437) return;
  video_movecursor(1, 0);
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
  return(fileexists(fname));
}
#endif


int main(int argc, char **argv) {
  struct slocales locales;
  int targetdrv;
  int sourcedrv;
  int action;
  const char *buildstring = "###";

  /* setup the internal int 24h handler ("always fail") */
  int24hdl();

  if (argc != 1) buildstring = argv[1];

  sourcedrv = get_cur_drive() + 'A';

  /* init screen and detect mono status */
  mono = video_init();

 SelectLang:
  action = selectlang(&locales); /* welcome to svardos, select your language */
  if (action != MENUNEXT) goto Quit;
  loadcp(&locales);
  svarlang_load("INSTALL", locales.lang, NULL); /* NLS support */

 SelectKeyb:
  action = selectkeyb(&locales);  /* what keyb layout should we use? */
  if (action == MENUQUIT) goto Quit;
  if (action == MENUPREV) goto SelectLang;

 WelcomeScreen:
  action = welcomescreen(); /* what svardos is, ask whether to run live dos or install */
  if (action == MENUQUIT) goto Quit;
  if (action == MENUPREV) {
    if (locales.keyblen > 1) goto SelectKeyb; /* if there is a choice of more than 1 layout, ask for it */
    goto SelectLang;
  }
  targetdrv = preparedrive(sourcedrv); /* what drive should we install from? check avail. space */
  if (targetdrv == MENUQUIT) goto Quit;
  if (targetdrv == MENUPREV) goto WelcomeScreen;
  bootfilesgen(targetdrv, &locales); /* generate boot files and other configurations */
  if (installpackages(targetdrv, sourcedrv, &locales, buildstring) != 0) goto Quit;    /* install packages */
  /*localcfg();*/ /* show local params (currency, etc), and propose to change them (based on localcfg) */
  /*netcfg();*/ /* basic networking config */
  finalreboot(); /* remove the CD and reboot */

 Quit:
  video_clear(0x0700, 0, 0);
  video_movecursor(0, 0);
  return(0);
}
