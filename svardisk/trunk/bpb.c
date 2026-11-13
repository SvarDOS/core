/* SVARDISK boot sector and FSInfo.
 * Copyright (C) 2026 Mateusz Viste. MIT license, see svardisk.c.
 */

#include "svardisk.h"

static u8 boot[512];
static u8 info[512];
static int fsinfo_need;

static int type_matches(const u8 *p) {
  const char *expect;
  const char *alt;
  int i;

  if (vol.fat_type == 12) expect = "FAT12   ";
  else if (vol.fat_type == 16) expect = "FAT16   ";
  else expect = "FAT32   ";
  for (i = 0; i < 8; i++) {
    if (p[i] != (u8)expect[i]) break;
  }
  if (i == 8) return(1);
  if (vol.fat_type == 32) return(0);
  alt = "FAT     ";
  for (i = 0; i < 8; i++) {
    if (p[i] != (u8)alt[i]) return(0);
  }
  return(1);
}

static const char *type_text(void) {
  if (vol.fat_type == 12) return("FAT12   ");
  if (vol.fat_type == 16) return("FAT16   ");
  return("FAT32   ");
}

static int type_at(void) {
  if (vol.fat_type == 32) return(82);
  return(54);
}

int bpb_load(void) {
  u16 bps;
  u16 fatsz16;
  u16 tot16;
  u16 root_ent;
  u32 fatsz;
  u32 total;
  u32 used;
  u32 data_sectors;
  u32 bytes;
  u32 fit;
  u8 spc;
  u8 nfats;
  u8 media;
  const u8 *ts;

  fsinfo_need = 0;
  if (io_read(0, 1, boot) != 0) return(-1);
  if (boot[510] != 0x55 || boot[511] != 0xAA) return(1);
  bps = RD16(boot + 11);
  if (bps != 512) return(2);

  spc = boot[13];
  if (spc == 0 || spc > 64) return(1);
  if ((spc & (u8)(spc - 1)) != 0) return(1);

  vol.reserved = RD16(boot + 14);
  nfats = boot[16];
  root_ent = RD16(boot + 17);
  tot16 = RD16(boot + 19);
  media = boot[21];
  fatsz16 = RD16(boot + 22);
  total = tot16 ? (u32)tot16 : RD32(boot + 32);
  fatsz = fatsz16 ? (u32)fatsz16 : RD32(boot + 36);

  if (vol.reserved == 0 || nfats == 0 || nfats > 2) return(1);
  if (fatsz == 0 || fatsz > total) return(1);
  if (total < 8) return(1);

  vol.spc = spc;
  vol.nfats = nfats;
  vol.media = media;
  vol.root_entries = root_ent;
  vol.fat_sectors = fatsz;
  vol.total_sectors = total;
  vol.root_sectors = (u16)((((u32)root_ent * 32ul) + 511ul) >> 9);
  vol.first_fat = vol.reserved;
  vol.root_sector = vol.reserved + (u32)nfats * fatsz;
  used = vol.root_sector + vol.root_sectors;
  if (used >= total) return(1);
  vol.data_start = used;
  data_sectors = total - used;
  vol.clusters = data_sectors / spc;
  if (vol.clusters < 1) return(1);
  vol.max_cluster = vol.clusters + 1;
  vol.cluster_bytes = (u16)((u16)spc * 512u);

  if (vol.clusters < 4085ul) vol.fat_type = 12;
  else if (vol.clusters < 65525ul) vol.fat_type = 16;
  else vol.fat_type = 32;

  if (vol.fat_type == 32 && root_ent != 0) return(1);
  if (vol.fat_type != 32 && root_ent == 0) return(1);

  bytes = fatsz << 9;
  if (fatsz > 0x400000ul) return(1);
  if (vol.fat_type == 12) fit = (bytes * 2ul) / 3ul;
  else if (vol.fat_type == 16) fit = bytes >> 1;
  else fit = bytes >> 2;
  if (fit < vol.clusters + 2ul) return(1);

  vol.root_cluster = 0;
  vol.fsinfo = 0;
  vol.backup_boot = 0;
  if (vol.fat_type == 32) {
    vol.root_cluster = RD32(boot + 44);
    if (vol.root_cluster < 2 || vol.root_cluster > vol.max_cluster) return(1);
    vol.fsinfo = RD16(boot + 48);
    vol.backup_boot = RD16(boot + 50);
    if (vol.fsinfo == 0 || vol.fsinfo == 0xffffu || vol.fsinfo >= vol.reserved)
      vol.fsinfo = 0;
    if (vol.backup_boot == 0 || vol.backup_boot == 0xffffu ||
        vol.backup_boot >= vol.reserved)
      vol.backup_boot = 0;
  }

  ts = boot + type_at();
  if (!type_matches(ts)) vol.typestr_bad = 1;
  return(0);
}

void bpb_notes(void) {
  if (vol.typestr_bad) note_line(4, 11);
}

int fsinfo_note(void) {
  u32 got;
  int bad;

  fsinfo_need = 0;
  if (vol.fat_type != 32 || vol.fsinfo == 0) return(0);
  if (io_read(vol.fsinfo, 1, info) != 0) return(-1);
  bad = 0;
  if (RD32(info) != 0x41615252ul) bad = 1;
  if (RD32(info + 484) != 0x61417272ul) bad = 1;
  if (info[510] != 0x55 || info[511] != 0xAA) bad = 1;
  got = RD32(info + 488);
  if (bad || (got != 0xfffffffful && got != vol.free_count)) {
    note_line(4, 12);
    fsinfo_need = 1;
  } else if (got != vol.free_count) {
    fsinfo_need = 1;
  }
  return(0);
}

int bpb_finish(void) {
  const char *text;
  int i;
  u32 next;
  u32 c;

  if (!vol.fix) return(0);
  if (vol.typestr_bad) {
    text = type_text();
    for (i = 0; i < 8; i++) boot[type_at() + i] = (u8)text[i];
    if (io_write(0, 1, boot) != 0) return(-1);
    if (vol.backup_boot != 0) {
      if (io_write(vol.backup_boot, 1, boot) != 0) return(-1);
    }
  }
  if (vol.fat_type == 32 && vol.fsinfo != 0 &&
      (fsinfo_need || RD32(info + 488) != vol.free_count)) {
    if (RD32(info) != 0x41615252ul || RD32(info + 484) != 0x61417272ul) {
      mem_set(info, 0, 512);
      WR32(info, 0x41615252ul);
      WR32(info + 484, 0x61417272ul);
      info[510] = 0x55;
      info[511] = 0xAA;
    }
    next = 0xfffffffful;
    for (c = 2; c <= vol.max_cluster; c++) {
      if (fat_get(c) == 0) {
        next = c;
        break;
      }
    }
    WR32(info + 488, vol.free_count);
    WR32(info + 492, next);
    if (io_write(vol.fsinfo, 1, info) != 0) return(-1);
  }
  return(0);
}
