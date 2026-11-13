/*
 * CHOICE - wait for one of a set of keys and return it in ERRORLEVEL.
 *
 * MS-DOS 6.2 compatible (switches /C /N /S /T, prompt rules, errorlevels).
 * Messages and the default keys come from SvarLANG.
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
 * Build notes: WMINCRT must be assembled with NOARGV. The parser reads the
 * raw PSP tail so quotes and spaces inside the prompt survive. argv[] would
 * split them.
 */

#include "wmincrt.h"
#include "svarlang.h"


/* PSP command tail is 127 bytes. These buffers live in BSS. */
static unsigned char choices[128];
static unsigned char prompt[128];
static char selfname[128];
static char lang[3];

static unsigned char show;
static unsigned char casesens;
static unsigned char hastime;
static unsigned char timeout;
static unsigned char defkey;
static unsigned char helpme;
static unsigned char gotprompt;


static void putch(unsigned char c);
#pragma aux putch = \
  "mov ah, 2" \
  "int 21h" \
  parm [dl] modify [ax]

static void doswrite(const char *s, unsigned n);
#pragma aux doswrite = \
  "mov ah, 40h" \
  "mov bx, 1" \
  "int 21h" \
  parm [dx] [cx] modify [ax bx]

static unsigned char getch(void);
#pragma aux getch = \
  "mov ah, 8" \
  "int 21h" \
  value [al] modify [ah]

/* AL = FFh when a key is waiting, 0 otherwise. Also honors Ctrl+C. */
static unsigned char key_ready(void);
#pragma aux key_ready = \
  "mov ah, 0bh" \
  "int 21h" \
  value [al] modify [ah]

static unsigned char dos_sec(void);
#pragma aux dos_sec = \
  "mov ah, 2ch" \
  "int 21h" \
  "mov al, dh" \
  value [al] modify [ah cx dx]

/* INT 2F/1680: give up the CPU while polling. Unsupported calls return at once. */
static void idle(void);
#pragma aux idle = \
  "mov ax, 1680h" \
  "int 2fh" \
  modify [ax bx cx dx si di]

/* DOS 4+ country uppercase of one character. ASCII fallback if unsupported. */
static unsigned char upchar(unsigned char c);
#pragma aux upchar = \
  "mov ax, 6520h" \
  "int 21h" \
  "jnc ok" \
  "cmp dl, 'a'" \
  "jb ok" \
  "cmp dl, 'z'" \
  "ja ok" \
  "sub dl, 20h" \
  "ok:" \
  "mov al, dl" \
  parm [dl] value [al] modify [ah dx]

/* Returns 0 on success, non-zero if DOS has no NLS uppercase. */
static unsigned nls_upstr(char *s);
#pragma aux nls_upstr = \
  "mov ax, 6522h" \
  "int 21h" \
  "sbb ax, ax" \
  parm [dx] value [ax] modify [bx cx si di]

static unsigned short psp_env(unsigned short psp);
#pragma aux psp_env = \
  "mov es, ax" \
  "mov ax, es:[2ch]" \
  parm [ax] value [ax] modify [es]

/* Far pointer to offset 0 of a segment, without a 32-bit shift helper. */
static unsigned char __far *seg0(unsigned short seg);
#pragma aux seg0 = \
  "mov dx, ax" \
  "xor ax, ax" \
  parm [ax] value [dx ax]


static void outstr(const char *s) {
  unsigned n = 0;
  while (s[n]) n++;
  if (n) doswrite(s, n);
}

static void upstr(char *s) {
  if (nls_upstr(s) == 0) return;
  while (*s) {
    if ((*s >= 'a') && (*s <= 'z')) *s = (char)(*s - 32);
    s++;
  }
}

static unsigned char asciiup(unsigned char c) {
  if ((c >= 'a') && (c <= 'z')) c = (unsigned char)(c - 32);
  return(c);
}

static void crlf(void) {
  putch(13);
  putch(10);
}

static void out_line(const char *s) {
  outstr(s);
  crlf();
}

/* Four spaces, the syntax line, and a blank line. */
static void out_form(void) {
  outstr("    ");
  out_line(svarlang_str(1, 1));
  crlf();
}

/* 2.0 is the one MS-DOS error that is not prefixed. The prefix is not
 * part of the translation. */
static int fail(unsigned short id, int form) {
  if (id != 0x0200) outstr("CHOICE: ");
  out_line(svarlang_strid(id));
  if (form) out_form();
  return(255);
}


/* LANG (2 letters) and the executable path, both from the environment. */
static void load_nls(void) {
  unsigned short envseg;
  unsigned char __far *p;
  unsigned i;

  lang[0] = 0;
  selfname[0] = 0;
  envseg = psp_env(crt_psp_seg);
  if (envseg == 0) return;

  p = seg0(envseg);
  while (p[0] != 0) {
    if (((p[0] == 'L') || (p[0] == 'l')) &&
        ((p[1] == 'A') || (p[1] == 'a')) &&
        ((p[2] == 'N') || (p[2] == 'n')) &&
        ((p[3] == 'G') || (p[3] == 'g')) &&
        (p[4] == '=')) {
      lang[0] = (char)p[5];
      if (lang[0] != 0) {
        lang[1] = (char)p[6];
        lang[2] = 0;
      }
    }
    while (p[0] != 0) p++;
    p++;
  }
  p++;          /* second NUL */
  p += 2;       /* string count */
  for (i = 0; (i < 127) && (p[i] != 0); i++) selfname[i] = (char)p[i];
  selfname[i] = 0;

  if ((lang[0] != 0) && (selfname[0] != 0)) {
    svarlang_autoload_exepath(selfname, lang);
  }
}


static void load_default_choices(void) {
  const char *d;
  unsigned i;

  d = svarlang_str(3, 0);
  if (d[0] == 0) d = "yn";
  for (i = 0; (d[i] != 0) && (i < 127); i++) choices[i] = (unsigned char)d[i];
  choices[i] = 0;
}


/* Raw PSP tail. Switches may be glued (/N/S) and may follow the prompt.
 * An unquoted prompt runs until '/' or CR and keeps its spaces. A quoted
 * prompt runs until the next quote. A second prompt is an error. */
static int parse(void) {
  unsigned char len;
  unsigned char *s;
  unsigned char *end;
  unsigned i;

  show = 1;
  load_default_choices();

  len = (unsigned char)crt_cmdline[0];
  s = (unsigned char *)(crt_cmdline + 1);
  end = s + len;

  while ((s < end) && (*s != '\r')) {
    unsigned char sw;

    if ((*s == ' ') || (*s == '\t')) {
      s++;
      continue;
    }

    if (*s != '/') {
      if (gotprompt) return(fail(0x0204, 1));
      gotprompt = 1;
      i = 0;
      if (*s == '"') {
        s++;
        while ((s < end) && (*s != '\r') && (*s != '"')) {
          if (i < 127) prompt[i++] = *s;
          s++;
        }
        if ((s < end) && (*s == '"')) s++;
      } else {
        while ((s < end) && (*s != '\r') && (*s != '/')) {
          if (i < 127) prompt[i++] = *s;
          s++;
        }
      }
      prompt[i] = 0;
      continue;
    }

    s++;
    if ((s >= end) || (*s == '\r')) return(fail(0x0200, 1));
    sw = asciiup(*s);
    s++;

    if (sw == '?') {
      helpme = 1;
      return(0);
    }
    if (sw == 'N') {
      show = 0;
      continue;
    }
    if (sw == 'S') {
      casesens = 1;
      continue;
    }
    if (sw == 'C') {
      if ((s < end) && (*s == ':')) s++;
      i = 0;
      while ((s < end) && (*s != ' ') && (*s != '\t') && (*s != '\r') && (*s != '/')) {
        if (i < 127) choices[i++] = *s;
        s++;
      }
      choices[i] = 0;
      if (i == 0) return(fail(0x0201, 0));
      continue;
    }
    if (sw == 'T') {
      unsigned char n;
      if ((s < end) && (*s == ':')) s++;
      if ((s >= end) || (*s == '\r') || (*s == ' ') || (*s == '\t') || (*s == '/')) {
        return(fail(0x0202, 0));
      }
      defkey = *s;
      s++;
      if ((s >= end) || (*s != ',')) return(fail(0x0202, 0));
      s++;
      if ((s >= end) || (*s < '0') || (*s > '9')) return(fail(0x0202, 0));
      n = (unsigned char)(*s - '0');
      s++;
      if ((s < end) && (*s >= '0') && (*s <= '9')) {
        n = (unsigned char)(n * 10 + (*s - '0'));
        s++;
      }
      timeout = n;
      hastime = 1;
      continue;
    }
    return(fail(0x0200, 1));
  }
  return(0);
}


/* One NLS string per line. Blank lines and the title are printed here. */
static void show_help(void) {
  unsigned char tmp[16];
  const char *d;
  unsigned i;

  out_line("SvarDOS CHOICE");
  crlf();
  out_line(svarlang_str(1, 0));
  crlf();
  out_line(svarlang_str(1, 1));
  crlf();
  outstr(svarlang_str(1, 2));
  putch(' ');
  d = svarlang_str(3, 0);
  if (d[0] == 0) d = "yn";
  for (i = 0; (d[i] != 0) && (i < 15); i++) tmp[i] = (unsigned char)d[i];
  tmp[i] = 0;
  upstr((char *)tmp);
  outstr((char *)tmp);
  crlf();
  for (i = 3; i < 7; i++) out_line(svarlang_str(1, i));
  crlf();
  out_line(svarlang_str(1, 7));
  crlf();
}


static int choice_has(unsigned char c) {
  unsigned i;
  for (i = 0; choices[i] != 0; i++) {
    if (choices[i] == c) return(1);
  }
  return(0);
}


static void show_prompt(void) {
  unsigned i;

  if (prompt[0] != 0) outstr((char *)prompt);
  if (show == 0) return;
  putch('[');
  for (i = 0; choices[i] != 0; i++) {
    if (i != 0) putch(',');
    putch(choices[i]);
  }
  putch(']');
  putch('?');
}


/* Once any key arrives, the timeout is cancelled, including a key that is
 * not a valid choice. /T with nn = 0 selects the default with no wait. */
static int read_choice(void) {
  unsigned char use_def = 0;

  if (hastime) {
    if (timeout == 0) {
      use_def = 1;
    } else {
      unsigned char last = dos_sec();
      unsigned char left = timeout;
      for (;;) {
        unsigned char now;
        if (key_ready() != 0) break;
        now = dos_sec();
        if (now != last) {
          last = now;
          left--;
          if (left == 0) {
            use_def = 1;
            break;
          }
        }
        idle();
      }
    }
  }

  for (;;) {
    unsigned char k;
    unsigned i;

    if (use_def) {
      k = defkey;
      use_def = 0;
    } else {
      k = getch();
      if (casesens == 0) k = upchar(k);
    }
    for (i = 0; choices[i] != 0; i++) {
      if (choices[i] == k) {
        putch(k);
        putch(13);
        putch(10);
        return((int)(i + 1));
      }
    }
    putch(7);
  }
}


int main(void) {
  int rc;

  load_nls();
  rc = parse();
  if (rc != 0) return(rc);
  if (helpme) {
    show_help();
    return(255);
  }
  if (casesens == 0) {
    upstr((char *)choices);
    if (hastime) defkey = upchar(defkey);
  }
  if (hastime && (choice_has(defkey) == 0)) return(fail(0x0203, 0));
  show_prompt();
  return(read_choice());
}
