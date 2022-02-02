/*
 * Copyright (C) 2021-2022 Mateusz Viste
 *
 * usage: tlumacz en fr pl etc
 *
 * computes a svarcom.lng file that contains all language ressources found
 * inside dirname.
 *
 * DAT format:
 *
 * 4-bytes signature:
 * "SvL\x1b"
 *
 * Then "LANG BLOCKS" follow. Each LANG BLOCK is prefixed with 4 bytes:
 * II LL    - II is the LANG identifier ("EN", "PL", etc) and LL is the size
 *            of the block (65535 bytes max).
 *
 * Inside a LANG BLOCK is a set of strings:
 *
 * II L S  where II is the string's 16-bit identifier, L is its length (1-255)
 *         and S is the actual string. All strings are ASCIIZ-formatted (ie.
 *         end with a NULL terminator).
 *
 * The list of strings ends with a single 0-long string.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>



struct bitmap {
  unsigned char bits[8192];
};

static void bitmap_set(struct bitmap *b, unsigned short id) {
  b->bits[id / 8] |= 1 << (id & 7);
}

static int bitmap_get(const struct bitmap *b, unsigned short id) {
  return(b->bits[id / 8] & (1 << (id & 7)));
}

static void bitmap_init(struct bitmap *b) {
  memset(b, 0, sizeof(struct bitmap));
}



/* read a single line from fd and fills it into dst, returns line length
 * ending CR/LF is trimmed, as well as any trailing spaces */
static unsigned short readl(char *dst, size_t dstsz, FILE *fd) {
  unsigned short l, lastnonspace = 0;

  if (fgets(dst, dstsz, fd) == NULL) return(0xffff); /* EOF */
  /* trim at first CR or LF and return len */
  for (l = 0; (dst[l] != 0) && (dst[l] != '\r') && (dst[l] != '\n'); l++) {
    if (dst[l] != ' ') lastnonspace = l;
  }

  if (lastnonspace < l) l = lastnonspace + 1; /* rtrim */
  dst[l] = 0;

  return(l);
}


/* parse a line in format "1.50:somestring". fills id and returns a pointer to
 * the actual string part on success, or NULL on error */
static char *parseline(unsigned short *id, char *s) {
  int i;
  int dotpos = 0, colpos = 0, gotdigits = 0;

  /* I must have a . and a : in the first 9 bytes */
  for (i = 0;; i++) {
    if (s[i] == '.') {
      if ((dotpos != 0) || (gotdigits == 0)) break;
      dotpos = i;
      gotdigits = 0;
    } else if (s[i] == ':') {
      if (gotdigits != 0) colpos = i;
      break;
    } else if ((s[i] < '0') || (s[i] > '9')) {
      break;
    }
    gotdigits++;
  }
  /* did I collect everything? */
  if ((dotpos == 0) || (colpos == 0)) return(NULL);
  if (s[colpos + 1] == 0) return(NULL);

  *id = atoi(s);
  *id <<= 8;
  *id |= atoi(s + dotpos + 1);

  /* printf("parseline(): %04X = '%s'\r\n", *id, s + colpos + 1); */

  return(s + colpos + 1);
}


/* opens a CATS-style file and compiles it into a ressources lang block */
static unsigned short gen_langstrings(unsigned char *buff, const char *langid, struct bitmap *b, const struct bitmap *refb, const unsigned char *refblock) {
  unsigned short len = 0, linelen;
  FILE *fd;
  char fname[] = "XX.TXT";
  char linebuf[512];
  char *ptr;
  unsigned short id, linecount;

  bitmap_init(b);

  memcpy(fname + strlen(fname) - 6, langid, 2);

  fd = fopen(fname, "rb");
  if (fd == NULL) {
    printf("ERROR: FAILED TO OPEN '%s'\r\n", fname);
    return(0);
  }

  for (linecount = 1;; linecount++) {

    linelen = readl(linebuf, sizeof(linebuf), fd);
    if (linelen == 0xffff) break; /* EOF */
    if ((linelen == 0) || (linebuf[0] == '#')) continue;

    /* read id and get ptr to actual string ("1.15:string") */
    ptr = parseline(&id, linebuf);
    if (ptr == NULL) {
      printf("ERROR: line #%u of %s is malformed\r\n", linecount, fname);
      len = 0;
      break;
    }

    /* write string into block (II L S) */
    memcpy(buff + len, &id, 2);
    len += 2;
    buff[len++] = strlen(ptr) + 1;
    memcpy(buff + len, ptr, strlen(ptr) + 1);
    len += strlen(ptr) + 1;

    /* if reference bitmap provided: check that the id is valid */
    if ((refb != NULL) && (bitmap_get(refb, id) == 0)) {
      printf("WARNING: %s[#%u] has an invalid id (%u.%u not present in ref lang)\r\n", fname, linecount, id >> 8, id & 0xff);
    }

    /* make sure this id is not already present */
    if (bitmap_get(b, id) == 0) {
      /* set bit in bitmap to remember I have this string */
      bitmap_set(b, id);
    } else {
      printf("WARNING: %s[#%u] has a duplicated id (%u.%u)\r\n", fname, linecount, id >> 8, id & 0xff);
    }
  }

  fclose(fd);

  /* if refblock provided, pull missing strings from it */
  if (refblock != NULL) {
    for (;;) {
      id = *((unsigned short *)refblock);
      if ((id == 0) && (refblock[2] == 0)) break;
      if (bitmap_get(b, id) == 0) {
        printf("WARNING: %s is missing string %u.%u (pulled from ref lang)\r\n", fname, id >> 8, id & 0xff);
        /* copy missing string from refblock */
        memcpy(buff + len, refblock, refblock[2] + 3);
        len += refblock[2] + 3;
      }
      refblock += refblock[2] + 3;
    }
  }

  /* write the block terminator (0-long string) */
  buff[len++] = 0; /* id */
  buff[len++] = 0; /* id */
  buff[len++] = 0; /* len */

  return(len);
}


#define MEMBLOCKSZ 65500

int main(int argc, char **argv) {
  FILE *fd;
  int ecode = 0;
  char *buff, *refblock;
  static struct bitmap bufbitmap;
  static struct bitmap refbitmap;
  unsigned short i;

  if (argc < 2) {
    puts("usage: tlumacz en fr pl etc");
    return(1);
  }

  buff = malloc(MEMBLOCKSZ);
  refblock = malloc(MEMBLOCKSZ);
  if ((buff == NULL) || (refblock == NULL)) {
    puts("out of memory");
    return(1);
  }

  fd = fopen("svarcom.lng", "wb");
  if (fd == NULL) {
    puts("ERR: failed to open or create SVARCOM.LNG");
    return(1);
  }

  /* write sig */
  fwrite("SvL\x1b", 1, 4, fd);

  /* write lang blocks */
  for (i = 1; i < argc; i++) {
    unsigned short sz;
    char id[3];

    if (strlen(argv[i]) != 2) {
      printf("INVALID LANG SPECIFIED: %s\r\n", argv[i]);
      ecode = 1;
      break;
    }

    id[0] = argv[i][0];
    id[1] = argv[i][1];
    id[2] = 0;
    if (id[0] >= 'a') id[0] -= 'a' - 'A';
    if (id[1] >= 'a') id[1] -= 'a' - 'A';

    sz = gen_langstrings(buff, id, &bufbitmap, (i != 1)?&refbitmap:NULL, (i != 1)?refblock:NULL);
    if (sz == 0) {
      printf("ERROR COMPUTING LANG '%s'\r\n", id);
      ecode = 1;
      break;
    } else {
      printf("computed %s lang block of %u bytes\r\n", id, sz);
    }
    /* write lang ID to file, followed by block size and then the actual block */
    if ((fwrite(id, 1, 2, fd) != 2) ||
        (fwrite(&sz, 1, 2, fd) != 2) ||
        (fwrite(buff, 1, sz, fd) != sz)) {
      printf("ERROR WRITING TO OUTPUT FILE\r\n");
      ecode = 1;
      break;
    }
    /* compute the default block for reference language */
    if (i == 1) {
      unsigned short x;
      FILE *fd2;
      fd2 = fopen("DEFLANG.C", "wb");
      if (fd2 == NULL) {
        puts("ERROR: FAILED TO OPEN OR CREATE DEFLANG.C");
        break;
      }
      fprintf(fd2, "/* THIS FILE HAS BEEN AUTOGENERATE BY TLUMACZ (PART OF THE SVARLANG LIBRARY) */\r\n");
      fprintf(fd2, "static char svarlang_mem[%u] = {\r\n", sz * 2);
      for (x = 0; x < sz; x++) {
        fprintf(fd2, "%u", buff[x]);
        if (x + 1 < sz) fprintf(fd2, ",");
        if ((x & 15) == 15) fprintf(fd2, "\r\n");
      }
      fprintf(fd2, "};\r\n");
      fclose(fd2);
      /* remember reference data for other languages */
      memcpy(refblock, buff, MEMBLOCKSZ);
      memcpy(&refbitmap, &bufbitmap, sizeof(struct bitmap));
    }
  }

  fclose(fd);

  return(ecode);
}
