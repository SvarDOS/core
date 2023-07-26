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

#include <dos.h>      /* _dos_open(), _dos_read(), _dos_close(), ... */
#include <fcntl.h>    /* O_RDONLY, O_WRONLY */
#include <stdlib.h>
#include <string.h>

#include "mdr\bios.h"
#include "mdr\cout.h"
#include "mdr\dos.h"
#include "mdr\keyb.h"

#include "svarlang\svarlang.h"


/*****************************************************************************
 * global variables and definitions                                          *
 *****************************************************************************/

/* preload the mono scheme (to be overloaded at runtime if color adapter present) */
static unsigned char SCHEME_TEXT   = 0x07,
                     SCHEME_STBAR1 = 0x70,
                     SCHEME_STBAR2 = 0x70,
                     SCHEME_STBAR3 = 0xf0,
                     SCHEME_SCROLL = 0x70,
                     SCHEME_MSG    = 0x70,
                     SCHEME_ERR    = 0xf0;

static unsigned char screenw, screenh;

static struct {
    unsigned char from;
    unsigned char to;
    unsigned char statusbar;
} uidirty = {0, 0xff, 1}; /* make sure to redraw entire UI at first run */

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
  unsigned short totlines;
  unsigned short curline;
  char lfonly;   /* set if line endings are LF (CR/LF otherwise) */
  char modflag;  /* non-zero if file has been modified since last save */
  char fname[128];
};


/*****************************************************************************
 * functions                                                                 *
 *****************************************************************************/

static struct line far *line_calloc(unsigned short siz) {
  struct line far *res;
  unsigned int seg;
  if (_dos_allocmem((sizeof(struct line) + siz + 15) / 16, &seg) != 0) return(NULL);
  res = MK_FP(seg, 0);
  _fmemset(res, 0, sizeof(struct line) + siz);
  return(MK_FP(seg, 0));
}


static void line_free(struct line far *ptr) {
  _dos_freemem(FP_SEG(ptr));
}


static signed char curline_resize(struct file far *db, unsigned short newsiz) {
  unsigned int maxavail;
  struct line far *newptr;

  /* try resizing the block (much faster) */
  if (_dos_setblock((sizeof(struct line) + newsiz + 15) / 16, FP_SEG(db->cursor), &maxavail) == 0) return(0);

  /* create a new block and copy data over */
  newptr = line_calloc(newsiz);
  if (newptr == NULL) return(-1);
  _fmemmove(newptr, db->cursor, sizeof(struct line) + db->cursor->len);

  /* rewire the linked list */
  db->cursor = newptr;
  if (newptr->next) newptr->next->prev = newptr;
  if (newptr->prev) newptr->prev->next = newptr;

  return(0);
}


/* adds a new line at cursor position into file linked list and advance cursor
 * returns non-zero on error */
static int line_add(struct file *db, const char far *line, unsigned short slen) {
  struct line far *l;

  l = line_calloc(slen);
  if (l == NULL) return(-1);

  l->prev = db->cursor;
  if (db->cursor) {
    l->next = db->cursor->next;
    db->cursor->next = l;
    l->next->prev = l;
  }
  db->cursor = l;
  if (slen > 0) {
    _fmemmove(l->payload, line, slen);
    l->len = slen;
  }

  db->totlines += 1;
  db->curline += 1;

  return(0);
}


static void ui_getstring(const char *query, char *s, unsigned char maxlen) {
  unsigned char len = 0, y, x;
  int k;

  if (maxlen == 0) return;
  maxlen--; /* make room for the nul terminator */

  y = screenh - 1;

  /* print query string */
  x = mdr_cout_str(y, 0, query, SCHEME_STBAR3, 40);
  mdr_cout_char_rep(y, x++, ' ', SCHEME_STBAR3, screenw - x);

  for (;;) {
    mdr_cout_locate(y, x + len);
    k = keyb_getkey();

    if (k == 0x1b) return; /* ESC */

    if (k == '\r') {
      s[len] = 0;
      return;
    }

    if ((k == 0x08) && (len > 0)) { /* BKSPC */
      len--;
      mdr_cout_char(y, x + len, ' ', SCHEME_STBAR3);
      continue;
    }

    if ((k <= 0xff) && (k >= ' ') && (len < maxlen)) {
      mdr_cout_char(y, x + len, k, SCHEME_STBAR3);
      s[len++] = k;
    }

  }
}


/* append a nul-terminated string to line at cursor position */
static int line_append(struct file *f, const char far *buf, unsigned short len) {
  if (sizeof(struct line) + f->cursor->len + len < len) return(-1); /* overflow check */
  if (curline_resize(f, f->cursor->len + len) != 0) return(-1);
  _fmemmove(f->cursor->payload + f->cursor->len, buf, len);
  f->cursor->len += len;

  return(0);
}


static void db_rewind(struct file *db) {
  if (db->cursor == NULL) return;
  while (db->cursor->prev) db->cursor = db->cursor->prev;
  db->curline = 0;
}


static void ui_basic(const struct file *db) {
  const char *s = svarlang_strid(0); /* HELP */
  unsigned char helpcol = screenw - strlen(s);

  /* fill status bar with background (without modflag as it is refreshed by ui_refresh) */
  mdr_cout_char_rep(screenh - 1, 1, ' ', SCHEME_STBAR1, screenw - 1);

  /* filename */
  {
    const char *fn = db->fname;
    if (*fn == 0) fn = svarlang_str(0, 1);
    mdr_cout_str(screenh - 1, 1, fn, SCHEME_STBAR1, screenw);
  }

  /* eol type */
  {
    const char *eoltype = "CRLF";
    if (db->lfonly) eoltype = "LF";
    mdr_cout_str(screenh - 1, helpcol - 6, eoltype, SCHEME_STBAR1, 5);
  }

  mdr_cout_str(screenh - 1, helpcol, s, SCHEME_STBAR2, 40);
}


static void ui_msg(const char *msg1, const char *msg2, unsigned char attr) {
  unsigned short x, y, msglen, i;
  unsigned char msg2flag = 0;

  msglen = strlen(msg1);
  if (msg2) {
    msg2flag = 1;
    i = strlen(msg2);
    if (i > msglen) msglen = i;
  }

  y = (screenh - 6) >> 1;
  x = (screenw - msglen - 4) >> 1;
  for (i = y+2+msg2flag; i >= y; i--) mdr_cout_char_rep(i, x, ' ', attr, msglen + 2);
  x++;
  mdr_cout_str(y+1, x, msg1, attr, msglen);
  if (msg2) mdr_cout_str(y+2, x, msg2, attr, msglen);

  if (uidirty.from > y) uidirty.from = y;
  if (uidirty.to < y+4) uidirty.to = y+4;
}


static unsigned char ui_confirm_if_unsaved(struct file *db) {
  if (db->modflag == 0) return(0);

  /* if file has been modified then ask for confirmation */
  ui_msg(svarlang_str(0,4), svarlang_str(0,5), SCHEME_MSG);
  if (keyb_getkey() == '\r') return(0);

  return(1);
}


static void ui_help(void) {
#define MAXLINLEN 35
  unsigned char i, offset;
  offset = (screenw - MAXLINLEN + 2) >> 1;

  for (i = 2; i < 18; i++) {
    mdr_cout_char_rep(i, offset - 2, ' ', SCHEME_STBAR1, MAXLINLEN + 2);
  }

  for (i = 0; i < 20; i++) {
    const char *s = svarlang_str(8, i);
    if (s[0] == 0) break;
    if (s[0] == '.') continue;
    mdr_cout_locate(3 + i, offset + mdr_cout_str(3 + i, offset, s, SCHEME_STBAR1, MAXLINLEN));
  }

  keyb_getkey();
#undef MAXLINLEN
}


static void ui_refresh(const struct file *db) {
  unsigned char x;
  const struct line far *l;
  unsigned char y = db->cursorposy;

#ifdef DBG_REFRESH
  static char m = 'a';
  m++;
  if (m > 'z') m = 'a';
#endif

  /* rewind cursor line to first line that needs redrawing */
  for (l = db->cursor; y > uidirty.from; y--) l = l->prev;

  /* iterate over lines and redraw whatever needs to be redrawn */
  for (; l != NULL; l = l->next, y++) {

    /* skip lines that do not need to be refreshed */
    if (y < uidirty.from) continue;
    if (y > uidirty.to) break;

    x = 0;
    if (db->xoffset < l->len) {
      unsigned char i, limit;
      if (l->len - db->xoffset < screenw) {
        limit = l->len;
      } else {
        limit = db->xoffset + screenw - 1;
      }
      for (i = db->xoffset; i < limit; i++) mdr_cout_char(y, x++, l->payload[i], SCHEME_TEXT);
    }

    /* write empty spaces until end of line */
    if (x < screenw - 1) mdr_cout_char_rep(y, x, ' ', SCHEME_TEXT, screenw - 1 - x);

#ifdef DBG_REFRESH
    mdr_cout_char(y, 0, m, SCHEME_STBAR1);
#endif

    if (y == screenh - 2) break;
  }

  /* fill all lines below if empty (and they need to be redrawn) */
  if (l == NULL) {
    while ((y < screenh - 1) && (y < uidirty.to)) {
      mdr_cout_char_rep(y++, 0, ' ', SCHEME_TEXT, screenw - 1);
    }
  }

  /* "file changed" flag */
  {
    char flg = ' ';
    if (db->modflag) flg = '*';
    mdr_cout_char(screenh - 1, 0, flg, SCHEME_STBAR1);
  }

  /* scroll bar */
  for (y = 0; y < (screenh - 1); y++) {
    mdr_cout_char(y, screenw - 1, SCROLL_CURSOR, SCHEME_SCROLL);
  }

  /* scroll cursor */
  if (db->totlines >= screenh) {
    unsigned short topline = db->curline - db->cursorposy;
    unsigned short col;
    unsigned short totlines = db->totlines - screenh + 1;
    if (db->totlines - screenh > screenh) {
      col = topline / (totlines / (screenh - 1));
    } else {
      col = topline * (screenh - 1) / totlines;
    }
    if (col >= screenh - 1) col = screenh - 2;
    mdr_cout_char(col, screenw - 1, ' ', SCHEME_SCROLL);
  }
}


static void check_cursor_not_after_eol(struct file *db) {
  if (db->xoffset + db->cursorposx <= db->cursor->len) return;

  if (db->cursor->len < db->xoffset) {
    db->cursorposx = 0;
    db->xoffset = db->cursor->len;
    uidirty.from = 0;
    uidirty.to = 0xff;
  } else {
    db->cursorposx = db->cursor->len - db->xoffset;
  }
}


static void cursor_up(struct file *db) {
  if (db->cursor->prev != NULL) {
    db->curline -= 1;
    db->cursor = db->cursor->prev;
    if (db->cursorposy == 0) {
      uidirty.from = 0;
      uidirty.to = 0xff;
    } else {
      db->cursorposy -= 1;
    }
  }
}


static void cursor_eol(struct file *db) {
  /* adjust xoffset to make sure eol is visible on screen */
  if (db->xoffset > db->cursor->len) {
    db->xoffset = db->cursor->len - 1;
    uidirty.from = 0;
    uidirty.to = 0xff;
  }

  if (db->xoffset + screenw - 1 <= db->cursor->len) {
    db->xoffset = db->cursor->len - screenw + 2;
    uidirty.from = 0;
    uidirty.to = 0xff;
  }
  db->cursorposx = db->cursor->len - db->xoffset;
}


static void cursor_down(struct file *db) {
  if (db->cursor->next != NULL) {
    db->curline += 1;
    db->cursor = db->cursor->next;
    if (db->cursorposy < screenh - 2) {
      db->cursorposy += 1;
    } else {
      uidirty.from = 0;
      uidirty.to = 0xff;
    }
  }
}


static void cursor_left(struct file *db) {
  if (db->cursorposx > 0) {
    db->cursorposx -= 1;
  } else if (db->xoffset > 0) {
    db->xoffset -= 1;
    uidirty.from = 0;
    uidirty.to = 0xff;
  } else if (db->cursor->prev != NULL) { /* jump to end of line above */
    cursor_up(db);
    cursor_eol(db);
  }
}


static void cursor_home(struct file *db) {
  db->cursorposx = 0;
  if (db->xoffset != 0) {
    db->xoffset = 0;
    uidirty.from = 0;
    uidirty.to = 0xff;
  }
}


static void cursor_right(struct file *db) {
  if (db->cursor->len > db->xoffset + db->cursorposx) {
    if (db->cursorposx < screenw - 2) {
      db->cursorposx += 1;
    } else {
      db->xoffset += 1;
      uidirty.from = 0;
      uidirty.to = 0xff;
    }
  } else {
    cursor_down(db);
    cursor_home(db);
  }
}


static void del(struct file *db) {
  if (db->cursorposx + db->xoffset < db->cursor->len) {
    _fmemmove(db->cursor->payload + db->cursorposx + db->xoffset, db->cursor->payload + db->cursorposx + db->xoffset + 1, db->cursor->len - db->cursorposx - db->xoffset);
    db->cursor->len -= 1; /* do this AFTER memmove so the copy includes the nul terminator */
    uidirty.from = db->cursorposy;
    uidirty.to = db->cursorposy;
    db->modflag = 1;
  } else if (db->cursor->next != NULL) { /* cursor is at end of line: merge current line with next one (if there is a next one) */
    struct line far *nextline = db->cursor->next;
    if (db->cursor->next->len > 0) {
      if (curline_resize(db, db->cursor->len + db->cursor->next->len + 1) == 0) {
        _fmemmove(db->cursor->payload + db->cursor->len, db->cursor->next->payload, db->cursor->next->len + 1);
        db->cursor->len += db->cursor->next->len;
      }
    }

    db->cursor->next = db->cursor->next->next;
    db->cursor->next->prev = db->cursor;

    line_free(nextline);
    uidirty.from = db->cursorposy;
    uidirty.to = 0xff;
    db->totlines -= 1;
    db->modflag = 1;
  }
}


static void bkspc(struct file *db) {

  /* backspace is basically "left + del", not applicable only if cursor is on 1st byte of the file */
  if ((db->cursorposx == 0) && (db->xoffset == 0) && (db->cursor->prev == NULL)) return;

  cursor_left(db);
  del(db);
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
  char buff[512]; /* read one entire sector at a time (faster) */
  char *buffptr;
  unsigned int len, llen;
  int fd;
  unsigned char eolfound;
  static struct file db[1];

  bzero(db, sizeof(db));
  memcpy(db->fname, fname, strlen(fname));

  if (*fname == 0) goto SKIPLOADING;

  if (_dos_open(fname, O_RDONLY, &fd) != 0) {
    mdr_coutraw_puts("Failed to open file:");
    mdr_coutraw_puts(fname);
    return(NULL);
  }

  db->lfonly = 1;

  /* start by adding an empty line */
  if (line_add(db, NULL, 0) != 0) {
    /* TODO ERROR HANDLING */
  }

  for (eolfound = 0;;) {
    unsigned short consumedbytes;

    if ((_dos_read(fd, buff, sizeof(buff), &len) != 0) || (len == 0)) break;
    buffptr = buff;

    FINDLINE:

    /* look for nearest \n */
    for (consumedbytes = 0;; consumedbytes++) {
      if (consumedbytes == len) {
        llen = consumedbytes;
        break;
      }
      if (buffptr[consumedbytes] == '\r') {
        llen = consumedbytes;
        consumedbytes++;
        db->lfonly = 0;
        break;
      }
      if (buffptr[consumedbytes] == '\n') {
        eolfound = 1;
        llen = consumedbytes;
        consumedbytes++;
        break;
      }
    }

    /* consumedbytes is the amount of bytes processed from buffptr,
     * llen is the length of line's payload (without its line terminator) */

    /* append content, if line is non-empty */
    if ((llen > 0) && (line_append(db, buffptr, llen) != 0)) {
      mdr_coutraw_puts("out of memory");
      goto IOERR;
    }

    /* add a new line if necessary */
    if (eolfound) {
      if (line_add(db, NULL, 0) != 0) {
      /* TODO ERROR HANDLING */
        mdr_coutraw_puts("out of memory");
        goto IOERR;
      }
      eolfound = 0;
    }

    /* anything left? process the buffer leftover again */
    if (consumedbytes < len) {
      len -= consumedbytes;
      buffptr += consumedbytes;
      goto FINDLINE;
    }

  }

  _dos_close(fd);

  SKIPLOADING:

  /* add an empty line at end if not present already, also rewind cursor to top of file */
  if ((db->cursor == NULL) || (db->cursor->len != 0)) line_add(db, NULL, 0);
  db_rewind(db);

  return(db);

  IOERR:
  _dos_close(fd);
  return(NULL);
}


static int savefile(const struct file *db, const char *newfname) {
  int fd;
  const struct line far *l;
  unsigned bytes;
  unsigned char eollen;
  unsigned char eolbuf[2];
  int errflag = 0;

  /* either create a new file if newfname provided, or... */
  if (newfname) {
    if (_dos_creatnew(newfname, _A_NORMAL, &fd) != 0) return(-1);
  } else { /* ...open db->fname */
    if (_dos_open(db->fname, O_WRONLY, &fd) != 0) return(-1);
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
    /* do not write the last empty line, it is only useful for edition */
    if (l->len != 0) {
      errflag |= _dos_write(fd, l->payload, l->len, &bytes);
    } else if (l->next == NULL) {
      break;
    }
    errflag |= _dos_write(fd, eolbuf, eollen, &bytes);
    l = l->next;
  }

  errflag |= _dos_close(fd);

  return(errflag);
}


static void insert_in_line(struct file *db, const char *databuf, unsigned short len) {
  if (curline_resize(db, db->cursor->len + len) == 0) {
    unsigned short off = db->xoffset + db->cursorposx;
    db->modflag = 1;
    _fmemmove(db->cursor->payload + off + len, db->cursor->payload + off, db->cursor->len - off + 1);
    db->cursor->len += len;
    uidirty.from = db->cursorposy;
    uidirty.to = db->cursorposy;
    while (len--) {
      db->cursor->payload[off++] = *databuf;
      databuf++;
      cursor_right(db);
    }
  }
}


static void clear_file(struct file *db) {

  /* free the entire linked list of lines */
  db_rewind(db);
  while (db->cursor) {
    struct line far *victim;
    victim = db->cursor;
    db->cursor = db->cursor->next;
    line_free(victim);
  }

  /* zero out the struct */
  bzero(db, sizeof(struct file));

  /* add a single empty line and rewind */
  line_add(db, NULL, 0);
  db_rewind(db);
}


/* recompute db->curline by counting nodes in linked list */
static void recompute_curline(struct file *db) {
  const struct line far *l = db->cursor;

  db->curline = 0;
  while (l->prev != NULL) {
    db->curline += 1;
    l = l->prev;
  }
}


int main(void) {
  const char *fname;
  struct file *db;

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

  if (mdr_cout_init(&screenw, &screenh)) {
    /* load color scheme if mdr_cout_init returns a color flag */
    SCHEME_TEXT = 0x17;
    SCHEME_STBAR1 = 0x70;
    SCHEME_STBAR2 = 0x78;
    SCHEME_STBAR3 = 0xf0;
    SCHEME_SCROLL = 0x70;
    SCHEME_MSG = 0xf0;
    SCHEME_ERR = 0x4f;
  }

  for (;;) {
    int k;

    check_cursor_not_after_eol(db);
    mdr_cout_locate(db->cursorposy, db->cursorposx);

    if (uidirty.from != 0xff) {
      ui_refresh(db);
      uidirty.from = 0xff;
    }
    if (uidirty.statusbar) {
      ui_basic(db);
      uidirty.statusbar = 0;
    }
#ifdef DBG_LINENUM
      {
        char ddd[10];
        db->curline += 1;
        ddd[0] = '0' + db->curline / 100;
        ddd[1] = '0' + (db->curline % 100) / 10;
        ddd[2] = '0' + (db->curline % 10);
        db->curline -= 1;
        ddd[3] = '/';
        ddd[4] = '0' + db->totlines / 100;
        ddd[5] = '0' + (db->totlines % 100) / 10;
        ddd[6] = '0' + (db->totlines % 10);
        ddd[7] = 0;
        mdr_cout_str(screenh - 1, 40, ddd, SCHEME_STBAR1, sizeof(ddd));
      }
#endif

    k = keyb_getkey();

    if (k == 0x150) { /* down */
      cursor_down(db);

    } else if (k == 0x148) { /* up */
      cursor_up(db);

    } else if (k == 0x14D) { /* right */
      cursor_right(db);

    } else if (k == 0x14B) { /* left */
      cursor_left(db);

    } else if (k == 0x149) { /* pgup */
      unsigned char dist = db->cursorposy + screenh - 1;
      while ((dist != 0) && (db->cursor->prev != NULL)) {
        db->cursor = db->cursor->prev;
        dist--;
      }
      if (dist != 0) {
        db->cursorposy = 0;
        db->cursorposx = 0;
      } else {
        dist = db->cursorposy;
        while ((dist--) && (db->cursor->next)) db->cursor = db->cursor->next;
      }
      uidirty.from = 0;
      uidirty.to = 0xff;
      recompute_curline(db);

    } else if (k == 0x151) { /* pgdown */
      unsigned char dist = screenh + screenh - db->cursorposy - 3;
      while ((dist != 0) && (db->cursor->next != NULL)) {
        db->cursor = db->cursor->next;
        dist--;
      }
      if (dist != 0) {
        db->cursorposy = screenh - 2;
        if (db->totlines <= db->cursorposy) db->cursorposy = db->totlines - 1;
        db->cursorposx = 0;
      } else {
        dist = screenh - 2 - db->cursorposy;
        while ((dist--) && (db->cursor->prev)) db->cursor = db->cursor->prev;
      }
      uidirty.from = 0;
      uidirty.to = 0xff;
      recompute_curline(db);

    } else if (k == 0x147) { /* home */
       cursor_home(db);

    } else if (k == 0x14F) { /* end */
       cursor_eol(db);

    } else if (k == 0x1B) { /* ESC */
      if (ui_confirm_if_unsaved(db) == 0) break;

    } else if (k == 0x0D) { /* ENTER */
      unsigned short off = db->xoffset + db->cursorposx;
      /* add a new line */
      if (line_add(db, db->cursor->payload + off, db->cursor->len - off) == 0) {
        db->modflag = 1;
        db->cursor = db->cursor->prev; /* back to original line */
        /* trim the line above */
        db->cursor->len = off;
        /* move cursor to the (new) line below */
        db->curline -= 1;
        uidirty.from = db->cursorposy;
        uidirty.to = 0xff;
        cursor_down(db);
        cursor_home(db);
      } else {
        /* ERROR: OUT OF MEMORY */
      }

    } else if (k == 0x153) {  /* DEL */
      del(db);

    } else if (k == 0x008) { /* BKSPC */
      bkspc(db);

    } else if ((k >= 0x20) && (k <= 0xff)) { /* "normal" character */
      char c = k;
      insert_in_line(db, &c, 1);

    } else if (k == 0x009) { /* TAB */
      const char *tab = "        ";
      insert_in_line(db, tab, 8);

    } else if (k == 0x13b) { /* F1 */
      ui_help();
      uidirty.from = 0;
      uidirty.to = 0xff;

    } else if (k == 0x13c) { /* F2 (new file) */
      if (ui_confirm_if_unsaved(db) == 0) clear_file(db);
      uidirty.from = 0;
      uidirty.to = 0xff;
      uidirty.statusbar = 1;

    } else if ((k == 0x13f) || (k == 0x140)) { /* F5 or F6 */
      int saveres;

      if ((k == 0x140) || (db->fname[0] == 0)) { /* save as... */
        char fname[25];
        ui_getstring(svarlang_str(0,6), fname, sizeof(fname));
        if (*fname == 0) continue;
        saveres = savefile(db, fname);
        if (saveres == 0) memcpy(db->fname, fname, sizeof(fname));
      } else {
        saveres = savefile(db, NULL);
      }

      if (saveres == 0) {
        db->modflag = 0;
        ui_msg(svarlang_str(0, 2), NULL, SCHEME_MSG);
        mdr_bios_tickswait(11); /* 11 ticks is about 600 ms */
      } else {
        ui_msg(svarlang_str(0, 3), NULL, SCHEME_ERR);
        mdr_bios_tickswait(36); /* 2s */
      }

      ui_refresh(db);
      uidirty.statusbar = 1;

    } else if (k == 0x144) { /* F10 */
      db->modflag = 1;
      db->lfonly ^= 1;
      uidirty.statusbar = 1;

    } else if (k == 0x174) { /* CTRL+ArrRight - jump to next word */
      /* if currently cursor is on a non-space, then fast-forward to nearest space or EOL */
      for (;;) {
        if (db->xoffset + db->cursorposx == db->cursor->len) break;
        if (db->cursor->payload[db->xoffset + db->cursorposx] == ' ') break;
        cursor_right(db);
      }
      /* now skip to next non-space or end of file */
      for (;;) {
        cursor_right(db);
        if (db->cursor->payload[db->xoffset + db->cursorposx] != ' ') break;
        if ((db->cursor->next == NULL) && (db->cursorposx + db->xoffset == db->cursor->len)) break;
      }

    } else if (k == 0x173) { /* CTRL+ArrLeft - jump to prev word */
      cursor_left(db);
      /* if currently cursor is on a space, then fast-forward to nearest non-space or start of line */
      for (;;) {
        if ((db->xoffset == 0) && (db->cursorposx == 0)) break;
        if (db->cursor->payload[db->xoffset + db->cursorposx] != ' ') break;
        cursor_left(db);
      }
      /* now skip to next space or start of file */
      for (;;) {
        cursor_left(db);
        if (db->cursor->payload[db->xoffset + db->cursorposx] == ' ') {
          cursor_right(db);
          break;
        }
        if ((db->cursorposx == 0) && (db->xoffset == 0)) break;
      }

#ifdef DBG_UNHKEYS
    } else { /* UNHANDLED KEY - TODO IGNORE THIS IN PRODUCTION RELEASE */
      char buff[4];
      const char *HEX = "0123456789ABCDEF";
      buff[0] = HEX[(k >> 8) & 15];
      buff[1] = HEX[(k >> 4) & 15];
      buff[2] = HEX[k & 15];
      mdr_cout_str(screenh - 1, 0, "UNHANDLED KEY: 0x", SCHEME_STBAR1, 17);
      mdr_cout_str(screenh - 1, 17, buff, SCHEME_STBAR1, 3);
      keyb_getkey();
      break;
#endif
    }
  }

  mdr_cout_close();

  /* no need to free memory, DOS will do it for me */

  return(0);
}
