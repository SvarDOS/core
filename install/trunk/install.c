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
#include "mdr\video.h"
#include "svarlang.lib\svarlang.h"

/* keyboard layouts and locales */
#include "keylay.h"
#include "keyoff.h"


/* color scheme (preset for color) */
static unsigned char COLOR_TITLEBAR  = 0x70;
static unsigned char COLOR_TITLEVER  = 0x78;
static unsigned char COLOR_BODY      = 0x17;
static unsigned char COLOR_BODYWARN  = 0x1F;
static unsigned char COLOR_SELECT    = 0x70;
static unsigned char COLOR_SELECTCUR = 0x1F;

/* build release string, populated at startup by reading floppy's label */
static char BUILDSTRING[13];

/* how much disk space does SvarDOS require (in MiB) */
#define SVARDOS_DISK_REQ 4

/* menu screens can output only one of these: */
#define MENUNEXT 0
#define MENUPREV -1
#define MENUQUIT -2

/* a convenience 'function' used for debugging */
#define DBG(x) { video_putstringfix(24, 0, 0x4F00u, x, 80); }

struct slocales {
  const char *lang;
  const char *keybcode;
  unsigned short codepage;
  unsigned char egafile;
  unsigned char keybfile;
  short keyboff;
  short keyblen;
  unsigned short keybid;
  unsigned short countryid; /* 1=USA, 33=FR, 48=PL, etc */
};


/* a special routine to display a message trough the video BIOS. this is much
 * slower than direct writes to video memory, but it is the only way to make
 * provox read such info message */
static void infomsg(unsigned char row, unsigned char col, const char *msg, unsigned char attr, unsigned short maxlen);
#pragma aux infomsg = \
"mov cx, 1" \
"xor bh, bh" \
"NEXT:" \
/* put cursor at position [dl,dh] */ \
"mov ah, 0x02" \
"int 0x10" \
/* am I allowed to print more? */ \
"test di, di" \
"jz DONE" \
/* write char (unless nul terminator) */ \
"mov ah, 0x09" \
"lodsb" \
"test al, al" \
"jz DONE" \
"int 0x10" \
"inc dl" \
"dec di" \
"jmp short NEXT" \
"DONE:" \
modify [ax bx cx dx si di] \
parm [dh] [dl] [si] [bl] [di]


/* install a dummy int24h handler that always fails. this is to avoid the
 * annoying "abort, retry, fail... DOS messages. */
static void install_int24(void) {
  static unsigned char handler[] = { /* contains machine code instructions */
    0xB0, 0x03,  /* mov al, 3   ; tell DOS the action has to FAIL   */
    0xCF};       /* ret         ; return from the interrupt handler */
  /* install the handler */
  _asm {
    push dx
    mov ax, 0x2524          /* set INT vector 0x24 (to DS:DX)   */
    mov dx, offset handler  /* DS:DX points at my dummy handler */
    int 0x21
    pop dx
  }
}


static void exec(const char *s) {
  system(s);
  install_int24(); /* reinstall my int24 handler, apparently system() reverts
                      the original (DOS) one */
}


/* put a string on screen and fill it until w chars with white space */
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
    infomsg(y++, x, s, attr, len);
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
 * height: max number of items to display inside the menu
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

  /* adjust height if there is less items than max height */
  if (count < height) height = count;

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
        mdr_cout_char(y, xpos+width+2, ' ', 0); /* cursor */
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
      UP_AGAIN:
      if (res > 0) {
        res--;
        if (res < offset) offset = res;
        if (list[res][0] == 0) goto UP_AGAIN;
      }
    } else if (key == 0x150) { /* down */
      DOWN_AGAIN:
      if (res+1 < count) {
        res++;
        if (res > offset + height - 1) offset = res - (height - 1);
        if (list[res][0] == 0) goto DOWN_AGAIN;
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

  /* use the specific codepage only on EGA+ */
  if (video_detectega()) {
    locales->codepage = ((unsigned short)m[1] << 8) | m[2];
    locales->egafile = m[3];
  } else {
    locales->codepage = 437;
    locales->egafile = 0;
  }

  locales->keybfile = m[4];
  locales->keybid = ((unsigned short)m[5] << 8) | m[6];
  locales->countryid = ((unsigned short)m[7] << 8) | m[8];
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

  newscreen(1);
  msg = svarlang_strid(0x0100); /* "Welcome to SvarDOS" */
  x = 40 - (strlen(msg) >> 1);
  infomsg(4, x, msg, COLOR_BODY, 80);
  mdr_cout_char_rep(5, x, '=', COLOR_BODY, strlen(msg));

  /* center out the string "Please select your language..." */
  msg = svarlang_str(1, 1); /* "Please select your language from the list below:" */
  if (strlen(msg) > 74) {
    putstringwrap(8, 1, COLOR_BODY, msg);
  } else {
    infomsg(8, 40 - (strlen(msg) / 2), msg, COLOR_BODY, 80);
  }

  choice = menuselect(11, 9, langlist, -1);
  if (choice < 0) return(MENUPREV);

  /* populate locales with default values */
  memset(locales, 0, sizeof(struct slocales));
  switch (choice) {
    case 1:
      locales->lang = "BR";
      locales->keyboff = OFFLOC_BR;
      locales->keyblen = OFFLEN_BR;
      break;
    case 2:
      locales->lang = "FR";
      locales->keyboff = OFFLOC_FR;
      locales->keyblen = OFFLEN_FR;
      break;
    case 3:
      locales->lang = "DE";
      locales->keyboff = OFFLOC_DE;
      locales->keyblen = OFFLEN_DE;
      break;
    case 4:
      locales->lang = "IT";
      locales->keyboff = OFFLOC_IT;
      locales->keyblen = OFFLEN_IT;
      break;
    case 5:
      locales->lang = "PL";
      locales->keyboff = OFFLOC_PL;
      locales->keyblen = OFFLEN_PL;
      break;
    case 6:
      locales->lang = "RU";
      locales->keyboff = OFFLOC_RU;
      locales->keyblen = OFFLEN_RU;
      break;
    case 7:
      locales->lang = "SI";
      locales->keyboff = OFFLOC_SI;
      locales->keyblen = OFFLEN_SI;
      break;
    case 8:
      locales->lang = "SV";
      locales->keyboff = OFFLOC_SV;
      locales->keyblen = OFFLEN_SV;
      break;
    case 9:
      locales->lang = "TR";
      locales->keyboff = OFFLOC_TR;
      locales->keyblen = OFFLEN_TR;
      break;
    default:
      locales->lang = "EN";
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


/* returns total disk space of drive drv (in MiB, max 2048, A=1 B=2 etc), or -1 if drive invalid */
static int disksize(unsigned char drv) {
  struct diskfree_t df;
  long res;

  if (_dos_getdiskfree(drv, &df) != 0) return(-1);

  res = df.sectors_per_cluster;
  res *= df.total_clusters;
  res *= df.bytes_per_sector;
  res >>= 20;
  return((int)res);
}


/* returns 0 if disk is empty, non-zero otherwise */
static int diskempty(char drv) {
  unsigned int rc;
  int res;
  char buff[8];
  struct find_t fileinfo;
  snprintf(buff, sizeof(buff), "%c:\\*.*", drv);
  rc = _dos_findfirst(buff, _A_NORMAL | _A_SUBDIR | _A_HIDDEN | _A_SYSTEM, &fileinfo);
  if (rc == 0) {
    res = 1; /* call successfull means disk is not empty */
  } else {
    res = 0;
  }
  /* _dos_findclose(&fileinfo); */ /* apparently required only on OS/2 */
  return(res);
}


/* replace all occurences of char a by char b in s */
static void strtr(char *s, char a, char b) {
  for (;*s != 0; s++) {
    if (*s == a) *s = b;
  }
}


/* tries to write an empty file to drive.
 * asks DOS to inhibit the int 24h handler for this job, so erros are
 * gracefully reported - unfortunately this does not work under FreeDOS because
 * the DOS-C kernel does not implement the required flag.
 * returns 0 on success (ie. "disk exists and is writeable"). */
static unsigned short test_drive_write(char drive) {
  unsigned short result = 0;
  char *buff = "@:\\SVWRTEST.123";
  buff[0] = drive;
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


#define MBR_WRITE 3
#define MBR_READ 2
/* read (action=2) or write (action=3) MBR of driveid (0x80, 0x81..) into buff */
static int READ_OR_WRITE_MBR(unsigned char action, char *buff, unsigned char driveid);
#pragma aux READ_OR_WRITE_MBR = \
"push es" \
"mov al, 1"       /* read 1 sector into memory */ \
"mov cx, 0x0001"  /* cylinder 0, sector 1 */ \
"xor dh, dh"      /* head 0 */ \
"push ds" \
"pop es" \
"int 0x13" \
"mov ax, 0" /* do not do "xor ax, ax", CF needs to be preserved */ \
"jnc DONE" \
"inc ax" \
"DONE:" \
"pop es" \
parm [ah] [bx] [dl] \
modify [cx dx] \
value [ax]



/*
 * MBR structure:
 *
 * Boot signature (word 0xAA55) is at 0x01FE
 *
 * partition entry 1 is at 0x01BE
 *                 2    at 0x01CE
 *                 3    at 0x01DE
 *                 4    at 0x01EE
 *
 * a partition entry is:
 * 0x00  status byte (active is bit 0x80 set)
 * 0x01  CHS addr of first sector (here: HEAD)
 * 0x02  CHS addr of first sector (CCSSSSSS) sector in 6 low bits, cylinder high bits in CC
 * 0x03  CHS addr of furst sector (here: low 8 bits of cylinder)
 * 0x04  Partition type:
        0x06/0x08 for CHS FAT16/32
        0x0E/0x0C for LBA FAT16/32
 * 0x05  CHS addr of last sector (same format as for 1st sector)
 * 0x06  CHS addr of last sector (same format as for 1st sector)
 * 0x07  CHS addr of last sector (same format as for 1st sector)
 * 0x08  LBA addr of first sector (4 bytes) - used sometimes instead of CHS
 * 0x0C  number of sectors in partition (4 bytes)
 */

struct drivelist {
  unsigned long start_lba;
  unsigned long tot_sect;  /* size (of partition), in sectors */
  unsigned short tot_size; /* size in MiB */
  unsigned char partid;    /* position of the partition in the MBR (0,1,2,3) */
  unsigned char hd;        /* 0-3 */
  unsigned char dosid;     /* DOS drive id (A=0, B=1...)*/
};


/* a (very partial) struct mimicking what EDR-DOS uses in int 2Fh,AX=0803h */
#pragma pack (push)
#pragma pack (0) /* disable the automatic alignment of struct members */
struct dos_udsc {
  unsigned short next_off; /* offset FFFFh if last */
  unsigned short next_seg;
  unsigned char hd;        /* physical unit (as in int 13h) */
  unsigned char letter;    /* DOS letter drive (A=0, B=1, C=2, ...) */
  unsigned short sectsize; /* bytes per sector */
  unsigned char unknown[15];
  unsigned long start_lba; /* LBA address of the partition (only for primary partitions) */
};
#pragma pack (pop)


/* get the DOS drive data table list */
static struct dos_udsc far *get_dos_udsc(void);
#pragma aux get_dos_udsc = \
"push ds" \
"mov ax, 0x0803" \
"int 0x2f" /* drive data table list is in DS:DI, error if DI is 0xffff */ \
"push ds" \
"pop es" \
"pop ds" \
"cmp di, 0xffff" \
"jne DONE" \
"xor di, di" \
"mov es, di" \
"DONE:" \
modify [ax] \
value [es di]


/* returns 0 if fsid is a valid (recognized) filesystem for SvarDOS install  */
static int is_fstype_valid(unsigned char fsid) {
  switch (fsid) {
    case 0x01: /* FAT-12 in first 32M of disk                                */
    case 0x04: /* FAT-16 partition that resides within first 32M of disk     */
    case 0x06: /* FAT-16B with 64K+ sectors, must be within first 8G of disk */
    case 0x0B: /* FAT-32 with CHS addressing                                 */
    case 0x0C: /* FAT-32 with LBA addressing                                 */
    case 0x0E: /* FAT-16B with LBA addressing                                */
      return(0);
    default:
      return(-1);
  }
}


/* reads the MBR and matches its entries to DOS drives
 * fills the drives array with up to 16 drives (4 partitions * 4 disks)
 * see https://github.com/SvarDOS/bugz/issues/89 */
static int get_drives_list(char *buff, struct drivelist *drives) {
  int i;
  int listlen = 0;
  int drv;
  struct dos_udsc far *udsc_root = get_dos_udsc();
  struct dos_udsc far *udsc_node;

  for (drv = 0x80; drv < 0x84; drv++) {

    if (READ_OR_WRITE_MBR(MBR_READ, buff, drv) != 0) continue;

    /* check boot signature at 0x01fe */
    if (*(unsigned short *)(buff + 0x01fe) != 0xAA55) continue;

    /* iterate over the 4 partitions in the MBR */
    for (i = 0; i < 4; i++) {
      unsigned char *entry;

      entry = buff + 0x01BE + (i * 16);

      /* ignore partition if fs is unknown (non-FAT) */
      if (is_fstype_valid(entry[4]) != 0) continue;

      bzero(&(drives[listlen]), sizeof(struct drivelist));
      drives[listlen].hd = drv & 3;
      drives[listlen].partid = i;
      drives[listlen].start_lba = ((unsigned long *)entry)[2];
      drives[listlen].tot_sect = ((unsigned long *)entry)[3];

      /* now iterate over DOS drives and try to match one to this MBR entry */
      udsc_node = udsc_root;
      while (udsc_node != NULL) {
        if (udsc_node->hd != drv) goto NEXT;
        if (udsc_node->start_lba != drives[listlen].start_lba) goto NEXT;

        /* FOUND! */
        drives[listlen].dosid = udsc_node->letter;
        {
          unsigned long sz = (drives[listlen].tot_sect * udsc_node->sectsize) >> 20;
          drives[listlen].tot_size = (unsigned short)sz;
        }
        listlen++;
        break;

        NEXT:
        if (udsc_node->next_off == 0xffff) break;
        udsc_node = MK_FP(udsc_node->next_seg, udsc_node->next_off);
      } /* iterate over UDSC nodes */
    } /* iterate over partition entries of the MBR */
  } /* iterate over BIOS disks (0x80, 0x81, ...) */

  return(listlen);
}


/* displays a list of drives and asks user to choose one for installation
 * returns a word with bits HHPPLLLLLLLL where:
 * HH = hard drive id (0..3)
 * PP = position of the partition in MBR (0..3)
 * LL... = drive's DOS id (A=0, B=1, C=2, ...) */
static int selectdrive(void) {
  char buff[512];
  struct drivelist drives[16];
  char drvlist[16][32]; /* up to 16 drives (4 partitions on 4 disks) */
  int i, drvlistlen;
  const char *menulist[16];
  unsigned char driveid = 1; /* fdisk runs on first drive (unless USB boot) */

  /* read MBR of first HDD */
  drvlistlen = get_drives_list(buff, drives);

  /* if no drive found - disk not partitioned? */
  if (drvlistlen == 0) {
    const char *list[5];
    newscreen(0);
    list[0] = svarlang_str(0, 3); /* Create a partition automatically */
    list[1] = svarlang_str(0, 12); /* Create a partition automatically (no FAT32) */
    list[2] = svarlang_str(0, 4); /* Run the FDISK tool */
    list[3] = svarlang_str(0, 2); /* Quit to DOS */
    list[4] = NULL;
    snprintf(buff, sizeof(buff), svarlang_strid(0x0300), SVARDOS_DISK_REQ); /* "ERROR: No drive could be found. Note, that SvarDOS requires at least %d MiB of available disk space */
    switch (menuselect(6 + putstringwrap(4, 1, COLOR_BODY, buff), 3, list, -1)) {
      case 0:
        sprintf(buff, "FDISK /IPL /PRI:MAX %u", driveid);
        exec(buff);
        break;
      case 1:
        sprintf(buff, "FDISK /IPL /PRIO:MAX %u", driveid);
        exec(buff);
        break;        
      case 2:
        mdr_cout_cls(0x07);
        mdr_cout_locate(0, 0);
        sprintf(buff, "FDISK %u", driveid);
        exec(buff);
        break;
      case 3:
        return(MENUQUIT);
      default:
        return(-1);
    }
    newscreen(2);
    putstringnls(10, 10, COLOR_BODY, 3, 1); /* "Your computer will reboot now." */
    putstringnls(12, 10, COLOR_BODY, 0, 5); /* "Press any key..." */
    mdr_dos_getkey();
    reboot();
    return(MENUQUIT);
  }

  /* build a menu with all drives */
  for (i = 0; i < drvlistlen; i++) {
    snprintf(drvlist[i], sizeof(drvlist[0]), "%c: [%u MiB, hd%c%u]", 'A' + drives[i].dosid, drives[i].tot_size, 'a' + drives[i].hd, drives[i].partid);
    menulist[i] = drvlist[i];
  }
  menulist[i++] = "";
  menulist[i++] = svarlang_str(0, 2); /* Quit to DOS */
  menulist[i] = NULL;

  newscreen(0);

  snprintf(buff, sizeof(buff), "%s", svarlang_strid(0x0307));
  i = menuselect(7 + putstringwrap(4, 1, COLOR_BODY, buff) /*ypos*/, 10 /*max-height*/, menulist, -1);
  if (i < 0) {
    return(MENUPREV);
  } else if (i < drvlistlen) {
    /* return a bitfield HHPPLLLLLLLL
     * HH = hard drive id (0..3)
     * PP = position of the partition in MBR (0..3)
     * LL... = drive's DOS id (A=0, B=1, C=2, ...) */
    return((drives[i].partid << 8) | (drives[i].hd << 10) | drives[i].dosid);
  }

  return(MENUQUIT);
}

/* hd_drv is a bitfield HHPPLLLLLLLL
 * HH = hard drive id (0..3)
 * PP = position of the partition in MBR (0..3)
 * LL... = drive's DOS id (A=0, B=1, C=2, ...) */
static int preparedrive(int hd_drv) {
  unsigned char selecteddrive;
  unsigned char driveid, partid;
  char cselecteddrive;
  int choice;
  char buff[512];

  /* decode hd and drive id from hd_drv */
  selecteddrive = hd_drv & 0xff; /* DOS drive letter (A=0. B=1, C=2 etc) */
  partid = (hd_drv >> 8) & 3;    /* position of partition in MBR (0..3) */
  driveid = (hd_drv >> 10);      /* HDD identifier (0..3) */

  cselecteddrive = 'A' + selecteddrive;

  TRY_AGAIN:

  /* if not formatted, propose to format it right away (try to create a directory) */
  if (test_drive_write(cselecteddrive) != 0) {
    const char *list[3];
    newscreen(0);
    snprintf(buff, sizeof(buff), svarlang_str(3, 3), cselecteddrive); /* "ERROR: Drive %c: seems to be unformated. Do you wish to format it?") */
    infomsg(7, 1, buff, COLOR_BODY, 80);

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
    exec(buff);
    goto TRY_AGAIN;
  }

  /* check total disk space */
  if (disksize(selecteddrive+1) < SVARDOS_DISK_REQ) {
    int y = 9;
    newscreen(2);
    snprintf(buff, sizeof(buff), svarlang_strid(0x0304), cselecteddrive, SVARDOS_DISK_REQ); /* "ERROR: Drive %c: is not big enough! SvarDOS requires a disk of at least %d MiB." */
    y += putstringwrap(y, 1, COLOR_BODY, buff);
    putstringnls(++y, 1, COLOR_BODY, 0, 5); /* "Press any key..." */
    mdr_dos_getkey();
    return(MENUPREV);
  }

  /* is the disk empty? */
  newscreen(0);
  if (diskempty(cselecteddrive) != 0) {
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
    exec(buff);
    goto TRY_AGAIN;
  } else {
    /* final confirmation */
    const char *list[3];
    list[0] = svarlang_strid(0x0001); /* Install SvarDOS */
    list[1] = svarlang_strid(0x0002); /* Quit to DOS */
    list[2] = NULL;
    snprintf(buff, sizeof(buff), svarlang_strid(0x0306), cselecteddrive); /* "The installation of SvarDOS to %c: is about to begin." */
    infomsg(7, 40 - (strlen(buff) / 2), buff, COLOR_BODY, 80);
    choice = menuselect(10, 2, list, -1);
    if (choice < 0) return(MENUPREV);
    if (choice == 1) return(MENUQUIT);

    /* update the disk's MBR, transfer the boot system, make sure the partition
     * is ACTIVE and create a TEMP directory to copy SVP files over */
    snprintf(buff, sizeof(buff), "SYS %c: > NUL", cselecteddrive);
    exec(buff);
    sprintf(buff, "FDISK /IPL %u", driveid + 1);
    exec(buff);

    if (READ_OR_WRITE_MBR(MBR_READ, buff, driveid | 0x80) == 0) {
      /* active flags for part 0,1,2,3 are at offsets 0x01BE, 0x01CE, 0x01DE, 0x01EE */
      unsigned char i, flag_before, changed = 0;
      unsigned short memoffset = 0x01BE; /* first partition flag is here */
      for (i = 0; i < 4; i++) {
        flag_before = buff[memoffset];
        buff[memoffset] &= 127;
        if (i == partid) buff[memoffset] |= 0x80;
        if (flag_before != buff[memoffset]) changed++;
        memoffset += 16; /* jump to next partition entry */
      }
      /* do I need to update the MBR? */
      if (changed != 0) READ_OR_WRITE_MBR(MBR_WRITE, buff, driveid | 0x80);
    }

    snprintf(buff, sizeof(buff), "%c:\\TEMP", cselecteddrive);
    mkdir(buff);
    return(0);
  }
}


/* generates locales-related configurations and writes them to file (this
 * is used to compute autoexec.bat content) */
static void genlocalesconf(FILE *fd, const struct slocales *locales) {

  fprintf(fd, "SET LANG=%s\r\n", locales->lang);

  if (locales->egafile > 0) {
    fprintf(fd, "DISPLAY CON=(EGA,,1)\r\n");
    if (locales->egafile == 1) {
      fprintf(fd, "MODE CON CP PREPARE=((%u) %%DOSDIR%%\\CPI\\EGA.CPX)\r\n", locales->codepage);
    } else {
      fprintf(fd, "MODE CON CP PREPARE=((%u) %%DOSDIR%%\\CPI\\EGA%u.CPX)\r\n", locales->codepage, locales->egafile);
    }
    fprintf(fd, "MODE CON CP SELECT=%u\r\n", locales->codepage);
  }

  if (locales->keybfile > 0) {
    fprintf(fd, "KEYB %s,%d,%%DOSDIR%%\\", locales->keybcode, locales->codepage);
    if (locales->keybfile == 1) {
      fprintf(fd, "KEYBOARD.SYS");
    } else {
      fprintf(fd, "KEYBRD%u.SYS", locales->keybfile);
    }
    if (locales->keybid != 0) fprintf(fd, " /ID:%d", locales->keybid);
    fprintf(fd, "\r\n");
  }
}


/* get the DOS "current drive" (0=A:, 1=B:, etc) */
static unsigned char get_cur_drive(void);
#pragma aux get_cur_drive = \
"mov ah, 0x19" /* DOS 1+ GET CURRENT DEFAULT DRIVE */ \
"int 0x21" \
modify [ah] \
value [al]


/* generates configuration files on the dest drive, this is run once system
 * booted successfully on the dst drive (postinst stage) */
static void bootfilesgen(void) {
  char buff[128];
  FILE *fd;
  unsigned char bootdrv = get_cur_drive() + 'A';
  struct slocales locales;

  /* load locales from ZLOCALES.DAT */
  fd = fopen("ZLOCALES.DAT", "rb");
  if (fd == NULL) return;
  fread(&locales, sizeof(struct slocales), 1, fd);
  fclose(fd);

  svarlang_load("INSTALL.LNG", locales.lang);

  /****************
   * CONFIG.SYS ***
   ****************/
  snprintf(buff, sizeof(buff), "%c:\\CONFIG.SYS", bootdrv);
  fd = fopen(buff, "wb");
  if (fd == NULL) return;
  fprintf(fd, "; SvarDOS kernel configuration\r\n"
              "\r\n"
              "; highest allowed drive letter\r\n"
              "LASTDRIVE=Z\r\n"
              "\r\n"
              "; max. number of files that programs are allowed to open simultaneously\r\n"
              "FILES=25\r\n");
  fprintf(fd, "\r\n"
              "; SHELL history: this allows to recall previous commands with up/down keys\r\n"
              "; Value after comma is the amount of allocated bytes (min: 128, max: 4096)\r\n"
              "HISTORY=ON,256\r\n");
  fprintf(fd, "\r\n"
              "; XMS memory driver\r\n"
              "DEVICE=%c:\\SVARDOS\\HIMEMX.EXE\r\n", bootdrv);
  fprintf(fd, "\r\n"
              "; try moving DOS to upper memory, then to high memory\r\n"
              "DOS=UMB,HIGH\r\n");
  fprintf(fd, "\r\n"
              "; command interpreter (shell) location and environment size\r\n"
              "SHELL=%c:\\COMMAND.COM /E:512 /P\r\n", bootdrv);
  fprintf(fd, "\r\n"
              "; NLS configuration\r\n");
  fprintf(fd, "COUNTRY=%03u,%u,%c:\\SVARDOS\\COUNTRY.SYS\r\n", locales.countryid, locales.codepage, bootdrv);

  /* insert provox initialization if to be installed */
  if (fileexists("PROVOX.SVP")) {
    fprintf(fd, "\r\n"
                "; PROVOX SCREEN READER DRIVER\r\n"
                "DEVICE=%c:\\DRIVERS\\PROVOX\\PROVOX7.EXE\r\n", bootdrv);
  }

  fprintf(fd, "\r\n"
              "; CD-ROM driver initialization\r\n"
              ";DEVICE=%c:\\DRIVERS\\VIDECDD\\VIDE-CDD.SYS /D:SVCD0001\r\n", bootdrv);
  fclose(fd);

  /****************
   * AUTOEXEC.BAT *
   ****************/
  snprintf(buff, sizeof(buff), "%c:\\TEMP\\AUTOEXEC.BAT", bootdrv);
  fd = fopen(buff, "wb");
  if (fd == NULL) {
    return;
  } else {
    char *autoexec_bat1 =
      "@ECHO OFF\r\n"
      "SET TEMP=#:\\TEMP\r\n"
      "SET DOSDIR=#:\\SVARDOS\r\n"
      "SET NLSPATH=%DOSDIR%\\NLS\r\n"
      "SET DIRCMD=/O/P\r\n"
      "SET WATTCP.CFG=%DOSDIR%\r\n"
      "PATH %DOSDIR%\r\n"
      "PROMPT $P$G\r\n"
      "\r\n"
      "REM enable CPU power saving\r\n"
      "FDAPM ADV:REG\r\n"
      "\r\n";
    char *autoexec_bat2 =
      "REM Uncomment the line below for CDROM support\r\n"
      "REM SHSUCDX /d:SVCD0001\r\n"
      "\r\n"
      "ECHO.\r\n";

    /* replace all '#' occurences by bootdrive */
    strtr(autoexec_bat1, '#', bootdrv);

    /* write all to file */
    fputs(autoexec_bat1, fd);
    /* insert provox initialization if to be installed */
    if (fileexists("PROVOX.SVP")) {
      fprintf(fd, "%c:\\DRIVERS\\PROVOX\\PV7.EXE BNS\r\n", bootdrv);
    }
    genlocalesconf(fd, &locales);
    fputs(autoexec_bat2, fd);

    fprintf(fd, "ECHO %s\r\n", svarlang_strid(0x0600)); /* "Welcome to SvarDOS!" */
    fclose(fd);
  }

  /*** CREATE DIRECTORY FOR CONFIGURATION FILES ***/
  snprintf(buff, sizeof(buff), "%c:\\SVARDOS", bootdrv);
  mkdir(buff);

  /****************
   * PKG.CFG      *
   ****************/
  snprintf(buff, sizeof(buff), "%c:\\SVARDOS\\PKG.CFG", bootdrv);
  fd = fopen(buff, "wb");
  if (fd == NULL) {
    return;
  } else {
    char *pkg_cfg =
      "# pkg config file - specifies locations where packages should be installed\r\n"
      "\r\n"
      "# System boot drive\r\n"
      "BOOTDRIVE @\r\n"
      "\r\n"
      "# DOS core binaries\r\n"
      "DIR BIN @:\\SVARDOS\r\n"
      "\r\n"
      "# Programs\r\n"
      "DIR PROGS @:\\\r\n"
      "\r\n"
      "# Games \r\n"
      "DIR GAMES @:\\\r\n"
      "\r\n"
      "# Drivers\r\n"
      "DIR DRIVERS @:\\DRIVERS\r\n"
      "\r\n"
      "# Development tools\r\n"
      "DIR DEVEL @:\\DEVEL\r\n";

    /* replace all @ by the actual boot drive */
    strtr(pkg_cfg, '@', bootdrv);

    /* write to file */
    fputs(pkg_cfg, fd);
    fclose(fd);
  }

  /****************
   * PICOTCP      *
   ****************/
  /* TODO (or not? maybe not that useful) */

  /****************
   * WATTCP.CFG   *
   ****************/
  snprintf(buff, sizeof(buff), "%c:\\SVARDOS\\WATTCP.CFG", bootdrv);
  fd = fopen(buff, "wb");
  if (fd == NULL) return;
  fprintf(fd, "my_ip = dhcp\r\n"
              "#my_ip = 192.168.0.7\r\n"
              "#netmask = 255.255.255.0\r\n"
              "#nameserver = 192.168.0.1\r\n"
              "#nameserver = 192.168.0.2\r\n"
              "#gateway = 192.168.0.1\r\n");
  fclose(fd);

  /****************
   * POSTINST.BAT *
   ****************/
  /* create the postinst.bat file for actual installation of packages */
  fd = fopen("POSTINST.BAT", "wb");
  if (fd == NULL) return;
  fprintf(fd,
    "@ECHO OFF\r\n"
    "SET DOSDIR=%c:\\SVARDOS\r\n"
    "PATH %%DOSDIR%%\r\n"
    "ECHO INSTALLING PACKAGES\r\n"
    "COPY \\COMMAND.COM \\CMD.COM\r\n" /* move COMMAND.COM so it does not clashes with the installation of the SVARCOM package */
    "SET COMSPEC=%c:\\CMD.COM\r\n"
    "DEL \\AUTOEXEC.BAT\r\n"
    "COPY AUTOEXEC.BAT \\\r\n"
    "DEL AUTOEXEC.BAT\r\n"
    "DEL ZLOCALES.DAT\r\n"
    "DEL \\COMMAND.COM\r\n"
    "DEL \\KERNEL.SYS\r\n" /* KERNEL.SYS will be installed from the package in a moment */
    "FOR %%%%P IN (*.SVP) DO PKG INOWARN %%%%P\r\n" /* install packages (with warnings muted) */
    "DEL *.SVP\r\n", bootdrv, bootdrv);

  /* restore COMSPEC and do some cleanup */
  fprintf(fd, "DEL pkg.exe\r\n"
              "DEL install.com\r\n"
              "DEL install.lng\r\n"
              "SET COMSPEC=\\COMMAND.COM\r\n"
              "DEL \\CMD.COM\r\n");
  /* load codepage, now that MODE is installed */
  genlocalesconf(fd, &locales);

  /* if provox has been installed, then load it now */
  fprintf(fd, "IF NOT EXIST \\DRIVERS\\PROVOX\\PROVOX7.EXE GOTO SKIPPROVOX\r\n"
              "CLS\r\n"
              "\\DRIVERS\\PROVOX\\PROVOX7.EXE\r\n"
              "\\DRIVERS\\PROVOX\\PV7.EXE INIT BNS > NUL\r\n"
              ":SKIPPROVOX\r\n");

  /* print out the "installation over" message */
  fprintf(fd, "ECHO.\r\n"
              "ECHO ");
  fprintf(fd, svarlang_strid(0x0502)); /* "SvarDOS installation is over. Please restart your computer now" */
  fprintf(fd, "\r\n"
              "ECHO.\r\n");
  fclose(fd);

}


static int copypackages(char drvletter, const struct slocales *locales) {
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
    infomsg(10, 30, "ERROR: INSTALL.LST NOT FOUND", COLOR_BODY, 80);
    mdr_dos_getkey();
    return(-1);
  }
  pkglistflen = fread(pkglist, 1, sizeof(pkglist) - 2, fd);
  fclose(fd);
  if (pkglistflen == sizeof(pkglist) - 2) {
    infomsg(10, 30, "ERROR: INSTALL.LST TOO LARGE", COLOR_BODY, 80);
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

  /* copy pkg.exe, install.com and install.lng to the new drive, along with all packages */
  snprintf(buff, sizeof(buff), "%c:\\TEMP\\PKG.EXE", drvletter);
  fcopy(buff, buff + 8, buff, sizeof(buff));
  snprintf(buff, sizeof(buff), "%c:\\TEMP\\INSTALL.COM", drvletter);
  fcopy(buff, buff + 8, buff, sizeof(buff));
  snprintf(buff, sizeof(buff), "%c:\\TEMP\\INSTALL.LNG", drvletter);
  fcopy(buff, buff + 8, buff, sizeof(buff));

  /* copy the PROVOX driver, if present */
  if (fileexists("provox.exe")) {
    snprintf(buff, sizeof(buff), "%c:\\TEMP\\PROVOX.EXE", drvletter);
    fcopy(buff, buff + 8, buff, sizeof(buff));
    snprintf(buff, sizeof(buff), "%c:\\TEMP\\PV.EXE", drvletter);
    fcopy(buff, buff + 8, buff, sizeof(buff));
  }

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

    /* copy the package */
    snprintf(buff, sizeof(buff), svarlang_strid(0x0400), i+1, pkglistlen, pkgptr); /* "Copying package %d/%d: %s" */
    strcat(buff, "       ");
    infomsg(10, 1, buff, COLOR_BODY, 80);

    /* proceed with package copy */
    sprintf(buff, "%c:\\TEMP\\%s.svp", drvletter, pkgptr);
    if (fcopy(buff, buff + 8, buff, sizeof(buff)) != 0) {
      infomsg(10, 30, "READ ERROR", COLOR_BODY, 80);
      mdr_dos_getkey();
      return(-1);
    }
    /* jump to next entry or end of list and zero out the pkg name in the process */
    while ((*pkgptr != 0) && (*pkgptr != 0xff)) {
      *pkgptr = 0;
      pkgptr++;
    }
  }

  /* dump locales into ZLOCALES.DAT so the 2nd stage install can get them back */
  snprintf(buff, sizeof(buff), "%c:\\TEMP\\ZLOCALES.DAT", drvletter);
  fd = fopen(buff, "wb");
  if (fd == NULL) return(-1);
  fwrite(locales, sizeof(struct slocales), 1, fd);
  fclose(fd);

  /* prepare a dummy autoexec.bat that will exec install and call temp\postinst.bat */
  snprintf(buff, sizeof(buff), "%c:\\autoexec.bat", drvletter);
  fd = fopen(buff, "wb");
  if (fd == NULL) return(-1);
  fprintf(fd, "@ECHO OFF\r\n"
              "CD TEMP\r\n"
              "IF EXIST PROVOX.EXE PROVOX.EXE\r\n"
              "IF EXIST PV.EXE PV BNS\r\n"
              "install\r\n"   /* installer will run in 2nd stage (generating autoexec.bat, pkg.cfg and stuff) */
              "postinst.bat\r\n");
  fclose(fd);

  return(0);
}


static void finalreboot(void) {
  int y = 9;
  newscreen(2);
  y += putstringnls(y, 1, COLOR_BODY, 5, 0); /* "Your computer will reboot now." */
  y += putstringnls(y, 1, COLOR_BODYWARN, 5, 1); /* Please remove the installation disk from your drive" */
  putstringnls(++y, 1, COLOR_BODY, 0, 5); /* "Press any key..." */
  mdr_dos_getkey();
  reboot();
}


static void reinit_ega(void);
#pragma aux reinit_ega = \
"mov ah, 0x0f" /* get current video mode */ \
"int 0x10"     /* al contains the current video mode now */ \
"or al, 128"   /* set AL's high bit so BIOS does not flush VRAM (EGA+) */ \
"xor ah, ah"   /* re-set video mode (to whatever is set in AL) */ \
"int 0x10" \
modify [ax bx]


static void loadcp(const struct slocales *locales) {
  char buff[64];
  if (locales->codepage == 437) return;
  mdr_cout_locate(1, 0);
  if (locales->egafile == 1) {
    snprintf(buff, sizeof(buff), "MODE CON CP PREP=((%u) EGA.CPX) > NUL", locales->codepage);
  } else {
    snprintf(buff, sizeof(buff), "MODE CON CP PREP=((%u) EGA%d.CPX) > NUL", locales->codepage, locales->egafile);
  }
  exec(buff);
  snprintf(buff, sizeof(buff), "MODE CON CP SEL=%u > NUL", locales->codepage);
  exec(buff);
  /* below I re-init the video controller - apparently this is required if
   * I want the new glyph symbols to be actually applied, at least some
   * (broken?) BIOSes, like VBox, apply glyphs only at next video mode change */
  reinit_ega();
}


int main(void) {
  struct slocales locales;
  int targetdrv;
  int action;

  /* setup an internal int 24h handler ("always fail") so DOS does not output
   * the ugly "abort, retry, fail" messages */
  install_int24();

  /* init screen and detect mono adapters */
  if (mdr_cout_init(NULL, NULL) == 0) {
    /* overload color scheme with mono settings */
    COLOR_TITLEBAR = 0x70;
    COLOR_TITLEVER = 0x70;
    COLOR_BODY = 0x07;
    COLOR_BODYWARN = 0x07;
    COLOR_SELECT = 0x70;
    COLOR_SELECTCUR = 0x07;
  }

  /* is it stage 2 of the installation? */
  if (fileexists("ZLOCALES.DAT")) goto GENCONF;

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
      mov bx, res
      mov [bx], byte ptr 0
      good:

      pop dx
      pop cx
    }

    memcpy(BUILDSTRING, res, 12);
  }

 SelectLang:
  action = selectlang(&locales); /* welcome to svardos, select your language */
  if (action != MENUNEXT) goto QUIT;
  loadcp(&locales);
  svarlang_load("INSTALL.LNG", locales.lang); /* NLS support */

  action = selectkeyb(&locales);  /* what keyb layout should we use? */
  if (action == MENUQUIT) goto QUIT;
  if (action == MENUPREV) goto SelectLang;

 WelcomeScreen:
  action = welcomescreen(); /* what svardos is, ask whether to run live dos or install */
  if (action == MENUQUIT) goto QUIT;
  if (action == MENUPREV) goto SelectLang;

 SelDriveScreen:
  targetdrv = selectdrive();
  if (targetdrv == MENUQUIT) goto QUIT;
  if (targetdrv == MENUPREV) goto WelcomeScreen;

  action = preparedrive(targetdrv); /* what drive should we install to? check avail. space */
  if (action == MENUQUIT) goto QUIT;
  if (action == MENUPREV) goto SelDriveScreen;
  targetdrv = (targetdrv & 0xff) + 'A'; /* convert the part+hd+drv value to a DOS letter */

  /* copy packages to dst drive */
  if (copypackages(targetdrv, &locales) != 0) goto QUIT;
  finalreboot(); /* remove the install medium and reboot */

  goto QUIT;

 GENCONF: /* second stage of the installation (run from the destination disk) */
  bootfilesgen(); /* generate boot files and other configurations */

 QUIT:
  mdr_cout_locate(0, 0);
  mdr_cout_close();
  return(0);
}
