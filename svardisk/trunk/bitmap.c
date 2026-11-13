/* Two bits per cluster, in conventional memory.
 * Bit 0: some other cluster's FAT entry points here.
 * Bit 1: a directory walk has claimed this cluster.
 * Copyright (C) 2026 Mateusz Viste. MIT license, see svardisk.c.
 */

#include "svardisk.h"

#ifdef SV_HOST
#include <stdlib.h>
#include <string.h>
static u8 *map_bytes;
static u32 map_limit;
#else
#include "wmincrt.h"
static u16 map_seg;
static u16 map_paras;
static u32 map_limit;
#endif

#ifdef SV_HOST

int map_alloc(u32 max_cluster) {
  u32 n;
  map_free();
  n = ((max_cluster + 1ul) + 3ul) >> 2;
  map_bytes = (u8 *)malloc((size_t)n);
  if (map_bytes == 0) return(-1);
  memset(map_bytes, 0, (size_t)n);
  map_limit = max_cluster;
  return(0);
}

void map_free(void) {
  if (map_bytes != 0) {
    free(map_bytes);
    map_bytes = 0;
  }
  map_limit = 0;
}

static u8 *map_byte(u32 c, u8 *shift) {
  *shift = (u8)((c & 3) << 1);
  return(map_bytes + (c >> 2));
}

#else

static u16 dos_alloc(u16 paras);
#pragma aux dos_alloc = \
  "mov ah, 48h" \
  "int 21h" \
  "jnc ok" \
  "xor ax, ax" \
  "ok:" \
  parm [bx] \
  value [ax] \
  modify [ah]

static void dos_free(u16 seg);
#pragma aux dos_free = \
  "mov es, ax" \
  "mov ah, 49h" \
  "int 21h" \
  parm [ax] \
  modify [ax es]

static void zero_seg(u16 seg, u16 n) {
  u8 __far *p;
  u16 i;
  p = (u8 __far *)MK_FP(seg, 0);
  for (i = 0; i < n; i++) p[i] = 0;
}

int map_alloc(u32 max_cluster) {
  u32 bytes;
  u32 paras;
  u32 left;
  u16 seg;
  map_free();
  bytes = ((max_cluster + 1ul) + 3ul) >> 2;
  paras = (bytes + 15ul) >> 4;
  if (paras == 0) paras = 1;
  if (paras > 0xfffful) return(-1);
  map_seg = dos_alloc((u16)paras);
  if (map_seg == 0) return(-1);
  map_paras = (u16)paras;
  map_limit = max_cluster;
  left = (u32)map_paras << 4;
  seg = map_seg;
  while (left > 0) {
    u16 n;
    n = 32768;
    if (left < 32768ul) n = (u16)left;
    zero_seg(seg, n);
    left -= n;
    seg = (u16)(seg + (n >> 4));
  }
  return(0);
}

void map_free(void) {
  if (map_seg != 0) {
    dos_free(map_seg);
    map_seg = 0;
  }
  map_paras = 0;
  map_limit = 0;
}

static u8 __far *map_byte(u32 c, u8 *shift) {
  u32 bi;
  u16 seg;
  u16 off;
  bi = c >> 2;
  *shift = (u8)((c & 3) << 1);
  seg = (u16)(map_seg + (u16)(bi >> 4));
  off = (u16)(bi & 15);
  return((u8 __far *)MK_FP(seg, off));
}

#endif

u8 map_get(u32 c) {
  u8 sh;
#ifdef SV_HOST
  u8 *p;
#else
  u8 __far *p;
#endif
  if (c > map_limit) return(0);
#ifdef SV_HOST
  if (map_bytes == 0) return(0);
#else
  if (map_seg == 0) return(0);
#endif
  p = map_byte(c, &sh);
  return((u8)((*p >> sh) & 3));
}

void map_or(u32 c, u8 bits) {
  u8 sh;
  u8 cur;
#ifdef SV_HOST
  u8 *p;
#else
  u8 __far *p;
#endif
  if (c > map_limit) return;
  p = map_byte(c, &sh);
  cur = *p;
  cur = (u8)(cur | (u8)((bits & 3) << sh));
  *p = cur;
}
