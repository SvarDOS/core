/* */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mdr\cout.h"
#include "mdr\keyb.h"

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


void ui_basic(unsigned char screenw, unsigned char screenh, const char *fname) {
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


void ui_refresh(const struct linedb *db, unsigned char screenw, unsigned char screenh) {
  unsigned char y = 0;
  unsigned char len;
  struct line *l;

  for (l = db->topscreen; l != NULL; l = l->next) {
    if (db->xoffset < l->len) {
      len = mdr_cout_str(y, 0, l->payload + db->xoffset, scheme[COL_TXT], screenw - 1);
    } else {
      len = 0;
    }
    while (len < screenw - 1) mdr_cout_char(y, len++, ' ', scheme[COL_TXT]);
    if (y == screenh - 2) break;
    y++;
  }
}


static void check_cursor_not_after_eol(struct linedb *db, unsigned char *cursorpos) {
  if (db->xoffset + *cursorpos <= db->cursor->len) return;

  if (db->cursor->len < db->xoffset) {
    *cursorpos = 0;
    db->xoffset = db->cursor->len;
  } else {
    *cursorpos = db->cursor->len - db->xoffset;
  }
}


static void cursor_up(struct linedb *db, unsigned char *cursorposy) {
  if (db->cursor->prev != NULL) {
    db->cursor = db->cursor->prev;
    if (*cursorposy == 0) {
      db->topscreen = db->cursor;
    } else {
      *cursorposy -= 1;
    }
  }
}


static void cursor_eol(struct linedb *db, unsigned char *cursorposx, unsigned char screenw) {
  /* adjust xoffset to make sure eol is visible on screen */
  if (db->xoffset >= db->cursor->len) db->xoffset = db->cursor->len - 1;
  if (db->xoffset + screenw - 1 <= db->cursor->len) db->xoffset = db->cursor->len - screenw + 2;
  *cursorposx = db->cursor->len - db->xoffset;
}


int main(int argc, char **argv) {
  FILE *fd;
  const char *fname = NULL;
  char buff[1024];
  struct linedb db;
  unsigned char screenw = 0, screenh = 0;
  unsigned char cursorposx = 0, cursorposy = 0;

  bzero(&db, sizeof(db));

  if ((argc != 2) || (argv[1][0] == '/')) {
    mdr_coutraw_puts("usage: sved file.txt");
    return(0);
  }

  fname = argv[1];

  printf("Loading %s...", fname);
  mdr_coutraw_puts("");

  fd = fopen(fname, "rb");
  if (fd == NULL) {
    printf("ERROR: failed to open file %s", fname);
    mdr_coutraw_puts("");
    return(1);
  }

  while (fgets(buff, sizeof(buff), fd) != NULL) {
    line_add(&db, buff);
  }
  /* add an empty line at end if not present already */
  if (db.cursor->len != 0) line_add(&db, "");

  fclose(fd);

  if (mdr_cout_init(&screenw, &screenh)) load_colorscheme();
  ui_basic(screenw, screenh, fname);

  db_rewind(&db);

  for (;;) {
    int k;

    check_cursor_not_after_eol(&db, &cursorposx);
    mdr_cout_locate(cursorposy, cursorposx);

    ui_refresh(&db, screenw, screenh);

    k = keyb_getkey();
    if (k == 0x150) { /* down */
      if (db.cursor->next != NULL) {
        db.cursor = db.cursor->next;
        if (cursorposy < screenh - 2) {
          cursorposy++;
        } else {
          db.topscreen = db.topscreen->next;
        }
      }

    } else if (k == 0x148) { /* up */
      cursor_up(&db, &cursorposy);

    } else if (k == 0x14D) { /* right */
      if (db.cursor->len > db.xoffset + cursorposx) {
        if (cursorposx < screenw - 2) {
          cursorposx++;
        } else {
          db.xoffset++;
        }
      }

    } else if (k == 0x14B) { /* left */
      if (cursorposx > 0) {
        cursorposx--;
      } else if (db.xoffset > 0) {
        db.xoffset--;
      } else {
        cursor_up(&db, &cursorposy);
        cursor_eol(&db, &cursorposx, screenw);
      }

    } else if (k == 0x1B) { /* ESC */
      break;

    } else {
      printf("UNHANDLED KEY 0x%02X", k);
      keyb_getkey();
      break;
    }
  }

  mdr_cout_close();

  /* TODO free memory */

  return(0);
}
