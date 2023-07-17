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

#include "mdr\cout.h"
#include "mdr\dos.h"
#include "mdr\keyb.h"

#include "svarlang\svarlang.h"

#define COL_TXT        0
#define COL_STATUSBAR1 1
#define COL_STATUSBAR2 2
#define COL_SCROLLBAR  3
/* preload the mono scheme (to be overloaded at runtime if color adapter present) */
static unsigned char scheme[] = {0x07, 0x70, 0x70, 0x70};

#define SCROLL_CURSOR 0xB1


struct line {
  struct line far *prev;
  struct line far *next;
  unsigned short len;
  char payload[1];
};

struct linedb {
  struct line far *topscreen;
  struct line far *cursor;
  unsigned short xoffset;
};


/* returns non-zero on error */
static int line_add(struct linedb *db, const char far *line) {
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


static void db_rewind(struct linedb *db) {
  if (db->cursor == NULL) return;
  while (db->cursor->prev) db->cursor = db->cursor->prev;
  db->topscreen = db->cursor;
}


static void load_colorscheme(void) {
  scheme[COL_TXT] = 0x17;
  scheme[COL_STATUSBAR1] = 0x70;
  scheme[COL_STATUSBAR2] = 0x78;
  scheme[COL_SCROLLBAR] = 0x70;
}


static void ui_basic(unsigned char screenw, unsigned char screenh, const char *fname) {
  unsigned char i;
  const char *s = svarlang_strid(0); /* HELP */
  unsigned char helpcol = screenw - (strlen(s) + 4);

  /* clear screen */
  mdr_cout_cls(scheme[COL_TXT]);

  /* status bar */
  for (i = 0; i < helpcol; i++) {
    mdr_cout_char(screenh - 1, i, *fname, scheme[COL_STATUSBAR1]);
    if (*fname != 0) fname++;
  }
  mdr_cout_str(screenh - 1, helpcol, " F1=", scheme[COL_STATUSBAR2], 40);
  mdr_cout_str(screenh - 1, helpcol + 4, s, scheme[COL_STATUSBAR2], 40);

  /* scroll bar */
  for (i = 0; i < (screenh - 1); i++) {
    mdr_cout_char(i, screenw - 1, SCROLL_CURSOR, scheme[COL_SCROLLBAR]);
  }
}


static void ui_refresh(const struct linedb *db, unsigned char screenw, unsigned char screenh, unsigned char uidirtyfrom, unsigned char uidirtyto) {
  unsigned char y = 0;
  unsigned char len;
  struct line far *l;

#ifdef DBG_REFRESH
  static char m = 'a';
  m++;
  if (m > 'z') m = 'a';
#endif

  for (l = db->topscreen; l != NULL; l = l->next, y++) {

    /* skip lines that do not to be refreshed */
    if (y < uidirtyfrom) continue;
    if (y > uidirtyto) break;

    if (db->xoffset < l->len) {
      for (len = 0; l->payload[len] != 0; len++) mdr_cout_char(y, len, l->payload[len], scheme[COL_TXT]);
    } else {
      len = 0;
    }
    while (len < screenw - 1) mdr_cout_char(y, len++, ' ', scheme[COL_TXT]);

#ifdef DBG_REFRESH
    mdr_cout_char(y, 0, m, scheme[COL_STATUSBAR1]);
#endif

    if (y == screenh - 2) break;
  }
}


static void check_cursor_not_after_eol(struct linedb *db, unsigned char *cursorpos, unsigned char *uidirtyfrom, unsigned char *uidirtyto) {
  if (db->xoffset + *cursorpos <= db->cursor->len) return;

  if (db->cursor->len < db->xoffset) {
    *cursorpos = 0;
    db->xoffset = db->cursor->len;
    *uidirtyfrom = 0;
    *uidirtyto = 0xff;
  } else {
    *cursorpos = db->cursor->len - db->xoffset;
  }
}


static void cursor_up(struct linedb *db, unsigned char *cursorposy, unsigned char *uidirtyfrom, unsigned char *uidirtyto) {
  if (db->cursor->prev != NULL) {
    db->cursor = db->cursor->prev;
    if (*cursorposy == 0) {
      db->topscreen = db->cursor;
      *uidirtyfrom = 0;
      *uidirtyto = 0xff;
    } else {
      *cursorposy -= 1;
    }
  }
}


static void cursor_eol(struct linedb *db, unsigned char *cursorposx, unsigned char screenw, unsigned char *uidirtyfrom, unsigned char *uidirtyto) {
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
  *cursorposx = db->cursor->len - db->xoffset;
}


static void cursor_down(struct linedb *db, unsigned char *cursorposy, unsigned char screenh, unsigned char *uidirtyfrom, unsigned char *uidirtyto) {
  if (db->cursor->next != NULL) {
    db->cursor = db->cursor->next;
    if (*cursorposy < screenh - 2) {
      *cursorposy += 1;
    } else {
      db->topscreen = db->topscreen->next;
      *uidirtyfrom = 0;
      *uidirtyto = 0xff;
    }
  }
}


static void cursor_left(struct linedb *db, unsigned char *cursorposx, unsigned char *cursorposy, unsigned char screenw, unsigned char *uidirtyfrom, unsigned char *uidirtyto) {
  if (*cursorposx > 0) {
    *cursorposx -= 1;
  } else if (db->xoffset > 0) {
    db->xoffset -= 1;
    *uidirtyfrom = 0;
    *uidirtyto = 0xff;
  } else if (db->cursor->prev != NULL) { /* jump to end of line above */
    cursor_up(db, cursorposy, uidirtyfrom, uidirtyto);
    cursor_eol(db, cursorposx, screenw, uidirtyfrom, uidirtyto);
  }
}


static void cursor_home(struct linedb *db, unsigned char *cursorposx, unsigned char *uidirtyfrom, unsigned char *uidirtyto) {
  *cursorposx = 0;
  if (db->xoffset != 0) {
    db->xoffset = 0;
    *uidirtyfrom = 0;
    *uidirtyto = 0xff;
  }
}


static void del(struct linedb *db, unsigned char cursorposx, unsigned char cursorposy, unsigned char *uidirtyfrom, unsigned char *uidirtyto) {
  if (cursorposx + db->xoffset < db->cursor->len) {
    _fmemmove(db->cursor->payload + cursorposx + db->xoffset, db->cursor->payload + cursorposx + db->xoffset + 1, db->cursor->len - cursorposx - db->xoffset);
    db->cursor->len -= 1; /* do this AFTER memmove so the copy includes the nul terminator */
    *uidirtyfrom = cursorposy;
    *uidirtyto = cursorposy;
  } else if (db->cursor->next != NULL) { /* cursor is at end of line: merge current line with next one (if there is a next one) */
    struct line far *nextline = db->cursor->next;
    if (db->cursor->next->len > 0) {
      void far *newptr = _frealloc(db->cursor, sizeof(struct line) + db->cursor->len + db->cursor->next->len + 1);
      if (newptr != NULL) {
        db->cursor = newptr;
        _fmemcpy(db->cursor->payload + db->cursor->len, db->cursor->next->payload, db->cursor->next->len + 1);
        db->cursor->len += db->cursor->next->len;
        /* update db->topscreen if needed */
        if (cursorposy == 0) db->topscreen = db->cursor;
      }
    }
    db->cursor->next = db->cursor->next->next;
    db->cursor->next->prev = db->cursor;
    if (db->cursor->prev != NULL) db->cursor->prev->next = db->cursor; /* in case realloc changed my pointer */
    _ffree(nextline);
    *uidirtyfrom = cursorposy;
    *uidirtyto = 0xff;
  }
}


static void bkspc(struct linedb *db, unsigned char *cursorposx, unsigned char *cursorposy, unsigned char screenw, unsigned char *uidirtyfrom, unsigned char *uidirtyto) {

  /* backspace is basically "left + del", not applicable only if cursor is on 1st byte of the file */
  if ((*cursorposx == 0) && (db->xoffset == 0) && (db->cursor->prev == NULL)) return;

  cursor_left(db, cursorposx, cursorposy, screenw, uidirtyfrom, uidirtyto);
  del(db, *cursorposx, *cursorposy, uidirtyfrom, uidirtyto);
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
  if (count != 1) return(NULL);

  return(argv[0]);
}


static int loadfile(struct linedb *db, const char *fname) {
  char buff[1024];
  unsigned int prevlen = 0, len, llen;
  int fd;
  int r = 0;

  if (_dos_open(fname, O_RDONLY, &fd) != 0) {
    mdr_coutraw_puts("Failed to open file:");
    mdr_coutraw_puts(fname);
    return(-1);
  }

  do {
    if (_dos_read(fd, buff + prevlen, sizeof(buff) - prevlen, &len) == 0) {
      len += prevlen;
    } else {
      len = prevlen;
    }

    /* look for nearest \n and replace with 0*/
    for (llen = 0; buff[llen] != '\n'; llen++) {
      if (llen == sizeof(buff)) break;
    }
    buff[llen] = 0;
    if ((llen > 0) && (buff[llen - 1])) buff[llen - 1] = 0; /* trim \r if line ending is cr/lf */
    if (line_add(db, buff) != 0) {
      mdr_coutraw_puts("out of memory");
      r = -1;
      break;
    }

    len -= llen + 1;
    memmove(buff, buff + llen + 1, len);
    prevlen = len;
  } while (len > 0);

  _dos_close(fd);

  return(r);
}


int main(void) {
  const char *fname;
  struct linedb db;
  unsigned char screenw = 0, screenh = 0;
  unsigned char cursorposx = 0, cursorposy = 0;
  unsigned char uidirtyfrom = 0, uidirtyto = 0xff; /* make sure to redraw entire UI at first run */

  bzero(&db, sizeof(db));

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
  if (loadfile(&db, fname) != 0) return(1);

  /* add an empty line at end if not present already */
  if (db.cursor->len != 0) line_add(&db, "");

  if (mdr_cout_init(&screenw, &screenh)) load_colorscheme();
  ui_basic(screenw, screenh, fname);

  db_rewind(&db);

  for (;;) {
    int k;

    check_cursor_not_after_eol(&db, &cursorposx, &uidirtyfrom, &uidirtyto);
    mdr_cout_locate(cursorposy, cursorposx);

    if (uidirtyfrom != 0xff) {
      ui_refresh(&db, screenw, screenh, uidirtyfrom, uidirtyto);
      uidirtyfrom = 0xff;
    }

    k = keyb_getkey();

    if (k == 0x150) { /* down */
      cursor_down(&db, &cursorposy, screenh, &uidirtyfrom, &uidirtyto);

    } else if (k == 0x148) { /* up */
      cursor_up(&db, &cursorposy, &uidirtyfrom, &uidirtyto);

    } else if (k == 0x14D) { /* right */
      if (db.cursor->len > db.xoffset + cursorposx) {
        if (cursorposx < screenw - 2) {
          cursorposx++;
        } else {
          db.xoffset++;
          uidirtyfrom = 0;
          uidirtyto = 0xff;
        }
      } else {
        cursor_down(&db, &cursorposy, screenh, &uidirtyfrom, &uidirtyto);
        cursor_home(&db, &cursorposx, &uidirtyfrom, &uidirtyto);
      }

    } else if (k == 0x14B) { /* left */
      cursor_left(&db, &cursorposx, &cursorposy, screenw, &uidirtyfrom, &uidirtyto);

    } else if (k == 0x149) { /* pgup */
      // TODO

    } else if (k == 0x151) { /* pgdown */
      // TODO

    } else if (k == 0x147) { /* home */
       cursor_home(&db, &cursorposx, &uidirtyfrom, &uidirtyto);

    } else if (k == 0x14F) { /* end */
       cursor_eol(&db, &cursorposx, screenw, &uidirtyfrom, &uidirtyto);

    } else if (k == 0x1B) { /* ESC */
      break;

    } else if (k == 0x0D) { /* ENTER */
      /* add a new line */
      if (line_add(&db, db.cursor->payload + db.xoffset + cursorposx) == 0) {
        /* trim the line above */
        db.cursor->prev->len = db.xoffset + cursorposx;
        db.cursor->prev->payload[db.cursor->prev->len] = 0;
        /* move cursor to the (new) line below */
        cursorposx = 0;
        if (cursorposy < screenh - 2) {
          uidirtyfrom = cursorposy;
          cursorposy++;
        } else {
          db.topscreen = db.topscreen->next;
          uidirtyfrom = 0;
        }
        uidirtyto = 0xff;
      } else {
        /* ERROR: OUT OF MEMORY */
      }

    } else if (k == 0x153) {  /* DEL */
      del(&db, cursorposx, cursorposy, &uidirtyfrom, &uidirtyto);

    } else if (k == 0x008) { /* BKSPC */
      bkspc(&db, &cursorposx, &cursorposy, screenw, &uidirtyfrom, &uidirtyto);

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

  /* TODO free memory */

  return(0);
}
