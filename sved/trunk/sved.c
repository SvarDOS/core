/* Sved, the SvarDOS editor
 *
 * Copyright (C) 2023 Mateusz Viste
 *
 * Sved is released under the terms of the MIT license.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include <dos.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>   /* _fcalloc() */

#include "mdr\bios.h"
#include "mdr\cout.h"
#include "mdr\dos.h"
#include "mdr\keyb.h"

#include "svarlang\svarlang.h"

#define COL_TXT        0
#define COL_STATUSBAR1 1
#define COL_STATUSBAR2 2
#define COL_SCROLLBAR  3
#define COL_MSG        4
#define COL_ERR        5
/* preload the mono scheme (to be overloaded at runtime if color adapter present) */
static unsigned char scheme[] = {0x07, 0x70, 0x70, 0x70, 0x70, 0x70};

#define SCROLL_CURSOR 0xB1


struct line {
  struct line far *prev;
  struct line far *next;
  unsigned short len;
  char payload[1];
};

struct file {
  int fd;
  struct line far *cursor;
  unsigned short xoffset;
  unsigned char cursorposx;
  unsigned char cursorposy;
  char lfonly; /* set if line endings are LF (CR/LF otherwise) */
  char fname[1]; /* dynamically sized */
};


/* returns non-zero on error */
static int line_add(struct file *db, const char far *line) {
  unsigned short slen;
  struct line far *l;

  /* slen = strlen(line) (but for far pointer) */
  for (slen = 0; line[slen] != 0; slen++);

  /* trim out CR/LF line endings */
  if ((slen >= 2) && (line[slen - 2] == '\r')) {
    slen -= 2;
  } else if ((slen >= 1) && (line[slen - 1] == '\n')) {
    slen--;
  }

  l = _fcalloc(1, sizeof(struct line) + slen + 1);
  if (l == NULL) return(-1);

  l->prev = db->cursor;
  if (db->cursor) {
    l->next = db->cursor->next;
    db->cursor->next = l;
    l->next->prev = l;
  }
  db->cursor = l;
  _fmemcpy(l->payload, line, slen);
  l->len = slen;

  return(0);
}


static void db_rewind(struct file *db) {
  if (db->cursor == NULL) return;
  while (db->cursor->prev) db->cursor = db->cursor->prev;
}


static void load_colorscheme(void) {
  scheme[COL_TXT] = 0x17;
  scheme[COL_STATUSBAR1] = 0x70;
  scheme[COL_STATUSBAR2] = 0x78;
  scheme[COL_SCROLLBAR] = 0x70;
  scheme[COL_MSG] = 0x2f;
  scheme[COL_ERR] = 0x4f;
}


static void ui_basic(unsigned char screenw, unsigned char screenh, const struct file *db) {
  unsigned char i;
  const char *s = svarlang_strid(0); /* HELP */
  unsigned char helpcol = screenw - (strlen(s) + 4);

  /* fill status bar with background */
  mdr_cout_char_rep(screenh - 1, 0, ' ', scheme[COL_STATUSBAR1], screenw);

  /* filename */
  if (db->fname[0] == 0) {
    mdr_cout_str(screenh - 1, 0, svarlang_str(0, 1), scheme[COL_STATUSBAR1], screenw);
  } else {
    mdr_cout_str(screenh - 1, 0, db->fname, scheme[COL_STATUSBAR1], screenw);
  }

  /* eol type */
  if (db->lfonly) {
    mdr_cout_str(screenh - 1, helpcol - 3, "LF", scheme[COL_STATUSBAR1], 5);
  } else {
    mdr_cout_str(screenh - 1, helpcol - 5, "CRLF", scheme[COL_STATUSBAR1], 5);
  }

  mdr_cout_str(screenh - 1, helpcol, " F1=", scheme[COL_STATUSBAR2], 40);
  mdr_cout_str(screenh - 1, helpcol + 4, s, scheme[COL_STATUSBAR2], 40);

  /* scroll bar */
  for (i = 0; i < (screenh - 1); i++) {
    mdr_cout_char(i, screenw - 1, SCROLL_CURSOR, scheme[COL_SCROLLBAR]);
  }
}


static void ui_msg(const char *msg, unsigned char screenw, unsigned char screenh, unsigned char *uidirtyfrom, unsigned char *uidirtyto, unsigned char attr) {
  unsigned short x, y, msglen, i;
  msglen = strlen(msg);
  y = (screenh - 4) >> 1;
  x = (screenw - msglen - 4) >> 1;
  for (i = y+2; i >= y; i--) mdr_cout_char_rep(i, x, ' ', attr, msglen + 2);
  mdr_cout_str(y+1, x+1, msg, attr, msglen);

  if (*uidirtyfrom > y) *uidirtyfrom = y;
  if (*uidirtyto < y+2) *uidirtyto = y+2;
}


static void ui_help(unsigned char screenw) {
#define MAXLINLEN 35
  unsigned short i, offset;
  offset = (screenw - MAXLINLEN + 1) >> 1;
  mdr_cout_cursor_hide();
  for (i = 2; i <= 12; i++) {
    mdr_cout_char_rep(i, offset - 2, ' ', scheme[COL_STATUSBAR1], MAXLINLEN);
  }

  mdr_cout_str(3, offset, svarlang_str(0, 0), scheme[COL_STATUSBAR1], MAXLINLEN);
  for (i = 0; i <= 2; i++) {
    mdr_cout_str(5 + i, offset, svarlang_str(8, i), scheme[COL_STATUSBAR1], MAXLINLEN);
  }
  mdr_cout_str(9, offset, svarlang_str(8, 10), scheme[COL_STATUSBAR1], MAXLINLEN);
  mdr_cout_str(11, offset, svarlang_str(8, 11), scheme[COL_STATUSBAR1], MAXLINLEN);

  keyb_getkey();
  mdr_cout_cursor_show();
#undef MAXLINLEN
}


static void ui_refresh(const struct file *db, unsigned char screenw, unsigned char screenh, unsigned char uidirtyfrom, unsigned char uidirtyto) {
  unsigned char x;
  const struct line far *l;
  unsigned char y = db->cursorposy;

#ifdef DBG_REFRESH
  static char m = 'a';
  m++;
  if (m > 'z') m = 'a';
#endif

  /* rewind cursor line to first line that needs redrawing */
  for (l = db->cursor; y > uidirtyfrom; y--) l = l->prev;

  /* iterate over lines and redraw whatever needs to be redrawn */
  for (; l != NULL; l = l->next, y++) {

    /* skip lines that do not need to be refreshed */
    if (y < uidirtyfrom) continue;
    if (y > uidirtyto) break;

    x = 0;
    if (db->xoffset < l->len) {
      unsigned char i, limit;
      if (l->len - db->xoffset < screenw) {
        limit = l->len;
      } else {
        limit = db->xoffset + screenw - 1;
      }
      for (i = db->xoffset; i < limit; i++) mdr_cout_char(y, x++, l->payload[i], scheme[COL_TXT]);
    }

    /* write empty spaces until end of line */
    if (x < screenw - 1) mdr_cout_char_rep(y, x, ' ', scheme[COL_TXT], screenw - 1 - x);

#ifdef DBG_REFRESH
    mdr_cout_char(y, 0, m, scheme[COL_STATUSBAR1]);
#endif

    if (y == screenh - 2) break;
  }

  /* fill all lines below if empty (and they need to be redrawn) */
  if (l == NULL) {
    while ((y < screenh - 1) && (y < uidirtyto)) {
      mdr_cout_char_rep(y++, 0, ' ', scheme[COL_TXT], screenw - 1);
    }
  }
}


static void check_cursor_not_after_eol(struct file *db, unsigned char *uidirtyfrom, unsigned char *uidirtyto) {
  if (db->xoffset + db->cursorposx <= db->cursor->len) return;

  if (db->cursor->len < db->xoffset) {
    db->cursorposx = 0;
    db->xoffset = db->cursor->len;
    *uidirtyfrom = 0;
    *uidirtyto = 0xff;
  } else {
    db->cursorposx = db->cursor->len - db->xoffset;
  }
}


static void cursor_up(struct file *db, unsigned char *uidirtyfrom, unsigned char *uidirtyto) {
  if (db->cursor->prev != NULL) {
    db->cursor = db->cursor->prev;
    if (db->cursorposy == 0) {
      *uidirtyfrom = 0;
      *uidirtyto = 0xff;
    } else {
      db->cursorposy -= 1;
    }
  }
}


static void cursor_eol(struct file *db, unsigned char screenw, unsigned char *uidirtyfrom, unsigned char *uidirtyto) {
  /* adjust xoffset to make sure eol is visible on screen */
  if (db->xoffset > db->cursor->len) {
    db->xoffset = db->cursor->len - 1;
    *uidirtyfrom = 0;
    *uidirtyto = 0xff;
  }

  if (db->xoffset + screenw - 1 <= db->cursor->len) {
    db->xoffset = db->cursor->len - screenw + 2;
    *uidirtyfrom = 0;
    *uidirtyto = 0xff;
  }
  db->cursorposx = db->cursor->len - db->xoffset;
}


static void cursor_down(struct file *db, unsigned char screenh, unsigned char *uidirtyfrom, unsigned char *uidirtyto) {
  if (db->cursor->next != NULL) {
    db->cursor = db->cursor->next;
    if (db->cursorposy < screenh - 2) {
      db->cursorposy += 1;
    } else {
      *uidirtyfrom = 0;
      *uidirtyto = 0xff;
    }
  }
}


static void cursor_left(struct file *db, unsigned char screenw, unsigned char *uidirtyfrom, unsigned char *uidirtyto) {
  if (db->cursorposx > 0) {
    db->cursorposx -= 1;
  } else if (db->xoffset > 0) {
    db->xoffset -= 1;
    *uidirtyfrom = 0;
    *uidirtyto = 0xff;
  } else if (db->cursor->prev != NULL) { /* jump to end of line above */
    cursor_up(db, uidirtyfrom, uidirtyto);
    cursor_eol(db, screenw, uidirtyfrom, uidirtyto);
  }
}


static void cursor_home(struct file *db, unsigned char *uidirtyfrom, unsigned char *uidirtyto) {
  db->cursorposx = 0;
  if (db->xoffset != 0) {
    db->xoffset = 0;
    *uidirtyfrom = 0;
    *uidirtyto = 0xff;
  }
}


static void cursor_right(struct file *db, unsigned char screenw, unsigned char screenh, unsigned char *uidirtyfrom, unsigned char *uidirtyto) {
  if (db->cursor->len > db->xoffset + db->cursorposx) {
    if (db->cursorposx < screenw - 2) {
      db->cursorposx += 1;
    } else {
      db->xoffset += 1;
      *uidirtyfrom = 0;
      *uidirtyto = 0xff;
    }
  } else {
    cursor_down(db, screenh, uidirtyfrom, uidirtyto);
    cursor_home(db, uidirtyfrom, uidirtyto);
  }
}


static void del(struct file *db, unsigned char *uidirtyfrom, unsigned char *uidirtyto) {
  if (db->cursorposx + db->xoffset < db->cursor->len) {
    _fmemmove(db->cursor->payload + db->cursorposx + db->xoffset, db->cursor->payload + db->cursorposx + db->xoffset + 1, db->cursor->len - db->cursorposx - db->xoffset);
    db->cursor->len -= 1; /* do this AFTER memmove so the copy includes the nul terminator */
    *uidirtyfrom = db->cursorposy;
    *uidirtyto = db->cursorposy;
  } else if (db->cursor->next != NULL) { /* cursor is at end of line: merge current line with next one (if there is a next one) */
    struct line far *nextline = db->cursor->next;
    if (db->cursor->next->len > 0) {
      void far *newptr = _frealloc(db->cursor, sizeof(struct line) + db->cursor->len + db->cursor->next->len + 1);
      if (newptr != NULL) {
        db->cursor = newptr;
        _fmemcpy(db->cursor->payload + db->cursor->len, db->cursor->next->payload, db->cursor->next->len + 1);
        db->cursor->len += db->cursor->next->len;
      }
    }
    db->cursor->next = db->cursor->next->next;
    db->cursor->next->prev = db->cursor;
    if (db->cursor->prev != NULL) db->cursor->prev->next = db->cursor; /* in case realloc changed my pointer */
    _ffree(nextline);
    *uidirtyfrom = db->cursorposy;
    *uidirtyto = 0xff;
  }
}


static void bkspc(struct file *db, unsigned char screenw, unsigned char *uidirtyfrom, unsigned char *uidirtyto) {

  /* backspace is basically "left + del", not applicable only if cursor is on 1st byte of the file */
  if ((db->cursorposx == 0) && (db->xoffset == 0) && (db->cursor->prev == NULL)) return;

  cursor_left(db, screenw, uidirtyfrom, uidirtyto);
  del(db, uidirtyfrom, uidirtyto);
}


/* a custom argv-parsing routine that looks directly inside the PSP, avoids the need
 * of argc and argv, saves some 330 bytes of binary size */
static char *parseargv(void) {
  char *tail = (void *)0x81; /* THIS WORKS ONLY IN SMALL MEMORY MODEL */
  unsigned char count = 0;
  char *argv[4];

  while (count < 4) {
    /* jump to nearest arg */
    while (*tail == ' ') {
      *tail = 0;
      tail++;
    }

    if (*tail == '\r') {
      *tail = 0;
      break;
    }

    argv[count++] = tail;

    /* jump to next delimiter */
    while ((*tail != ' ') && (*tail != '\r')) tail++;
  }

  /* check args now */
  if (count == 0) return("");

  return(argv[0]);
}


static struct file *loadfile(const char *fname) {
  char buff[1024];
  unsigned int prevlen = 0, len, llen;
  int fd;
  struct file *db;

  len = strlen(fname) + 1;
  db = calloc(1, sizeof(struct file) + len);
  if (db == NULL) return(NULL);
  memcpy(db->fname, fname, len);

  if (*fname == 0) goto SKIPLOADING;

  if (_dos_open(fname, O_RDONLY, &fd) != 0) {
    mdr_coutraw_puts("Failed to open file:");
    mdr_coutraw_puts(fname);
    free(db);
    return(NULL);
  }

  db->lfonly = 1;

  do {
    if (_dos_read(fd, buff + prevlen, sizeof(buff) - prevlen, &len) == 0) {
      len += prevlen;
    } else {
      len = prevlen;
    }

    /* look for nearest \n and replace with 0, also expand tabs */
    for (llen = 0; buff[llen] != '\n'; llen++) {
      if (buff[llen] == '\t') {
        unsigned char c;
        for (c = 0; c < 8; c++) buff[llen++] = ' ';
      }
      if (llen == sizeof(buff)) { /* TODO line too long: handle it in some classy way */
        break;
      }
    }
    buff[llen] = 0;
    if ((llen > 0) && (buff[llen - 1] == '\r')) {
      buff[llen - 1] = 0; /* trim \r if line ending is cr/lf */
      db->lfonly = 0;
    }
    if (line_add(db, buff) != 0) {
      mdr_coutraw_puts("out of memory");
      free(db);
      db = NULL;
      break;
    }

    len -= llen + 1;
    memmove(buff, buff + llen + 1, len);
    prevlen = len;
  } while (len > 0);

  _dos_close(fd);

  SKIPLOADING:

  /* add an empty line at end if not present already, also rewind cursor to top of file */
  if (db != NULL) {
    if ((db->cursor == NULL) || (db->cursor->len != 0)) line_add(db, "");
    db_rewind(db);
  }

  return(db);
}


static int savefile(const struct file *db) {
  int fd;
  const struct line far *l;
  unsigned bytes;
  unsigned char eollen;
  unsigned char eolbuf[2];

  if (_dos_open(db->fname, O_WRONLY, &fd) != 0) {
    return(-1);
  }

  l = db->cursor;
  while (l->prev) l = l->prev;

  /* preset line terminators */
  if (db->lfonly) {
    eolbuf[0] = '\n';
    eollen = 1;
  } else {
    eolbuf[0] = '\r';
    eolbuf[1] = '\n';
    eollen = 2;
  }

  while (l) {
    _dos_write(fd, l->payload, l->len, &bytes);
    _dos_write(fd, eolbuf, eollen, &bytes);
    l = l->next;
  }

  _dos_close(fd);
  return(0);
}


static void insert_in_line(struct file *db, const char *databuf, unsigned short len, unsigned char screenw, unsigned char screenh, unsigned char *uidirtyfrom, unsigned char *uidirtyto) {
  struct line far *n;
  n = _frealloc(db->cursor, sizeof(struct line) + db->cursor->len + len);
  if (n != NULL) {
    unsigned short off = db->xoffset + db->cursorposx;
    if (n->prev) n->prev->next = n;
    if (n->next) n->next->prev = n;
    db->cursor = n;
    _fmemmove(db->cursor->payload + off + len, db->cursor->payload + off, db->cursor->len - off + 1);
    db->cursor->len += len;
    *uidirtyfrom = db->cursorposy;
    *uidirtyto = db->cursorposy;
    while (len--) {
      db->cursor->payload[off++] = *databuf;
      databuf++;
      cursor_right(db, screenw, screenh, uidirtyfrom, uidirtyto);
    }
  }
}


int main(void) {
  const char *fname;
  struct file *db;
  unsigned char screenw = 0, screenh = 0;
  unsigned char uidirtyfrom = 0, uidirtyto = 0xff; /* make sure to redraw entire UI at first run */

  {
    char nlspath[128], lang[8];
    svarlang_autoload_pathlist("sved", mdr_dos_getenv(nlspath, "NLSPATH", sizeof(nlspath)), mdr_dos_getenv(lang, "LANG", sizeof(lang)));
  }

  fname = parseargv();

  if (fname == NULL) {
    mdr_coutraw_puts(svarlang_str(1,0)); /* usage: sved file.txt */
    return(0);
  }

  /* load file */
  db = loadfile(fname);
  if (db == NULL) return(1);

  if (mdr_cout_init(&screenw, &screenh)) load_colorscheme();
  ui_basic(screenw, screenh, db);

  for (;;) {
    int k;

    check_cursor_not_after_eol(db, &uidirtyfrom, &uidirtyto);
    mdr_cout_locate(db->cursorposy, db->cursorposx);

    if (uidirtyfrom != 0xff) {
      ui_refresh(db, screenw, screenh, uidirtyfrom, uidirtyto);
      uidirtyfrom = 0xff;
    }

    k = keyb_getkey();

    if (k == 0x150) { /* down */
      cursor_down(db, screenh, &uidirtyfrom, &uidirtyto);

    } else if (k == 0x148) { /* up */
      cursor_up(db, &uidirtyfrom, &uidirtyto);

    } else if (k == 0x14D) { /* right */
      cursor_right(db, screenw, screenh, &uidirtyfrom, &uidirtyto);

    } else if (k == 0x14B) { /* left */
      cursor_left(db, screenw, &uidirtyfrom, &uidirtyto);

    } else if (k == 0x149) { /* pgup */
      // TODO

    } else if (k == 0x151) { /* pgdown */
      // TODO

    } else if (k == 0x147) { /* home */
       cursor_home(db, &uidirtyfrom, &uidirtyto);

    } else if (k == 0x14F) { /* end */
       cursor_eol(db, screenw, &uidirtyfrom, &uidirtyto);

    } else if (k == 0x1B) { /* ESC */
      break;

    } else if (k == 0x0D) { /* ENTER */
      /* add a new line */
      if (line_add(db, db->cursor->payload + db->xoffset + db->cursorposx) == 0) {
        /* trim the line above */
        db->cursor->prev->len = db->xoffset + db->cursorposx;
        db->cursor->prev->payload[db->cursor->prev->len] = 0;
        /* move cursor to the (new) line below */
        db->cursorposx = 0;
        if (db->cursorposy < screenh - 2) {
          uidirtyfrom = db->cursorposy;
          db->cursorposy++;
        } else {
          uidirtyfrom = 0;
        }
        uidirtyto = 0xff;
      } else {
        /* ERROR: OUT OF MEMORY */
      }

    } else if (k == 0x153) {  /* DEL */
      del(db, &uidirtyfrom, &uidirtyto);

    } else if (k == 0x008) { /* BKSPC */
      bkspc(db, screenw, &uidirtyfrom, &uidirtyto);

    } else if ((k >= 0x20) && (k <= 0xff)) { /* "normal" character */
      char c = k;
      insert_in_line(db, &c, 1, screenw, screenh, &uidirtyfrom, &uidirtyto);

    } else if (k == 0x009) { /* TAB */
      const char *tab = "        ";
      insert_in_line(db, tab, 8, screenw, screenh, &uidirtyfrom, &uidirtyto);

    } else if (k == 0x13b) { /* F1 */
      ui_help(screenw);
      uidirtyfrom = 0;
      uidirtyto = 0xff;

    } else if (k == 0x13f) { /* F5 */
      if (savefile(db) == 0) {
        ui_msg(svarlang_str(0, 2), screenw, screenh, &uidirtyfrom, &uidirtyto, scheme[COL_MSG]);
        mdr_bios_tickswait(11); /* 11 ticks is about 600 ms */
      } else {
        ui_msg(svarlang_str(0, 3), screenw, screenh, &uidirtyfrom, &uidirtyto, scheme[COL_ERR]);
        mdr_bios_tickswait(36); /* 2s */
      }

    } else if (k == 0x144) { /* F10 */
      db->lfonly ^= 1;
      ui_basic(screenw, screenh, db);

    } else { /* UNHANDLED KEY - TODO IGNORE THIS IN PRODUCTION RELEASE */
      char buff[4];
      const char *HEX = "0123456789ABCDEF";
      buff[0] = HEX[(k >> 8) & 15];
      buff[1] = HEX[(k >> 4) & 15];
      buff[2] = HEX[k & 15];
      mdr_cout_str(screenh - 1, 0, "UNHANDLED KEY: 0x", scheme[COL_STATUSBAR1], 17);
      mdr_cout_str(screenh - 1, 17, buff, scheme[COL_STATUSBAR1], 3);
      keyb_getkey();
      break;
    }
  }

  mdr_cout_close();

  /* no need to free memory, DOS will do it for me */

  return(0);
}
