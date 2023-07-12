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

#include "mdr\cout.h"
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
  struct line *prev;
  struct line *next;
  unsigned short len;
  char payload[1];
};

struct linedb {
  struct line *topscreen;
  struct line *cursor;
  unsigned short xoffset;
};


void line_add(struct linedb *db, const char *line) {
  unsigned short slen = strlen(line);
  struct line *l;

  /* trim out CR/LF line endings */
  if ((slen >= 2) && (line[slen - 2] == '\r')) {
    slen -= 2;
  } else if ((slen >= 1) && (line[slen - 1] == '\n')) {
    slen--;
  }

  l =  calloc(1, sizeof(struct line) + slen + 1);
  l->prev = db->cursor;
  if (db->cursor) {
    l->next = db->cursor->next;
    db->cursor->next = l;
    l->next->prev = l;
  }
  db->cursor = l;
  memcpy(l->payload, line, slen);
  l->len = slen;
}


void db_rewind(struct linedb *db) {
  if (db->cursor == NULL) return;
  while (db->cursor->prev) db->cursor = db->cursor->prev;
  db->topscreen = db->cursor;
}


void load_colorscheme(void) {
  scheme[COL_TXT] = 0x17;
  scheme[COL_STATUSBAR1] = 0x70;
  scheme[COL_STATUSBAR2] = 0x78;
  scheme[COL_SCROLLBAR] = 0x70;
}


static void ui_basic(unsigned char screenw, unsigned char screenh, const char *fname) {
  unsigned char i;
  const char *s = "HELP";
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
  struct line *l;

  /* DEBUG TODO FIXME */
  static char m = 'a';
  m++;
  if (m > 'z') m = 'a';

  for (l = db->topscreen; l != NULL; l = l->next) {

    /* skip lines that do not to be refreshed */
    if ((y < uidirtyfrom) || (y > uidirtyto)) continue;

    if (db->xoffset < l->len) {
      len = mdr_cout_str(y, 0, l->payload + db->xoffset, scheme[COL_TXT], screenw - 1);
    } else {
      len = 0;
    }
    while (len < screenw - 1) mdr_cout_char(y, len++, ' ', scheme[COL_TXT]);

    /* FIXME DEBUG */
    mdr_cout_char(y, 0, m, scheme[COL_STATUSBAR1]);

    if (y == screenh - 2) break;
    y++;
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


static void cursor_home(struct linedb *db, unsigned char *cursorposx, unsigned char *uidirtyfrom, unsigned char *uidirtyto) {
  *cursorposx = 0;
  if (db->xoffset != 0) {
    db->xoffset = 0;
    *uidirtyfrom = 0;
    *uidirtyto = 0xff;
  }
}


int main(int argc, char **argv) {
  int fd;
  const char *fname = NULL;
  char buff[1024];
  struct linedb db;
  unsigned char screenw = 0, screenh = 0;
  unsigned char cursorposx = 0, cursorposy = 0;
  unsigned char uidirtyfrom = 0, uidirtyto = 0xff; /* make sure to redraw entire UI at first run */

  bzero(&db, sizeof(db));

  svarlang_autoload_nlspath("sved");

  if ((argc != 2) || (argv[1][0] == '/')) {
    mdr_coutraw_puts("usage: sved file.txt");
    return(0);
  }

  fname = argv[1];

  mdr_coutraw_puts("");

  if (_dos_open(fname, O_RDONLY, &fd) != 0) {
    mdr_coutraw_puts("Failed to open file:");
    mdr_coutraw_puts(fname);
    return(1);
  }

  /* load file */
  {
    unsigned int prevlen = 0, len, llen;

    do {
      if (_dos_read(fd, buff + prevlen, sizeof(buff) - prevlen, &len) != 0) break;
      len += prevlen;

      /* look for nearest \n and replace with 0*/
      for (llen = 0; buff[llen] != '\n'; llen++) {
        if (llen == sizeof(buff)) break;
      }
      buff[llen] = 0;
      if ((llen > 0) && (buff[llen - 1])) buff[llen - 1] = 0; /* trim \r if line ending is cr/lf */
      line_add(&db, buff);

      len -= llen + 1;
      memmove(buff, buff + llen + 1, len);
      prevlen = len;
    } while (len > 0);

  }

  /* add an empty line at end if not present already */
  if (db.cursor->len != 0) line_add(&db, "");

  _dos_close(fd);

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
      if (cursorposx > 0) {
        cursorposx--;
      } else if (db.xoffset > 0) {
        db.xoffset--;
        uidirtyfrom = 0;
        uidirtyto = 0xff;
      } else if (db.cursor->prev != NULL) { /* jump to end of line above */
        cursor_up(&db, &cursorposy, &uidirtyfrom, &uidirtyto);
        cursor_eol(&db, &cursorposx, screenw, &uidirtyfrom, &uidirtyto);
      }

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
