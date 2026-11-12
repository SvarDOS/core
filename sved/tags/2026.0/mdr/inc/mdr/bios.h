/*
 * BIOS functions
 *
 * This file is part of Mateusz' DOS Routines <http://mateusz.fr/mdr>
 * Published under the terms of the MIT License, as stated below.
 *
 * Copyright (C) 2014-2026 Mateusz Viste
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
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
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#ifndef MDR_BIOS_H
#define MDR_BIOS_H

/* waits for ticks time (1 tick is roughly 55ms, an hour has 65543 ticks)
 * works on IBM PC, XT, AT - ie. it's always safe */
void mdr_bios_tickswait(unsigned short ticks);

/* returns the current BIOS tick counter (18.2 Hz, 1 tick is roughly 55ms, an
 * hour has 65543 ticks). works on IBM PC, XT, AT - ie. it's always safe */
unsigned short mdr_bios_ticks(void);

/* Enhanced keyboard.
 *
 * mdr_bios_kbd_detect() is required before mdr_bios_kbd_getscan(),
 * mdr_bios_kbd_scanpending(), and mdr_bios_kbd_ralt().
 * getscan and scanpending use the enhanced keyboard services. An original
 * PC BIOS does not implement them. The call returns at once and never
 * delivers a key, so a program that waits on them for input never wakes
 * up: the application is bricked and does not respond to the keyboard.
 * ralt reports Right Alt. That key is present only on an enhanced
 * keyboard, so the call cannot succeed when detect returns zero.
 * When mdr_bios_kbd_detect() returns zero, wait with mdr_dos_getkey() and
 * poll with mdr_dos_keypending() (both declared in mdr/dos.h).
 *
 * mdr_bios_kbd_scan2ch() does not touch the keyboard. */

/* non-zero when the BIOS reports an enhanced (101/102-key) keyboard.
 * bit 4 of 0040:0096. the BIOS that implements INT 16h AH=10h/11h/12h sets
 * this bit after the keyboard answers the enhanced-keyboard ID. an original
 * PC leaves the byte clear, and so does a later BIOS with an 84-key keyboard
 * attached.
 * REQUIRED before mdr_bios_kbd_getscan(), mdr_bios_kbd_scanpending(), and
 * mdr_bios_kbd_ralt(). zero means those three must not be called: wait with
 * mdr_dos_getkey() and poll with mdr_dos_keypending() instead. */
int mdr_bios_kbd_detect(void);

/* waits for a key and returns the BIOS INT 16h AH=10h result.
 * high byte: set-1 scancode. it names the physical key and does not depend
 * on KEYB or the code page.
 * low byte: ASCII produced by the loaded KEYB driver, or 0 / 0xE0 when the
 * key is extended (arrows, delete, ...).
 * INT 16h does not check CTRL+C. pair this with mdr_bios_kbd_scanpending().
 * REQUIRES mdr_bios_kbd_detect() != 0. calling this when detect returned
 * zero bricks the program on an original PC: the BIOS does not wait, and
 * no key is ever delivered. */
unsigned short mdr_bios_kbd_getscan(void);

/* returns non-zero when a key is waiting in the BIOS keyboard buffer.
 * INT 16h AH=11h. does not consume the key.
 * REQUIRES mdr_bios_kbd_detect() != 0. on an original PC this does not look
 * at the keyboard buffer, so a loop that waits for it never sees a key and
 * the program stays unresponsive. */
int mdr_bios_kbd_scanpending(void);

/* non-zero when Right Alt is held (BIOS data area 0040:0096 bit 3).
 * on a 101-key keyboard this is the AltGr key.
 * REQUIRES mdr_bios_kbd_detect() != 0. an original PC has no Right Alt
 * key, so this cannot report one there. */
int mdr_bios_kbd_ralt(void);

/* physical letter positions for mdr_bios_kbd_scan2ch(). the id selects which
 * letter sits on a scancode. it is not the KEYB driver loaded in DOS.
 * values are stable: DE=0 EN=1 FR=2 PL=3 RU=4 */
enum MDR_KEYLAY {
  MDR_KEYLAY_DE = 0,
  MDR_KEYLAY_EN = 1,
  MDR_KEYLAY_FR = 2,
  MDR_KEYLAY_PL = 3,
  MDR_KEYLAY_RU = 4
};

/* lowercase letter for this set-1 scancode on layout, or 0 if that key is
 * not a letter there. shift and caps lock are ignored: the result is the
 * lowercase byte a dictionary would store. does not call INT 16h.
 * altgr matters for MDR_KEYLAY_PL only. pass mdr_bios_kbd_ralt().
 *
 * bytes:
 *   EN, and the plain letters of the other Latin layouts: ASCII a-z
 *   DE (QWERTZ, CP437): ä=0x84 ö=0x94 ü=0x81 ß=0xE1
 *   FR (France AZERTY, CP437): é=0x82 on the key labeled 2, è=0x8A on the
 *       key labeled 7, ê=0x88 on the key labeled ^
 *   PL (programmer QWERTY, Mazovia): AltGr plus a c e l n o s x z gives
 *       ą=0x86 ć=0x8D ę=0x91 ł=0x92 ń=0xA4 ó=0xA2 ś=0x9E ź=0xA6 ż=0xA7
 *   RU (ЙЦУКЕН, CP866 lowercase, ё=0xF1 on the key left of 1) */
unsigned char mdr_bios_kbd_scan2ch(int layout, unsigned char scan, int altgr);

#endif
