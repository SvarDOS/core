/* SVARDISK - check and repair FAT12, FAT16 and FAT32.
 * Copyright (C) 2026 Mateusz Viste. MIT license, see svardisk.c.
 */

#ifndef SVARDISK_H
#define SVARDISK_H

#define MAX_DEPTH 40

#define CUT_NONE 0
#define CUT_OWN  1
#define CUT_LOOP 2
#define CUT_BAD  3
#define CUT_IO   4
#define CUT_HOLE 5

#define BIT_PRED 1
#define BIT_OWN  2

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;

#define RD16(p) ((u16)((p)[0] | ((u16)(p)[1] << 8)))
#define RD32(p) ((u32)((p)[0] | ((u32)(p)[1] << 8) | \
  ((u32)(p)[2] << 16) | ((u32)(p)[3] << 24)))

#define WR16(p, v) do { \
  (p)[0] = (u8)(v); \
  (p)[1] = (u8)((u16)(v) >> 8); \
} while (0)

#define WR32(p, v) do { \
  (p)[0] = (u8)(v); \
  (p)[1] = (u8)((u32)(v) >> 8); \
  (p)[2] = (u8)((u32)(v) >> 16); \
  (p)[3] = (u8)((u32)(v) >> 24); \
} while (0)

typedef struct {
  int is_image;
  int fix;
  int io_error;       /* 0 none, 1 read, 2 write */
  int fat_dirty;
  int mirror_diff;
  int typestr_bad;
  int dirty_flag;
  int media_bad;
  char drive_letter;
  u8 drive1;
  u8 fat_type;
  u8 spc;
  u8 nfats;
  u8 media;
  u16 reserved;
  u16 root_entries;
  u16 root_sectors;
  u16 fsinfo;
  u16 backup_boot;
  u16 problems;
  u16 cluster_bytes;
  u32 total_sectors;
  u32 fat_sectors;
  u32 first_fat;
  u32 root_sector;
  u32 data_start;
  u32 clusters;
  u32 max_cluster;
  u32 root_cluster;
  u32 free_count;
  u32 bad_count;
} Vol;

extern Vol vol;

void mem_set(void *p, u8 v, u16 n);
void mem_copy(void *d, const void *s, u16 n);
int mem_eq(const void *a, const void *b, u16 n);

void out_str(const char *s);
void out_chr(char c);
void out_eol(void);
void out_u32(u32 n);
void out_msg(u16 major, u16 minor);
void out_dosname(const u8 *n11);

void note_begin(void);
void note_msg(u16 major, u16 minor);
void note_chr(char c);
void note_num(u32 n);
void note_nam(const u8 *n11);
void note_end(void);
void note_line(u16 major, u16 minor);
int note_shown(void);

int io_open_drive(u8 drive1);
int io_open_image(const char *path, int writable);
void io_close(void);
int io_read(u32 sector, u16 count, void *buf);
int io_write(u32 sector, u16 count, const void *buf);
void io_reset(void);

int map_alloc(u32 max_cluster);
void map_free(void);
u8 map_get(u32 c);
void map_or(u32 c, u8 bits);

/* bpb_load: 0 ok, 1 not FAT, 2 bad sector size, -1 cannot read. */
int bpb_load(void);
void bpb_notes(void);
int fsinfo_note(void);
int bpb_finish(void);

u32 cluster_sector(u32 c);
u32 fat_get(u32 c);
void fat_set(u32 c, u32 v);
int fat_is_eof(u32 v);
int fat_is_bad(u32 v);
int fat_valid(u32 v);
u32 fat_eof(void);
int fat_compare(void);
int fat_scan(void);
int fat_flush(void);
int fat_finish(void);

int walk_all(void);
int lost_and_loops(void);

#endif
