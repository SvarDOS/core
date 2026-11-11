/*
 * Copyright (C) 2021-2026 Mateusz Viste
 *
 * Dictionary-based lookups contributed by Bernd Boeckmann, 2023
 *
 * usage: tlumacz en fr pl etc
 *
 * computes:
 * OUT.LNG -> contains all language resources.
 *
 * String bytes are MVCOMP-compressed unless /nocomp is given. The compressor
 * is the bundled MVCOMP library in mvcomp/ (mvpack.c).
 * A compressed language block has the high bit set in its 16-bit language id.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "svarlang.h"
#include "mvcomp.h"

#define STRINGS_CAP 65000   /* string storage size in characters */
#define DICT_CAP    10000   /* dictionary size in elements */

enum {                      /* DEFLANG output format */
  C_OUTPUT,
  NO_OUTPUT,
  ASM_OUTPUT,
  NASM_OUTPUT
};


/* read a single line from fd and fills it into dst, returns line length
 * ending CR/LF is trimmed, as well as any trailing spaces */
static unsigned short readl(char *dst, size_t dstsz, FILE *fd) {
  unsigned short l, lastnonspace = 0;

  if (fgets(dst, (int)dstsz, fd) == NULL) return(0xffff); /* EOF */
  /* trim at first CR or LF and return len */
  for (l = 0; (dst[l] != 0) && (dst[l] != '\r') && (dst[l] != '\n'); l++) {
    if (dst[l] != ' ') lastnonspace = l;
  }

  if (lastnonspace < l) l = lastnonspace + 1; /* rtrim */
  dst[l] = 0;

  return(l);
}


/* parse a line in format "[?]1.50:somestring". fills id and returns a pointer to
 * the actual string part on success, or NULL on error. sets *toobig when the
 * id does not fit in one byte per part (a fatal error for the caller). */
static const char *parseline(unsigned short *id, const char *s, int *toobig) {
  int i, major, minor;
  int dotpos = 0, colpos = 0, gotdigits = 0;

  *toobig = 0;

  /* strings prefixed by '?' are flagged as "dirty": ignore this flag here */
  if (*s == '?') s++;

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
  /* each part is one byte: at most 3 digits, and not above 255 */
  if ((dotpos > 3) || (colpos - dotpos - 1 > 3)) {
    *toobig = 1;
    return(NULL);
  }

  major = atoi(s);
  minor = atoi(s + dotpos + 1);
  if ((major > 255) || (minor > 255)) {
    *toobig = 1;
    return(NULL);
  }
  *id = (unsigned short)(major << 8) | (unsigned short)minor;

  /* printf("parseline(): %04X = '%s'\r\n", *id, s + colpos + 1); */

  return(s + colpos + 1);
}


/* converts escape sequences like "\n" or "\t" into actual bytes, returns
 * the new length of the string. */
static unsigned short unesc_string(char *linebuff) {
  unsigned short i;
  for (i = 0; linebuff[i] != 0; i++) {
    if (linebuff[i] != '\\') continue;
    memmove(linebuff + i, linebuff + i + 1, strlen(linebuff + i));
    if (linebuff[i] == 0) break;
    switch (linebuff[i]) {
      case 'e':
        linebuff[i] = 0x1B; /* ESC code, using hex because '\e' is not ANSI C */
        break;
      case 'n':
        linebuff[i] = '\n';
        break;
      case 'r':
        linebuff[i] = '\r';
        break;
      case 't':
        linebuff[i] = '\t';
        break;
    }
  }
  return(i);
}

#pragma pack(1)
struct dict_entry {
  unsigned short id;
  unsigned short offset;
};
#pragma pack()

struct svl_lang {
  char id[2];
  unsigned short num_strings;

  struct dict_entry *dict;
  size_t dict_cap;

  char *strings;
  char *strings_end;
  size_t strings_cap;

};


static struct svl_lang *svl_lang_new(const char langid[2], size_t dict_cap, size_t strings_cap) {
  struct svl_lang *l;

  l = malloc(sizeof(struct svl_lang));
  if (!l) return(NULL);

  l->id[0] = (char)toupper(langid[0]);
  l->id[1] = (char)toupper(langid[1]);

  l->dict = malloc(dict_cap * sizeof(struct dict_entry));
  if (!l->dict) return(NULL);

  l->dict_cap = dict_cap;

  l->num_strings = 0;
  l->strings = l->strings_end = malloc(strings_cap);
  if (!l->strings) {
    free(l->dict);
    return(NULL);
  }
  l->strings_cap = strings_cap;

  return(l);
}


/* compacts the dict and string buffer */
static void svl_compact_lang(struct svl_lang *l) {
  size_t bytes;
  bytes = l->strings_end - l->strings;
  if (bytes < l->strings_cap) {
    l->strings = l->strings_end = realloc(l->strings, bytes);
    l->strings_end += bytes;
    l->strings_cap = bytes;
  }
  l->dict_cap = l->num_strings;
  l->dict = realloc(l->dict, l->dict_cap * sizeof(struct dict_entry));
}


static void svl_lang_free(struct svl_lang *l) {
  l->num_strings = 0;
  if (l->dict) {
    free(l->dict);
    l->dict = NULL;
  }
  if (l->strings) {
    free(l->strings);
    l->strings = l->strings_end = NULL;
  }
  l->dict_cap = 0;
  l->strings_cap = 0;
}


static size_t svl_strings_bytes(const struct svl_lang *l) {
  return(l->strings_end - l->strings);
}


static size_t svl_dict_bytes(const struct svl_lang *l) {
  return(l->num_strings * sizeof(struct dict_entry));
}


static int svl_add_str(struct svl_lang *l, unsigned short id, const char *s) {
  size_t len = strlen(s) + 1;
  size_t cursor;

  /* dict_cap counts entries; strings_cap counts bytes */
  if ((l->strings_cap < svl_strings_bytes(l) + len) || (l->dict_cap < l->num_strings + 1)) {
    return(0);
  }

  /* find dictionary insert position, search backwards in assumption
     that in translation files, strings are generally ordered ascending */
  for (cursor = l->num_strings; cursor > 0 && l->dict[cursor-1].id > id; cursor--);

  memmove(&(l->dict[cursor+1]), &(l->dict[cursor]), sizeof(struct dict_entry) * (l->num_strings - cursor));
  l->dict[cursor].id = id;
  l->dict[cursor].offset = l->strings_end - l->strings;

  memcpy(l->strings_end, s, len);
  l->strings_end += len;
  l->num_strings++;

  return(1);
}


static int svl_find(const struct svl_lang *l, unsigned short id) {
  size_t left = 0, right = l->num_strings - 1, x;
  unsigned short v;

  if (l->num_strings == 0) return(0);

  while (left <= right ) {
    x = left + ( (right - left ) >> 2 );
    v = l->dict[x].id;
    if ( id == v ) return(1); /* found! */

    if (id > v) {
      if (x == 65535) return(0); /* avoid left overflow */
      left = x + 1;
    } else {
      if (x == 0) return(0); /* avoid right underflow */
      right = x - 1;
    }
  }
  return(0);
}


/* opens a CATS-style file and compiles it into a ressources lang block
 * returns 0 on error, or the size of the generated data block otherwise */
static unsigned short svl_lang_from_cats_file(struct svl_lang *l, struct svl_lang *refl) {
  unsigned short linelen;
  FILE *fd;
  char fname[] = "xx.txt";
  static char linebuf[8192];
  const char *ptr;
  unsigned short id, maxid=0, maxid_line=0, linecount;
  int i, toobig;

  fname[strlen(fname) - 6] = (char)tolower( l->id[0] );
  fname[strlen(fname) - 5] = (char)tolower( l->id[1] );

  fd = fopen(fname, "rb");
  if (fd == NULL) {
    printf("ERROR: FAILED TO OPEN '%s'\n", fname);
    return(0);
  }

  for (linecount = 1;; linecount++) {
    linelen = readl(linebuf, sizeof(linebuf), fd);
    if (linelen == 0xffff) break; /* EOF */
    if ((linelen == 0) || (linebuf[0] == '#')) continue;

    /* convert escaped chars to actual bytes (\n -> newline, etc) */
    linelen = unesc_string(linebuf);

    /* read id and get ptr to actual string ("1.15:string") */
    ptr = parseline(&id, linebuf, &toobig);

    /* an id that does not fit in one byte per part would alias another id */
    if (toobig) {
      fprintf(stderr, "ERROR: %s[#%u] string id exceeds 255.255\r\n", fname, linecount);
      fprintf(stderr, "%s\r\n", linebuf);
      fclose(fd);
      return(0);
    }

    /* handle malformed lines */
    if (ptr == NULL) {
      printf("WARNING: %s[#%u] is malformed (linelen = %u):\n", fname, linecount, linelen);
      puts(linebuf);
      continue;
    }

    /* ignore empty strings (but emit a warning) */
    if (ptr[0] == 0) {
      printf("WARNING: %s[#%u] ignoring empty string %u.%u\n", fname, linecount, id >> 8, id & 0xff);
      continue;
    }

    /* warn about dirty lines */
    if (linebuf[0] == '?') {
      printf("WARNING: %s[#%u] string id %u.%u is flagged as 'dirty'\n", fname, linecount, id >> 8, id & 0xff);
    }

    /* add the string contained in current line, if conditions are met */
    if (!svl_find(l, id)) {
      if ((refl == NULL) || (svl_find(refl, id))) {
        if (!svl_add_str(l, id, ptr)) {
          fprintf(stderr, "ERROR: %s[#%u] output size limit exceeded\r\n", fname, linecount);
          fclose(fd);
          return(0);
        }
        if (id >= maxid) {
          maxid = id;
          maxid_line = linecount;
        } else {
          printf("WARNING:%s[#%u] file unsorted - line %u has higher id %u.%u\n", fname, linecount, maxid_line, maxid >> 8, maxid & 0xff);
        }
      } else {
        printf("WARNING: %s[#%u] has an invalid id (%u.%u not present in ref lang)\n", fname, linecount, id >> 8, id & 0xff);
      }
    } else {
      printf("WARNING: %s[#%u] has a duplicated id (%u.%u)\n", fname, linecount, id >> 8, id & 0xff);
    }
  }

  fclose(fd);

  /* if reflang provided, pull missing strings from it */
  if (refl != NULL) {
    for (i = 0; i < refl->num_strings; i++) {
      id = refl->dict[i].id;
      if (!svl_find(l, id)) {
        printf("WARNING: %s is missing string %u.%u (pulled from ref lang)\n", fname, id >> 8, id & 0xff);
        if (!svl_add_str(l, id, refl->strings + refl->dict[i].offset)) {
          fprintf(stderr, "ERROR: %s[#%u] output size limit exceeded\r\n", fname, linecount);
          return(0);
        }
      }
    }
  }

  return(svl_strings_bytes(l));
}


static int svl_write_header(unsigned short num_strings, FILE *fd) {
  return((fwrite("SvL\x1a", 1, 4, fd) == 4) && (fwrite(&num_strings, 1, 2, fd) == 2));
}



/* write the language block (id, dict, strings) into the LNG file.
 * strings are compressed when compflag != 0, using mvcomp_pack(). */
static int svl_write_lang(const struct svl_lang *l, FILE *fd, int compflag) {
  unsigned short strings_bytes = svl_strings_bytes(l);
  unsigned short langid = *((unsigned short *)(&l->id));
  const unsigned char *stringsptr = (const unsigned char *)l->strings;
  unsigned char *packed = NULL;
  int ok;

  if (compflag) {
    /* MVCOMP_BOUND is enough for any result. A 16-bit segment cannot hold
     * more than 65534 bytes, and a block that does not shrink is stored raw. */
    unsigned long bound = MVCOMP_BOUND((unsigned long)strings_bytes);
    unsigned long cap = bound;
    unsigned long got;
    struct mvcomp *probe;
    if (cap > 65534ul) cap = 65534ul;
    packed = malloc((size_t)cap);
    if (packed == NULL) {
      fprintf(stderr, "ERROR: OUT OF MEMORY\r\n");
      return(0);
    }
    got = mvcomp_pack(packed, cap, stringsptr, strings_bytes);
    if (got == MVCOMP_ERR) {
      /* the bound always fits the result. a short buffer means the packed
       * form does not fit in one 16-bit segment. a failed index allocation
       * is reported on its own. */
      probe = mvcomp_open();
      if (probe == NULL) {
        fprintf(stderr, "ERROR: OUT OF MEMORY\r\n");
        free(packed);
        return(0);
      }
      mvcomp_close(probe);
      if (cap >= bound) {
        fprintf(stderr, "ERROR: mvcomp failed\r\n");
        free(packed);
        return(0);
      }
      printf("lang %c%c left UNCOMPRESSED (uncomp=%u bytes, mvcomp result exceeds 64K)\n", l->id[0], l->id[1], strings_bytes);
    } else if (got < strings_bytes) {
      printf("lang %c%c mvcomp-ressed (%u bytes -> %lu bytes)\n", l->id[0], l->id[1], strings_bytes, got);
      langid |= 0x8000; /* LNG langblock flag that means "this lang is compressed" */
      strings_bytes = (unsigned short)got;
      stringsptr = packed;
    } else {
      printf("lang %c%c left UNCOMPRESSED (uncomp=%u bytes ; mvcomp=%lu bytes)\n", l->id[0], l->id[1], strings_bytes, got);
    }
  }

  ok = ((fwrite(&langid, 1, 2, fd) == 2) &&
        (fwrite(&strings_bytes, 1, 2, fd) == 2) &&
        (fwrite(l->dict, 1, svl_dict_bytes(l), fd) == svl_dict_bytes(l)) &&
        (fwrite(stringsptr, 1, strings_bytes, fd) == strings_bytes));
  free(packed);
  return(ok);
}


/* 5% margin, added in 32 bits so a block above 62415 bytes does not wrap.
 * margin == 0 keeps the exact block size (/nomarg). */
static int svl_margin_allocsz(unsigned short biggest, int margin, unsigned short *allocsz) {
  unsigned long sz;
  if (!margin) {
    *allocsz = biggest;
    return(1);
  }
  sz = (unsigned long)biggest + (unsigned long)biggest / 20;
  if (sz > 65535ul) return(0);
  *allocsz = (unsigned short)sz;
  return(1);
}


static int svl_write_c_source(const struct svl_lang *l, const char *fn, unsigned short biggest_langsz, int margin) {
  FILE *fd;
  int i;
  unsigned short strings_bytes = svl_strings_bytes(l);
  unsigned short nextnlat = 0;
  unsigned short allocsz;

  if (!svl_margin_allocsz(biggest_langsz, margin, &allocsz)) {
    fprintf(stderr, "ERROR: lang block of %u bytes + 5%% margin exceeds 65535\r\n", biggest_langsz);
    return(-1);
  }

  fd = fopen(fn, "wb");
  if (fd == NULL) {
    return(0);
  }

  printf("biggest lang block is %u bytes -> allocating a %u bytes buffer%s\n", biggest_langsz, allocsz, margin ? " (5% safety margin)" : "");
  fprintf(fd, "/* THIS FILE HAS BEEN GENERATED BY TLUMACZ (PART OF THE SVARLANG LIBRARY) */\r\n");
  fprintf(fd, "const unsigned short svarlang_memsz = %uu;\r\n", allocsz);
  fprintf(fd, "const unsigned short svarlang_string_count = %uu;\r\n\r\n", l->num_strings);
  fprintf(fd, "char svarlang_mem[%u] = {\r\n", allocsz);

  for (i = 0; i < strings_bytes; i++) {
    if (!fprintf(fd, "0x%02x", l->strings[i])) {
      fclose(fd);
      return(0);
    }

    if (i + 1 < strings_bytes) fprintf(fd, ",");
    nextnlat++;
    if (l->strings[i] == '\0' || nextnlat == 16) {
      fprintf(fd, "\r\n");
      nextnlat = 0;
    }
  }
  fprintf(fd, "};\r\n\r\n");

  fprintf(fd, "unsigned short svarlang_dict[%u] = {\r\n", l->num_strings * 2);
  for (i = 0; i < l->num_strings; i++) {
    if (!fprintf(fd, "0x%04x,0x%04x", l->dict[i].id, l->dict[i].offset)) {
      fclose(fd);
      return(0);
    }
    if (i + 1 < l->num_strings) fprintf(fd, ",");
    fprintf(fd, "\r\n");
  }
  fprintf(fd, "};\r\n");

  fclose(fd);

  return(1);
}


static int svl_write_asm_source(const struct svl_lang *l, const char *fn, unsigned short biggest_langsz, int format, int margin) {
  FILE *fd;
  int i;
  unsigned short strings_bytes = svl_strings_bytes(l);
  unsigned short nextnlat = 0;
  unsigned short allocsz;

  const char *public = (format == ASM_OUTPUT) ? "public" : "global";

  if (!svl_margin_allocsz(biggest_langsz, margin, &allocsz)) {
    fprintf(stderr, "ERROR: lang block of %u bytes + 5%% margin exceeds 65535\r\n", biggest_langsz);
    return(-1);
  }

  fd = fopen(fn, "wb");
  if (fd == NULL) {
    return(0);
  }

  printf("biggest lang block is %u bytes -> allocating a %u bytes buffer%s\n", biggest_langsz, allocsz, margin ? " (5% safety margin)" : "");
  fprintf(fd, "; THIS FILE HAS BEEN GENERATED BY TLUMACZ (PART OF THE SVARLANG LIBRARY)\r\n");
  fprintf(fd, "%s svarlang_memsz\r\n", public);
  fprintf(fd, "svarlang_memsz dw %u\r\n", allocsz);
  fprintf(fd, "%s svarlang_string_count\r\n", public);
  fprintf(fd, "svarlang_string_count dw %u\r\n\r\n", l->num_strings);
  fprintf(fd, "%s svarlang_mem\r\n", public);
  fprintf(fd, "svarlang_mem:\r\n");

  if (strings_bytes > 0) fprintf(fd, "db ");

  for (i = 0; i < strings_bytes; i++) {
    if (!fprintf(fd, "%u", l->strings[i])) {
      fclose(fd);
      return(0);
    }

    nextnlat++;
    if (l->strings[i] == '\0' || nextnlat == 16) {
      fprintf(fd, "\r\n");
      if (i + 1 < strings_bytes ) fprintf(fd, "db ");
      nextnlat = 0;
    }
    else {
      fprintf(fd, ",");
    }
  }

  fprintf(fd, "\r\n%s svarlang_dict\r\n", public);
  fprintf(fd, "svarlang_dict:\r\n");
  for (i = 0; i < l->num_strings; i++) {
    if (!fprintf(fd, "dw %u,%u\r\n", l->dict[i].id, l->dict[i].offset)) {
      fclose(fd);
      return(0);
    }
  }

  fclose(fd);

  return(1);
}


int main(int argc, char **argv) {
  FILE *fd;
  int ecode = 0;
  int i, output_format = C_OUTPUT;
  int mvcomp_enabled = 1;
  int excref = 0;
  int allocmargin = 1;
  unsigned short biggest_langsz = 0;
  struct svl_lang *lang = NULL, *reflang = NULL;

  if (argc < 2) {
    puts("tlumacz ver " SVARLANGVER " - this tool is part of the SvarLANG project.");
    puts("converts a set of CATS-style translations in files EN.TXT, PL.TXT, etc");
    puts("into a single resource file (OUT.LNG). Also generates a deflang source");
    puts("file that contains a properly sized buffer pre-filled with the first");
    puts("(reference) language.");
    puts("");
    puts("usage: tlumacz [/c|/asm|/nasm|/nodef] [/nocomp] [/excref] [/nomarg]");
    puts("               en fr pl ...");
    puts("");
    puts("/c        generates deflang.c (default)");
    puts("/asm      deflang ASM output");
    puts("/nasm     deflang NASM output");
    puts("/nodef    does NOT generate a deflang source file (only an LNG file)");
    puts("/nocomp   disables the MVCOMP compression of strings in the LNG file");
    puts("/excref   excludes ref lang from the LNG file (inserted to deflang only)");
    puts("/nomarg   disables the 5% safety margin of the deflang buffer");
    return(1);
  }

  fd = fopen("out.lng", "wb");
  if (fd == NULL) {
    fprintf(stderr, "ERROR: FAILED TO CREATE OR OPEN OUT.LNG");
    return(1);
  }

  /* write lang blocks */
  for (i = 1; i < argc; i++) {
    unsigned short sz;
    char id[3];

    if (!strcmp(argv[i], "/c")) {
      output_format = C_OUTPUT;
      continue;
    } else if (!strcmp(argv[i], "/asm")) {
      output_format = ASM_OUTPUT;
      continue;
    } else if(!strcmp(argv[i], "/nasm")) {
      output_format = NASM_OUTPUT;
      continue;
    } else if(!strcmp(argv[i], "/nocomp")) {
      mvcomp_enabled = 0;
      continue;
    } else if(!strcmp(argv[i], "/nodef")) {
      output_format = NO_OUTPUT;
      continue;
    } else if(!strcmp(argv[i], "/excref")) {
      excref = 1;
      continue;
    } else if(!strcmp(argv[i], "/nomarg")) {
      allocmargin = 0;
      continue;
    }

    if (strlen(argv[i]) != 2) {
      fprintf(stderr, "INVALID LANG SPECIFIED: %s\r\n", argv[i]);
      ecode = 1;
      goto exit_main;
    }
    id[0] = argv[i][0];
    id[1] = argv[i][1];
    id[2] = 0;

    if ((lang = svl_lang_new(id, DICT_CAP, STRINGS_CAP)) == NULL) {
      fprintf(stderr, "OUT OF MEMORY\r\n");
      ecode = 1;
      goto exit_main;
    }

    sz = svl_lang_from_cats_file(lang, reflang);
    if (sz == 0) {
      fprintf(stderr, "ERROR COMPUTING LANG '%s'\r\n", id);
      ecode = 1;
      goto exit_main;
    } else {
      printf("computed %s lang block of %u bytes\r\n", id, sz);
      if (sz > biggest_langsz) biggest_langsz = sz;
    }
    svl_compact_lang(lang);

    /* write header if first (reference) language */
    if (!reflang) {
      if (!svl_write_header(lang->num_strings, fd)) {
        fprintf(stderr, "ERROR WRITING TO OUTPUT FILE\r\n");
        ecode = 1;
        goto exit_main;
      }
    }

    /* write lang ID to file, followed string table size, and then
       the dictionary and string table for current language
       skip this for reference language if /excref given */
    if ((reflang != NULL) || (excref == 0)) {
      if (!svl_write_lang(lang, fd, mvcomp_enabled)) {
        fprintf(stderr, "ERROR WRITING TO OUTPUT FILE\r\n");
        ecode = 1;
        goto exit_main;
      }
    } else {
      puts("ref language NOT saved in the LNG file (/excref)");
    }

    /* remember reference data for other languages */
    if (!reflang) {
      reflang = lang;
    } else {
      svl_lang_free(lang);
      lang = NULL;
    }
  }

  if (!reflang) {
    fprintf(stderr, "ERROR: NO LANGUAGE GIVEN\r\n");
    ecode = 1;
    goto exit_main;
  }

  /* compute the deflang file containing a dump of the reference lang block */
  if (output_format == C_OUTPUT) {
    i = svl_write_c_source(reflang, "deflang.c", biggest_langsz, allocmargin);
    if (i == 0) fprintf(stderr, "ERROR: FAILED TO OPEN OR CREATE DEFLANG.C\r\n");
    if (i != 1) ecode = 1;
  } else if ((output_format == ASM_OUTPUT) || (output_format == NASM_OUTPUT)) {
    i = svl_write_asm_source(reflang, "deflang.inc", biggest_langsz, output_format, allocmargin);
    if (i == 0) fprintf(stderr, "ERROR: FAILED TO OPEN OR CREATE DEFLANG.INC\r\n");
    if (i != 1) ecode = 1;
  }

exit_main:
  if (lang && (lang != reflang)) {
    svl_lang_free(lang);
  }
  if (reflang) {
    svl_lang_free(reflang);
    reflang = NULL;
    lang = NULL;
  }

  fclose(fd);

  return(ecode);
}
