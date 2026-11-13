/* SVARDISK - check and repair FAT12, FAT16 and FAT32.
 *
 * Copyright (C) 2026 Mateusz Viste
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Build: Open Watcom 1.9, small-model EXE, WMINCRT, SvarLANG. 8086 code.
 */

#include "svardisk.h"
#include "svarlang.h"

#ifdef SV_HOST
#include <stdlib.h>
#else
#include "wmincrt.h"
#endif

static int help_only;
static int saw_fix;
static int saw_surface;
static int saw_retest;
static const char *target;

static void show_help(void) {
  out_msg(1, 0);
  out_msg(1, 1);
  out_msg(1, 2);
}

static int up_letter(char c) {
  if (c >= 'a' && c <= 'z') return(c - 32);
  return(c);
}

static int is_drive(const char *s, char *letter, u8 *drive1) {
  int c;
  c = up_letter(s[0]);
  if (c < 'A' || c > 'Z') return(0);
  if (s[1] == 0 || (s[1] == ':' && s[2] == 0)) {
    *letter = (char)c;
    *drive1 = (u8)(c - 'A' + 1);
    return(1);
  }
  return(0);
}

#ifdef SV_HOST

static void load_lang(const char *argv0) {
  const char *lang;
  lang = getenv("LANG");
  if (argv0 != 0 && argv0[0] != 0) svarlang_autoload_exepath(argv0, lang);
}

static char exe_drive(void) {
  return(0);
}

#else

static u16 psp_env(u16 psp);
#pragma aux psp_env = \
  "mov es, ax" \
  "mov ax, es:[2ch]" \
  parm [ax] \
  value [ax] \
  modify [es]

static u8 __far *seg0(u16 seg);
#pragma aux seg0 = \
  "mov dx, ax" \
  "xor ax, ax" \
  parm [ax] \
  value [dx ax]

static void load_lang(const char *argv0) {
  u16 env;
  u8 __far *p;
  char lang[3];

  lang[0] = 0;
  lang[1] = 0;
  lang[2] = 0;
  if (crt_psp_seg == 0) return;
  env = psp_env(crt_psp_seg);
  if (env == 0) return;
  p = seg0(env);
  while (p[0] != 0) {
    if ((p[0] == 'L' || p[0] == 'l') &&
        (p[1] == 'A' || p[1] == 'a') &&
        (p[2] == 'N' || p[2] == 'n') &&
        p[3] == '=') {
      if (p[4] != 0) {
        lang[0] = (char)p[4];
        lang[1] = (char)p[5];
        lang[2] = 0;
      }
    }
    while (p[0] != 0) p++;
    p++;
  }
  if (lang[0] != 0 && argv0 != 0 && argv0[0] != 0)
    svarlang_autoload_exepath(argv0, lang);
}

static char exe_drive(void) {
  const char *a;
  int c;
  a = crt_argv[0];
  if (a != 0 && a[0] != 0 && a[1] == ':') {
    c = up_letter(a[0]);
    if (c >= 'A' && c <= 'Z') return((char)c);
  }
  return(0);
}

#endif

#ifdef SV_HOST
/* A DOS switch is "/F". A Linux image path is "/tmp/disk.img". */
static int host_path(const char *s) {
  const char *p;
  if (s[0] != '/' || s[1] == 0 || s[2] == 0) return(0);
  for (p = s + 1; *p != 0; p++) {
    if (*p == '/') return(1);
  }
  return(0);
}
#endif

static int parse(int argc, char **argv) {
  int i;
  help_only = 0;
  saw_fix = 0;
  saw_surface = 0;
  saw_retest = 0;
  target = 0;
  for (i = 1; i < argc; i++) {
    char *a;
    int sw;
    a = argv[i];
    if (a[0] == 0) continue;
#ifdef SV_HOST
    if (host_path(a)) {
      if (target != 0) return(-1);
      target = a;
      continue;
    }
#endif
    if (a[0] == '/') {
      if (a[1] == 0) return(-1);
      sw = up_letter(a[1]);
      if (a[2] != 0) return(-1);
      if (sw == '?') {
        help_only = 1;
        continue;
      }
      if (sw == 'F') {
        saw_fix = 1;
        continue;
      }
      if (sw == 'S') {
        saw_surface = 1;
        continue;
      }
      if (sw == 'R') {
        saw_retest = 1;
        continue;
      }
      return(-1);
    }
    if (target != 0) return(-1);
    target = a;
  }
  return(0);
}

static int open_target(void) {
  char letter;
  u8 drive1;

  vol.fix = saw_fix;
  vol.surface = saw_surface;
  vol.retest = saw_retest;
  if (target == 0) {
#ifdef SV_HOST
    out_msg(2, 1);
    return(-1);
#else
    if (io_open_drive(0) != 0) {
      out_msg(2, 1);
      return(-1);
    }
    vol.drive_letter = (char)('A' + vol.drive1 - 1);
    vol.is_image = 0;
    return(0);
#endif
  }
  if (is_drive(target, &letter, &drive1)) {
#ifdef SV_HOST
    out_msg(2, 8);
    return(-1);
#else
    vol.drive_letter = letter;
    if (io_open_drive(drive1) != 0) {
      out_msg(2, 2);
      return(-1);
    }
    return(0);
#endif
  }
  if (io_open_image(target, saw_fix) != 0) {
    out_msg(2, 8);
    return(-1);
  }
  return(0);
}

static void say_io(void) {
  out_msg(2, vol.io_error == 2 ? 6 : 5);
}

static int run_check(void) {
  int kind;
  char mine;

  kind = bpb_load();
  if (kind == -1) {
    out_msg(2, 2);
    return(3);
  }
  if (kind == 2) {
    out_msg(2, 9);
    return(3);
  }
  if (kind != 0) {
    out_msg(2, 3);
    return(3);
  }

  mine = exe_drive();
  if (vol.fix && !vol.is_image && mine != 0 && mine == vol.drive_letter) {
    out_msg(2, 7);
    return(3);
  }

  out_msg(3, 0);
  out_chr(' ');
  if (vol.is_image) out_str(target);
  else {
    out_chr(vol.drive_letter);
    out_chr(':');
  }
  out_eol();
  if (vol.fat_type == 12) out_msg(3, 1);
  else if (vol.fat_type == 16) out_msg(3, 2);
  else out_msg(3, 3);
  out_msg(3, 4);
  out_chr(' ');
  out_u32(vol.clusters);
  out_msg(3, 5);
  out_msg(3, vol.fix ? 7 : 6);
  bpb_notes();

  if (map_alloc(vol.max_cluster) != 0) {
    out_msg(2, 4);
    return(3);
  }

  out_msg(3, 8);
  if (fat_compare() != 0 || vol.io_error) {
    say_io();
    return(3);
  }
  if (fat_scan() != 0 || vol.io_error) {
    fat_flush();
    say_io();
    return(3);
  }

  out_msg(3, 9);
  if (walk_all() != 0 || vol.io_error) {
    fat_flush();
    say_io();
    return(3);
  }

  out_msg(3, 10);
  if (lost_and_loops() != 0 || vol.io_error) {
    fat_flush();
    say_io();
    return(3);
  }
  if (fsinfo_note() != 0 || vol.io_error) {
    fat_flush();
    say_io();
    return(3);
  }
  if (surface_scan() != 0 || vol.io_error) {
    fat_flush();
    say_io();
    return(3);
  }

  if (vol.fix) {
    if (fat_finish() != 0 || bpb_finish() != 0 || vol.io_error) {
      fat_flush();
      say_io();
      return(3);
    }
  }

  out_msg(5, 0);
  out_chr(' ');
  out_u32(vol.free_count);
  out_eol();
  if (vol.problems == 0) {
    out_msg(4, 0);
    return(0);
  }
  /* An in-use cluster is reported and left alone. If that was the only
   * problem and the table was not written, /F did not repair anything. */
  if (vol.fix && (vol.problems != vol.unfixed || vol.fat_dirty)) {
    out_msg(4, 2);
    out_chr(' ');
    out_u32(vol.problems);
    out_eol();
    if (!vol.is_image) out_msg(4, 3);
    io_reset();
    return(1);
  }
  out_msg(4, 1);
  out_chr(' ');
  out_u32(vol.problems);
  out_eol();
  return(2);
}

int main(int argc, char **argv) {
  int rc;

  mem_set(&vol, 0, (u16)sizeof(vol));
  load_lang(argv[0]);
  if (parse(argc, argv) != 0) {
    out_msg(2, 0);
    return(3);
  }
  if (help_only) {
    show_help();
    return(0);
  }
  if (open_target() != 0) return(3);
  rc = run_check();
  fat_flush();
  io_close();
  map_free();
  return(rc);
}
