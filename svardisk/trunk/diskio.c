/* SVARDISK sector I/O. DOS uses INT 21h AX=7305h for drives.
 * Copyright (C) 2026 Mateusz Viste. MIT license, see svardisk.c.
 */

#include "svardisk.h"

#ifdef SV_HOST
#include <stdio.h>
#else
static u8 pkt[10];
static u16 img_handle;
#endif

static int opened;
static int writable;

#ifdef SV_HOST
static FILE *img;
#endif

static void mark_io(int writing) {
  vol.fix = 0;
  if (vol.io_error == 0) vol.io_error = writing ? 2 : 1;
}

#ifdef SV_HOST

int io_open_drive(u8 drive1) {
  (void)drive1;
  return(-1);
}

int io_open_image(const char *path, int want_write) {
  io_close();
  img = fopen(path, want_write ? "rb+" : "rb");
  if (img == 0) return(-1);
  opened = 1;
  writable = want_write;
  vol.is_image = 1;
  return(0);
}

void io_close(void) {
  if (img != 0) {
    fclose(img);
    img = 0;
  }
  opened = 0;
  writable = 0;
}

static int io_one(u32 sector, void *buf, int writing) {
  u32 off;
  if (!opened || img == 0) {
    mark_io(writing);
    return(-1);
  }
  if (writing && !writable) {
    mark_io(1);
    return(-1);
  }
  if (sector > 0x7ffffful) {
    mark_io(writing);
    return(-1);
  }
  off = sector << 9;
  if (fseek(img, (long)off, SEEK_SET) != 0) {
    mark_io(writing);
    return(-1);
  }
  if (writing) {
    if (fwrite(buf, 1, 512, img) != 512) {
      mark_io(1);
      return(-1);
    }
  } else if (fread(buf, 1, 512, img) != 512) {
    mark_io(0);
    return(-1);
  }
  return(0);
}

void io_reset(void) {
  if (img != 0) fflush(img);
}

#else

static u16 dos_open(const char *path, u8 mode);
#pragma aux dos_open = \
  "mov ah, 3dh" \
  "int 21h" \
  "jnc ok" \
  "xor ax, ax" \
  "ok:" \
  parm [dx] [al] \
  value [ax] \
  modify [ah]

static void dos_close(u16 h);
#pragma aux dos_close = \
  "mov ah, 3eh" \
  "int 21h" \
  parm [bx] \
  modify [ax]

static u16 img_read(u16 h, void *buf, u16 n);
#pragma aux img_read = \
  "mov ah, 3fh" \
  "int 21h" \
  "jnc ok" \
  "xor ax, ax" \
  "ok:" \
  parm [bx] [dx] [cx] \
  value [ax] \
  modify [ah]

static u16 img_write(u16 h, const void *buf, u16 n);
#pragma aux img_write = \
  "mov ah, 40h" \
  "int 21h" \
  "jnc ok" \
  "xor ax, ax" \
  "ok:" \
  parm [bx] [dx] [cx] \
  value [ax] \
  modify [ah]

static u16 dos_seek(u16 h, u16 hi, u16 lo);
#pragma aux dos_seek = \
  "mov ax, 4200h" \
  "int 21h" \
  "sbb ax, ax" \
  parm [bx] [cx] [dx] \
  value [ax] \
  modify [dx]

static u16 get_ds(void);
#pragma aux get_ds = "mov ax, ds" value [ax]

static u16 dos_abs(u8 drive1, u16 mode, void *packet);
#pragma aux dos_abs = \
  "push ds" \
  "mov ax, 7305h" \
  "mov cx, 0ffffh" \
  "int 21h" \
  "pop ds" \
  "sbb ax, ax" \
  parm [dl] [si] [bx] \
  value [ax] \
  modify [cx]

static void dos_reset(void);
#pragma aux dos_reset = "mov ah, 0dh" "int 21h" modify [ax]

static u8 drive_cur(void);
#pragma aux drive_cur = \
  "mov ah, 19h" \
  "int 21h" \
  "inc al" \
  value [al] \
  modify [ah]

int io_open_drive(u8 drive1) {
  io_close();
  if (drive1 == 0) drive1 = drive_cur();
  vol.drive1 = drive1;
  vol.is_image = 0;
  opened = 1;
  writable = 1;
  return(0);
}

int io_open_image(const char *path, int want_write) {
  u16 h;
  io_close();
  h = dos_open(path, (u8)(want_write ? 2 : 0));
  if (h == 0) return(-1);
  img_handle = h;
  opened = 1;
  writable = want_write;
  vol.is_image = 1;
  return(0);
}

void io_close(void) {
  if (img_handle != 0) {
    dos_close(img_handle);
    img_handle = 0;
  }
  opened = 0;
  writable = 0;
}

static void put_u32(u8 *p, u32 v) {
  p[0] = (u8)v;
  p[1] = (u8)(v >> 8);
  p[2] = (u8)(v >> 16);
  p[3] = (u8)(v >> 24);
}

static int io_one(u32 sector, void *buf, int writing) {
  u32 off;
  u16 rc;
  if (!opened) {
    mark_io(writing);
    return(-1);
  }
  if (writing && !writable) {
    mark_io(1);
    return(-1);
  }
  if (vol.is_image) {
    if (sector > 0x7ffffful) {
      mark_io(writing);
      return(-1);
    }
    off = sector << 9;
    rc = dos_seek(img_handle, (u16)(off >> 16), (u16)off);
    if (rc != 0) {
      mark_io(writing);
      return(-1);
    }
    if (writing) rc = img_write(img_handle, buf, 512);
    else rc = img_read(img_handle, buf, 512);
    if (rc != 512) {
      mark_io(writing);
      return(-1);
    }
    return(0);
  }
  put_u32(pkt, sector);
  pkt[4] = 1;
  pkt[5] = 0;
  pkt[6] = (u8)((u16)buf);
  pkt[7] = (u8)(((u16)buf) >> 8);
  rc = get_ds();
  pkt[8] = (u8)rc;
  pkt[9] = (u8)(rc >> 8);
  if (dos_abs(vol.drive1, (u16)(writing ? 1 : 0), pkt) != 0) {
    mark_io(writing);
    return(-1);
  }
  return(0);
}

void io_reset(void) {
  dos_reset();
}

#endif

int io_read(u32 sector, u16 count, void *buf) {
  u16 i;
  u8 *p;
  p = (u8 *)buf;
  for (i = 0; i < count; i++) {
    if (io_one(sector + i, p, 0) != 0) return(-1);
    p += 512;
  }
  return(0);
}

int io_write(u32 sector, u16 count, const void *buf) {
  u16 i;
  u8 *p;
  p = (u8 *)buf;
  for (i = 0; i < count; i++) {
    if (io_one(sector + i, p, 1) != 0) return(-1);
    p += 512;
  }
  return(0);
}
