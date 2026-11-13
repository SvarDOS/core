/* SVARDISK directory walk, chain claims, lost chains.
 * Copyright (C) 2026 Mateusz Viste. MIT license, see svardisk.c.
 *
 * Each cluster carries two bits. PRED means some FAT entry points at it.
 * OWN means a directory entry's chain has claimed it. A second claim is a
 * cross-link, or a loop when the cluster is already in this same chain.
 * After the walk, an allocated cluster with neither bit is a lost head.
 * An allocated cluster with PRED but not OWN is an orphan loop.
 * A chain that steps on a free cluster is cut there. A bad-cluster mark
 * stays in the table. A directory nested past MAX_DEPTH is not entered,
 * and lost chains are then left alone so those clusters are not renamed.
 */

#include "svardisk.h"

typedef struct {
  u32 cluster;
  u32 sector;
  u32 self;
  u32 parent_expect;
  u32 stop_at;
  u32 steps;
  u16 index;
  u16 sec_i;
  u16 sec_n;
  u16 entries;
  u8 is_root;
  u8 done;
} Frame;

static Frame stack[MAX_DEPTH];
static int sp;
static u8 secbuf[512];
static u32 loaded_sec = 0xfffffffful;
static u32 descend_at;
static u32 descend_stop;

static u32 lfn_sec[20];
static u8 lfn_idx[20];
static u8 lfn_n;
static u8 lfn_sum;
static u8 lfn_expect;
static u8 lfn_bad;
static u8 lfn_on;

static u16 file_seq;
static u8 saved_name[11];
static int depth_cut;

static int load_sec(u32 sec) {
  if (loaded_sec == sec) return(0);
  if (io_read(sec, 1, secbuf) != 0) return(-1);
  loaded_sec = sec;
  return(0);
}

static int save_sec(u32 sec) {
  if (!vol.fix) return(0);
  if (io_write(sec, 1, secbuf) != 0) return(-1);
  loaded_sec = sec;
  return(0);
}

static u8 *entry_ptr(u16 idx) {
  return(secbuf + (u16)(idx * 32));
}

static u32 ent_cluster(const u8 *e) {
  u32 c;
  c = RD16(e + 26);
  if (vol.fat_type == 32) c |= ((u32)RD16(e + 20) << 16);
  return(c);
}

static void set_cluster(u8 *e, u32 c, int *changed) {
  if (!vol.fix) return;
  WR16(e + 26, (u16)c);
  if (vol.fat_type == 32) WR16(e + 20, (u16)(c >> 16));
  *changed = 1;
}

static void set_size(u8 *e, u32 size, int *changed) {
  if (!vol.fix) return;
  WR32(e + 28, size);
  *changed = 1;
}

static void clear_start(u8 *e, int *changed) {
  if (!vol.fix) return;
  e[20] = 0;
  e[21] = 0;
  e[26] = 0;
  e[27] = 0;
  e[28] = 0;
  e[29] = 0;
  e[30] = 0;
  e[31] = 0;
  *changed = 1;
}

static int spaces_from(const u8 *n, int i) {
  for (; i < 11; i++) {
    if (n[i] != ' ') return(0);
  }
  return(1);
}

static int is_dot(const u8 *n) {
  return(n[0] == '.' && n[1] == ' ' && spaces_from(n, 2));
}

static int is_dotdot(const u8 *n) {
  return(n[0] == '.' && n[1] == '.' && spaces_from(n, 2));
}

static u8 lfn_checksum(const u8 *n) {
  u8 sum;
  int i;
  sum = 0;
  for (i = 0; i < 11; i++) {
    sum = (u8)(((sum & 1) ? 0x80 : 0) + (sum >> 1) + n[i]);
  }
  return(sum);
}

static void drop_lfn(void) {
  u8 i;
  u32 back;

  if (!lfn_on) return;
  note_line(4, 15);
  if (!vol.fix) {
    lfn_on = 0;
    lfn_n = 0;
    return;
  }
  back = loaded_sec;
  for (i = 0; i < lfn_n; i++) {
    if (load_sec(lfn_sec[i]) != 0) return;
    secbuf[lfn_idx[i] * 32] = 0xe5;
    if (save_sec(lfn_sec[i]) != 0) return;
  }
  lfn_on = 0;
  lfn_n = 0;
  if (back != 0xfffffffful) load_sec(back);
}

static void take_lfn(const u8 *shortname) {
  int bad;
  if (!lfn_on) return;
  bad = lfn_bad;
  if (lfn_expect != 0) bad = 1;
  if (lfn_checksum(shortname) != lfn_sum) bad = 1;
  if (bad) drop_lfn();
  else {
    lfn_on = 0;
    lfn_n = 0;
  }
}

static void see_lfn(const u8 *e, u32 sector, u8 idx) {
  u8 id;
  u8 seq;
  u8 sum;

  id = e[0];
  seq = (u8)(id & 0x1f);
  sum = e[13];
  if (id & 0x40) {
    lfn_on = 1;
    lfn_bad = (seq == 0);
    lfn_n = 0;
    lfn_sum = sum;
    lfn_expect = seq;
  } else if (!lfn_on || seq == 0 || seq != lfn_expect || sum != lfn_sum) {
    lfn_on = 1;
    lfn_bad = 1;
  }
  if (lfn_n < 20) {
    lfn_sec[lfn_n] = sector;
    lfn_idx[lfn_n] = idx;
    lfn_n++;
  } else {
    lfn_bad = 1;
  }
  if (seq > 0) lfn_expect = (u8)(seq - 1);
}

static int in_this_chain(u32 start, u32 prev, u32 c) {
  u32 x;
  u32 guard;
  x = start;
  guard = 0;
  while (fat_valid(x)) {
    if (x == c) return(1);
    if (x == prev) return(0);
    x = fat_get(x);
    if (vol.io_error) return(0);
    if (++guard > vol.clusters) return(1);
  }
  return(0);
}

static int claim_chain(u32 *start, u32 *kept, u32 *at) {
  u32 c;
  u32 prev;
  u32 n;
  u32 guard;
  u32 v;

  *kept = 0;
  *at = *start;
  c = *start;
  prev = 0;
  n = 0;
  guard = 0;
  if (!fat_valid(c)) {
    *start = 0;
    return(CUT_BAD);
  }
  for (;;) {
    if (vol.io_error) return(CUT_IO);
    if (map_get(c) & BIT_OWN) {
      *at = c;
      if (prev != 0 && in_this_chain(*start, prev, c)) {
        if (vol.fix) fat_set(prev, fat_eof());
        return(CUT_LOOP);
      }
      if (prev != 0 && vol.fix) fat_set(prev, fat_eof());
      if (prev == 0) *start = 0;
      return(CUT_OWN);
    }
    v = fat_get(c);
    if (vol.io_error) return(CUT_IO);
    if (v == 0 || fat_is_bad(v)) {
      *at = c;
      if (prev != 0 && vol.fix) fat_set(prev, fat_eof());
      if (prev == 0) *start = 0;
      *kept = n;
      return(fat_is_bad(v) ? CUT_BAD : CUT_HOLE);
    }
    map_or(c, BIT_OWN);
    n++;
    *kept = n;
    if (fat_is_eof(v)) return(CUT_NONE);
    if (!fat_valid(v)) {
      *at = c;
      if (vol.fix) fat_set(c, fat_eof());
      /* The scan already reported this pointer. */
      return(CUT_NONE);
    }
    prev = c;
    c = v;
    if (++guard > vol.clusters) {
      *at = prev;
      if (vol.fix) fat_set(prev, fat_eof());
      return(CUT_LOOP);
    }
  }
}

static u32 chain_bytes(u32 n) {
  if (n == 0 || vol.cluster_bytes == 0) return(0);
  if (n > (0xfffffffful / vol.cluster_bytes)) return(0xfffffffful);
  return(n * (u32)vol.cluster_bytes);
}

static void note_cross(const u8 *name, u32 at) {
  note_begin();
  note_msg(4, 20);
  note_chr(' ');
  note_nam(name);
  note_chr(' ');
  note_msg(4, 6);
  note_chr(' ');
  note_num(at);
  note_end();
}

static void note_loop(u32 at) {
  note_begin();
  note_msg(4, 14);
  note_chr(' ');
  note_num(at);
  note_end();
}

static void note_badfile(const u8 *name) {
  note_begin();
  note_msg(4, 20);
  note_chr(' ');
  note_nam(name);
  note_chr(' ');
  note_msg(4, 21);
  note_end();
}

static void note_toobig(const u8 *name) {
  note_begin();
  note_msg(4, 20);
  note_chr(' ');
  note_nam(name);
  note_chr(' ');
  note_msg(4, 8);
  note_end();
}

static void note_broken(const u8 *name) {
  note_begin();
  note_msg(4, 20);
  note_chr(' ');
  note_nam(name);
  note_chr(' ');
  note_msg(4, 22);
  note_end();
}

static int advance(Frame *f) {
  u32 next;

  f->index = 0;
  f->sec_i++;
  if (f->sec_i < f->sec_n) {
    f->sector++;
    return(0);
  }
  if (f->cluster == 0) return(1);
  next = fat_get(f->cluster);
  if (vol.io_error) return(-1);
  if (!fat_valid(next)) return(1);
  if (f->stop_at != 0 && next == f->stop_at) return(1);
  f->steps++;
  if (f->steps > vol.clusters) return(1);
  f->cluster = next;
  f->sec_i = 0;
  f->sec_n = vol.spc;
  f->sector = cluster_sector(next);
  return(0);
}

static void push_dir(u32 cluster, u32 parent_expect, u32 stop_at) {
  Frame *f;

  if (sp >= MAX_DEPTH) {
    note_line(4, 19);
    depth_cut = 1;
    return;
  }
  f = &stack[sp++];
  f->cluster = cluster;
  f->sector = cluster_sector(cluster);
  f->self = cluster;
  f->parent_expect = parent_expect;
  f->stop_at = stop_at;
  f->steps = 0;
  f->index = 0;
  f->sec_i = 0;
  f->sec_n = vol.spc;
  f->entries = 0;
  f->is_root = 0;
  f->done = 0;
  lfn_on = 0;
  lfn_n = 0;
}

static void do_entry(Frame *f, u16 idx, int *changed) {
  u8 *e;
  u32 c;
  u32 start;
  u32 kept;
  u32 at;
  u32 size;
  int reason;

  e = entry_ptr(idx);
  if (e[0] == 0xe5) {
    if (lfn_on) drop_lfn();
    return;
  }
  if ((e[11] & 0x3f) == 0x0f) {
    see_lfn(e, f->sector, (u8)idx);
    return;
  }
  take_lfn(e);
  if (vol.io_error) return;
  e = entry_ptr(idx);

  if (!f->is_root && f->entries == 0) {
    if (is_dot(e)) {
      if (ent_cluster(e) != f->self) {
        note_line(4, 9);
        set_cluster(e, f->self, changed);
      }
      f->entries++;
      return;
    }
    note_line(4, 9);
  }
  if (!f->is_root && f->entries == 1) {
    if (is_dotdot(e)) {
      if (ent_cluster(e) != f->parent_expect) {
        note_line(4, 9);
        set_cluster(e, f->parent_expect, changed);
      }
      f->entries++;
      return;
    }
    note_line(4, 9);
  }
  f->entries++;

  if ((e[11] & 0x18) == 0x08) {
    if (!f->is_root) note_line(4, 9);
    return;
  }

  c = ent_cluster(e);
  size = RD32(e + 28);
  if (c == 0) {
    if (!(e[11] & 0x10) && size != 0) {
      note_toobig(e);
      set_size(e, 0, changed);
    } else if (e[11] & 0x10) {
      note_line(4, 9);
    }
    return;
  }

  start = c;
  reason = claim_chain(&start, &kept, &at);
  if (reason == CUT_IO || vol.io_error) return;
  if (start == 0 || kept == 0) {
    if (reason == CUT_OWN) note_cross(e, at);
    else if (reason == CUT_LOOP) note_loop(at);
    else if (reason == CUT_HOLE) note_broken(e);
    else note_badfile(e);
    clear_start(e, changed);
    return;
  }
  if (reason == CUT_OWN) note_cross(e, at);
  if (reason == CUT_LOOP) note_loop(at);
  if (reason == CUT_HOLE) note_broken(e);
  if (reason == CUT_BAD) note_badfile(e);

  if (e[11] & 0x10) {
    if (size != 0) {
      note_line(4, 9);
      set_size(e, 0, changed);
    }
    descend_at = start;
    descend_stop = (!vol.fix && reason == CUT_LOOP) ? at : 0;
    return;
  }

  if (size > chain_bytes(kept)) {
    note_toobig(e);
    set_size(e, chain_bytes(kept), changed);
  }
}

static int root_walk(int what, const u8 *name, u32 *sec, u16 *idx) {
  u32 cluster;
  u32 sector;
  u16 sec_i;
  u16 sec_n;
  u32 guard;

  if (vol.fat_type == 32) {
    cluster = vol.root_cluster;
    sector = cluster_sector(cluster);
    sec_n = vol.spc;
  } else {
    cluster = 0;
    sector = vol.root_sector;
    sec_n = vol.root_sectors;
  }
  sec_i = 0;
  guard = 0;
  for (;;) {
    u16 i;
    if (load_sec(sector) != 0) return(-1);
    for (i = 0; i < 16; i++) {
      u8 *e;
      e = entry_ptr(i);
      if (what == 1 && (e[0] == 0 || e[0] == 0xe5)) {
        *sec = sector;
        *idx = i;
        return(0);
      }
      if (e[0] == 0) return(what == 1 ? 1 : 0);
      if (e[0] == 0xe5) continue;
      if ((e[11] & 0x3f) == 0x0f) continue;
      if (what == 0 && mem_eq(e, name, 11)) return(1);
    }
    sec_i++;
    if (sec_i < sec_n) {
      sector++;
      continue;
    }
    if (cluster == 0) return(what == 1 ? 1 : 0);
    {
      u32 next;
      next = fat_get(cluster);
      if (vol.io_error) return(-1);
      if (!fat_valid(next)) return(what == 1 ? 1 : 0);
      cluster = next;
      sector = cluster_sector(cluster);
      sec_i = 0;
      sec_n = vol.spc;
    }
    if (++guard > vol.clusters) return(what == 1 ? 1 : 0);
  }
}

static u32 alloc_cluster(void) {
  u32 c;
  for (c = 2; c <= vol.max_cluster; c++) {
    if (fat_get(c) == 0) {
      fat_set(c, fat_eof());
      map_or(c, BIT_OWN);
      if (vol.free_count > 0) vol.free_count--;
      return(c);
    }
  }
  return(0);
}

static int extend_root(u32 *sec, u16 *idx) {
  u32 neu;
  u32 last;
  u32 next;
  u32 guard;
  u32 s;
  u16 i;

  if (vol.fat_type != 32) return(-1);
  neu = alloc_cluster();
  if (neu == 0) return(-1);
  last = vol.root_cluster;
  guard = 0;
  for (;;) {
    next = fat_get(last);
    if (!fat_valid(next)) break;
    last = next;
    if (++guard > vol.clusters) return(-1);
  }
  fat_set(last, neu);
  mem_set(secbuf, 0, 512);
  s = cluster_sector(neu);
  for (i = 0; i < vol.spc; i++) {
    if (io_write(s + i, 1, secbuf) != 0) return(-1);
  }
  loaded_sec = 0xfffffffful;
  *sec = s;
  *idx = 0;
  return(0);
}

static void make_chk(u8 *nm, u16 n) {
  nm[0] = 'F';
  nm[1] = 'I';
  nm[2] = 'L';
  nm[3] = 'E';
  nm[4] = (u8)('0' + (n / 1000) % 10);
  nm[5] = (u8)('0' + (n / 100) % 10);
  nm[6] = (u8)('0' + (n / 10) % 10);
  nm[7] = (u8)('0' + (n % 10));
  nm[8] = 'C';
  nm[9] = 'H';
  nm[10] = 'K';
}

static int place_lost(u32 cluster, u32 size) {
  u8 name[11];
  u8 ent[32];
  u16 n;
  u32 sec;
  u16 idx;
  int used;

  for (n = file_seq; n < 10000; n++) {
    make_chk(name, n);
    used = root_walk(0, name, 0, 0);
    if (used < 0) return(-1);
    if (used == 0) break;
  }
  if (n >= 10000) return(-1);
  file_seq = (u16)(n + 1);
  used = root_walk(1, 0, &sec, &idx);
  if (used < 0) return(-1);
  if (used != 0) {
    if (extend_root(&sec, &idx) != 0) return(-1);
  }
  mem_set(ent, 0, 32);
  mem_copy(ent, name, 11);
  ent[11] = 0x20;
  WR16(ent + 26, (u16)cluster);
  if (vol.fat_type == 32) WR16(ent + 20, (u16)(cluster >> 16));
  WR32(ent + 28, size);
  if (load_sec(sec) != 0) return(-1);
  mem_copy(entry_ptr(idx), ent, 32);
  if (save_sec(sec) != 0) return(-1);
  mem_copy(saved_name, name, 11);
  return(0);
}

static void harvest(u32 c, int is_loop) {
  u32 start;
  u32 kept;
  u32 at;
  int reason;

  start = c;
  reason = claim_chain(&start, &kept, &at);
  if (reason == CUT_IO || vol.io_error) return;
  note_begin();
  if (is_loop) {
    note_msg(4, 14);
    note_chr(' ');
    note_num(c);
  } else {
    note_msg(4, 5);
    note_chr(' ');
    note_num(c);
  }
  note_end();
  if (!vol.fix || start == 0 || kept == 0) return;
  if (place_lost(start, chain_bytes(kept)) == 0) {
    if (note_shown()) {
      out_msg(4, 16);
      out_chr(' ');
      out_dosname(saved_name);
      out_eol();
    }
  } else if (note_shown()) {
    out_msg(4, 17);
    out_eol();
  }
}

int walk_all(void) {
  Frame *f;
  u32 start;
  u32 kept;
  u32 at;
  int reason;

  sp = 0;
  depth_cut = 0;
  lfn_on = 0;
  lfn_n = 0;
  loaded_sec = 0xfffffffful;
  descend_at = 0;
  f = &stack[sp++];
  f->is_root = 1;
  f->done = 0;
  f->entries = 0;
  f->index = 0;
  f->sec_i = 0;
  f->steps = 0;
  f->stop_at = 0;
  f->parent_expect = 0;
  if (vol.fat_type == 32) {
    start = vol.root_cluster;
    reason = claim_chain(&start, &kept, &at);
    if (reason == CUT_IO || vol.io_error) return(-1);
    if (start == 0 || kept == 0) {
      if (reason == CUT_HOLE) note_line(4, 23);
      else note_line(4, 9);
      return(0);
    }
    if (reason == CUT_LOOP) note_loop(at);
    if (reason == CUT_HOLE) note_line(4, 23);
    f->cluster = start;
    f->self = start;
    f->sector = cluster_sector(start);
    f->sec_n = vol.spc;
    if (!vol.fix && reason == CUT_LOOP) f->stop_at = at;
  } else {
    f->cluster = 0;
    f->self = 0;
    f->sector = vol.root_sector;
    f->sec_n = vol.root_sectors;
  }

  while (sp > 0 && vol.io_error == 0) {
    u16 idx;
    int changed;
    f = &stack[sp - 1];
    if (f->done) {
      sp--;
      lfn_on = 0;
      lfn_n = 0;
      continue;
    }
    if (f->index >= 16) {
      if (advance(f) != 0) {
        if (lfn_on) drop_lfn();
        f->done = 1;
      }
      continue;
    }
    if (load_sec(f->sector) != 0) return(-1);
    idx = f->index;
    f->index++;
    if (secbuf[idx * 32] == 0) {
      if (lfn_on) drop_lfn();
      if (!f->is_root && f->entries < 2) note_line(4, 9);
      f->done = 1;
      continue;
    }
    changed = 0;
    descend_at = 0;
    descend_stop = 0;
    do_entry(f, idx, &changed);
    if (vol.io_error) return(-1);
    if (changed && save_sec(f->sector) != 0) return(-1);
    if (descend_at != 0) push_dir(descend_at, f->self, descend_stop);
  }
  return(vol.io_error ? -1 : 0);
}

int lost_and_loops(void) {
  u32 c;
  u32 v;

  if (depth_cut) return(0);
  for (c = 2; c <= vol.max_cluster; c++) {
    if (vol.io_error) return(-1);
    v = fat_get(c);
    if (v == 0 || fat_is_bad(v)) continue;
    if (map_get(c) & BIT_OWN) continue;
    if (map_get(c) & BIT_PRED) continue;
    harvest(c, 0);
  }
  for (c = 2; c <= vol.max_cluster; c++) {
    if (vol.io_error) return(-1);
    v = fat_get(c);
    if (v == 0 || fat_is_bad(v)) continue;
    if (map_get(c) & BIT_OWN) continue;
    harvest(c, 1);
  }
  return(vol.io_error ? -1 : 0);
}
