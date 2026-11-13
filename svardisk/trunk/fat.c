/* SVARDISK FAT cache, scan and mirror copy.
 * The whole table is never held in RAM. A few sectors are cached.
 * Copyright (C) 2026 Mateusz Viste. MIT license, see svardisk.c.
 */

#include "svardisk.h"

#define CACHE_N 4

static u8 cache[CACHE_N][512];
static u32 cache_sec[CACHE_N];
static u8 cache_on[CACHE_N];
static u8 cache_dirty[CACHE_N];
static int cache_next;
static int cache_last;
static int flushing;
static u8 raw1[512];
static u8 raw2[512];

static u8 *cache_get(u32 sec) {
  int i;

  if (vol.io_error) return(0);
  for (i = 0; i < CACHE_N; i++) {
    if (cache_on[i] && cache_sec[i] == sec) {
      cache_last = i;
      return(cache[i]);
    }
  }
  i = cache_next;
  cache_next++;
  if (cache_next >= CACHE_N) cache_next = 0;
  if (cache_on[i] && cache_dirty[i]) {
    if (io_write(cache_sec[i], 1, cache[i]) != 0) return(0);
    cache_dirty[i] = 0;
  }
  if (io_read(sec, 1, cache[i]) != 0) return(0);
  cache_on[i] = 1;
  cache_sec[i] = sec;
  cache_dirty[i] = 0;
  cache_last = i;
  return(cache[i]);
}

static void cache_mark(void) {
  cache_dirty[cache_last] = 1;
  vol.fat_dirty = 1;
}

int fat_flush(void) {
  int i;

  if (flushing) return(-1);
  flushing = 1;
  for (i = 0; i < CACHE_N; i++) {
    if (cache_on[i] && cache_dirty[i]) {
      if (io_write(cache_sec[i], 1, cache[i]) != 0) {
        flushing = 0;
        return(-1);
      }
      cache_dirty[i] = 0;
    }
  }
  flushing = 0;
  return(0);
}

static void locate(u32 c, u32 *sec, u16 *inner) {
  u32 off;

  if (vol.fat_type == 12) off = c + (c >> 1);
  else if (vol.fat_type == 16) off = c << 1;
  else off = c << 2;
  *sec = vol.first_fat + (off >> 9);
  *inner = (u16)(off & 511);
}

u32 cluster_sector(u32 c) {
  return(vol.data_start + (c - 2) * (u32)vol.spc);
}

u32 fat_eof(void) {
  if (vol.fat_type == 12) return(0x0ffful);
  if (vol.fat_type == 16) return(0xfffful);
  return(0x0ffffffful);
}

int fat_is_eof(u32 v) {
  if (vol.fat_type == 12) return(v >= 0x0ff8ul);
  if (vol.fat_type == 16) return(v >= 0xfff8ul);
  return((v & 0x0ffffffful) >= 0x0ffffff8ul);
}

int fat_is_bad(u32 v) {
  if (vol.fat_type == 12) return(v == 0x0ff7ul);
  if (vol.fat_type == 16) return(v == 0xfff7ul);
  return((v & 0x0ffffffful) == 0x0ffffff7ul);
}

int fat_valid(u32 v) {
  return(v >= 2 && v <= vol.max_cluster);
}

u32 fat_get(u32 c) {
  u32 sec;
  u16 inner;
  u8 *p;
  u16 v16;
  u8 lo;

  if (vol.io_error) return(0);
  locate(c, &sec, &inner);
  p = cache_get(sec);
  if (p == 0) return(0);
  if (vol.fat_type == 12) {
    if (inner < 511) {
      v16 = (u16)(p[inner] | ((u16)p[inner + 1] << 8));
    } else {
      lo = p[511];
      p = cache_get(sec + 1);
      if (p == 0) return(0);
      v16 = (u16)(lo | ((u16)p[0] << 8));
    }
    if (c & 1) v16 = (u16)(v16 >> 4);
    return((u32)(v16 & 0x0fff));
  }
  if (vol.fat_type == 16) {
    return((u32)(p[inner] | ((u16)p[inner + 1] << 8)));
  }
  return((u32)p[inner] | ((u32)p[inner + 1] << 8) |
         ((u32)p[inner + 2] << 16) | ((u32)p[inner + 3] << 24));
}

void fat_set(u32 c, u32 v) {
  u32 sec;
  u16 inner;
  u8 *p;
  u16 word;
  u8 lo;
  u8 hi;
  u32 old;

  if (!vol.fix || vol.io_error) return;
  locate(c, &sec, &inner);
  p = cache_get(sec);
  if (p == 0) return;
  if (vol.fat_type == 12) {
    v &= 0x0fff;
    if (inner < 511) {
      word = (u16)(p[inner] | ((u16)p[inner + 1] << 8));
      if (c & 1) word = (u16)((word & 0x000f) | ((u16)v << 4));
      else word = (u16)((word & 0xf000) | (u16)v);
      p[inner] = (u8)word;
      p[inner + 1] = (u8)(word >> 8);
      cache_mark();
      return;
    }
    lo = p[511];
    p = cache_get(sec + 1);
    if (p == 0) return;
    hi = p[0];
    word = (u16)(lo | ((u16)hi << 8));
    if (c & 1) word = (u16)((word & 0x000f) | ((u16)v << 4));
    else word = (u16)((word & 0xf000) | (u16)v);
    p[0] = (u8)(word >> 8);
    cache_mark();
    p = cache_get(sec);
    if (p == 0) return;
    p[511] = (u8)word;
    cache_mark();
    return;
  }
  if (vol.fat_type == 16) {
    p[inner] = (u8)v;
    p[inner + 1] = (u8)(v >> 8);
    cache_mark();
    return;
  }
  old = (u32)p[inner] | ((u32)p[inner + 1] << 8) |
        ((u32)p[inner + 2] << 16) | ((u32)p[inner + 3] << 24);
  v = (v & 0x0ffffffful) | (old & 0xf0000000ul);
  p[inner] = (u8)v;
  p[inner + 1] = (u8)(v >> 8);
  p[inner + 2] = (u8)(v >> 16);
  p[inner + 3] = (u8)(v >> 24);
  cache_mark();
}

int fat_compare(void) {
  u32 s;
  u8 f;
  u32 other;

  if (vol.nfats < 2) return(0);
  for (s = 0; s < vol.fat_sectors; s++) {
    if (io_read(vol.first_fat + s, 1, raw1) != 0) return(-1);
    for (f = 1; f < vol.nfats; f++) {
      other = vol.first_fat + (u32)f * vol.fat_sectors + s;
      if (io_read(other, 1, raw2) != 0) return(-1);
      if (!mem_eq(raw1, raw2, 512)) {
        vol.mirror_diff = 1;
        note_line(4, 4);
        return(0);
      }
    }
  }
  return(0);
}

static int copy_mirrors(void) {
  u32 s;
  u8 f;
  u32 other;

  if (vol.nfats < 2) return(0);
  for (s = 0; s < vol.fat_sectors; s++) {
    if (io_read(vol.first_fat + s, 1, raw1) != 0) return(-1);
    for (f = 1; f < vol.nfats; f++) {
      other = vol.first_fat + (u32)f * vol.fat_sectors + s;
      if (io_write(other, 1, raw1) != 0) return(-1);
    }
  }
  return(0);
}

int fat_scan(void) {
  u32 c;
  u32 v;
  u32 e0;
  u32 e1;
  u32 bit;
  u32 repl;

  vol.free_count = 0;
  vol.bad_count = 0;
  for (c = 2; c <= vol.max_cluster; c++) {
    if (vol.io_error) return(-1);
    v = fat_get(c);
    if (v == 0) {
      vol.free_count++;
      continue;
    }
    if (fat_is_bad(v)) {
      vol.bad_count++;
      continue;
    }
    if (fat_is_eof(v)) continue;
    if (fat_valid(v)) {
      map_or(v, BIT_PRED);
      continue;
    }
    note_begin();
    note_msg(4, 7);
    note_chr(' ');
    note_num(c);
    note_end();
    if (vol.fix) fat_set(c, fat_eof());
  }
  if (vol.io_error) return(-1);

  e0 = fat_get(0);
  if ((e0 & 0xfful) != (u32)vol.media) {
    vol.media_bad = 1;
    note_line(4, 13);
    if (vol.fix) {
      if (vol.fat_type == 12) repl = 0x0f00ul | vol.media;
      else if (vol.fat_type == 16) repl = 0xff00ul | vol.media;
      else repl = 0x0fffff00ul | vol.media;
      fat_set(0, repl);
    }
  }

  if (vol.fat_type != 12) {
    e1 = fat_get(1);
    bit = (vol.fat_type == 16) ? 0x8000ul : 0x08000000ul;
    if ((e1 & bit) == 0) {
      vol.dirty_flag = 1;
      note_line(4, 10);
    }
  }
  return(vol.io_error ? -1 : 0);
}

int fat_finish(void) {
  u32 e1;

  if (vol.fat_type != 12 && vol.dirty_flag) {
    e1 = fat_get(1);
    if (vol.fat_type == 16) e1 |= 0xc000ul;
    else e1 = (e1 & 0x0ffffffful) | 0x0c000000ul;
    fat_set(1, e1);
  }
  if (fat_flush() != 0) return(-1);
  if (vol.mirror_diff || vol.fat_dirty) {
    if (copy_mirrors() != 0) return(-1);
  }
  return(0);
}
