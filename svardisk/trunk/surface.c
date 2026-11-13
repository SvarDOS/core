/* SVARDISK surface read. Bad marks cover a whole cluster.
 * Copyright (C) 2026 Mateusz Viste. MIT license, see svardisk.c.
 *
 * /S reads clusters that are not marked bad. /R reads clusters that are.
 * A free cluster that cannot be read is marked bad when repairs are
 * written. A cluster that is in use is only reported: the bad mark would
 * replace the link to the rest of the chain. A bad cluster that reads
 * completely is marked free. The link it used to hold is already gone.
 */

#include "svardisk.h"

static u8 buf[512];

static u32 bad_mark(void) {
  if (vol.fat_type == 12) return(0x0ff7ul);
  if (vol.fat_type == 16) return(0xfff7ul);
  return(0x0ffffff7ul);
}

static int cluster_ok(u32 c) {
  u32 s;
  u8 i;

  s = cluster_sector(c);
  for (i = 0; i < vol.spc; i++) {
    if (io_probe(s + i, buf) != 0) return(0);
  }
  return(1);
}

static void note_cluster(u16 minor, u32 c) {
  note_begin();
  note_msg(4, minor);
  note_chr(' ');
  note_num(c);
  note_end();
}

int surface_scan(void) {
  u32 c;
  u32 v;

  if (!vol.surface && !vol.retest) return(0);
  out_msg(3, 11);
  for (c = 2; c <= vol.max_cluster; c++) {
    if (vol.io_error) return(-1);
    v = fat_get(c);
    if (vol.io_error) return(-1);
    if (fat_is_bad(v)) {
      if (!vol.retest) continue;
      if (!cluster_ok(c)) continue;
      note_cluster(26, c);
      if (vol.fix) {
        fat_set(c, 0);
        if (vol.io_error) return(-1);
        if (vol.bad_count > 0) vol.bad_count--;
        vol.free_count++;
      }
      continue;
    }
    if (!vol.surface) continue;
    if (cluster_ok(c)) continue;
    if (v == 0) {
      note_cluster(24, c);
      if (vol.fix) {
        fat_set(c, bad_mark());
        if (vol.io_error) return(-1);
        if (vol.free_count > 0) vol.free_count--;
        vol.bad_count++;
      }
    } else {
      note_cluster(25, c);
      if (vol.unfixed < 65535u) vol.unfixed++;
    }
  }
  return(vol.io_error ? -1 : 0);
}
