/*
 * Library to access OPL2/OPL3 hardware (YM3812 / YMF262)
 *
 * This file is part of the Mateusz' DOS Routines (MDR): http://mdr.osdn.io
 * Published under the terms of the MIT License, as stated below.
 *
 * Copyright (C) 2015-2023 Mateusz Viste
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

#ifndef mdr_opl_h
#define mdr_opl_h

struct mdr_opl_timbre {
  unsigned char mod_ws, car_ws; /* waveform select (0-4), reg Exh */
  unsigned char mod_sr, car_sr; /* sustain/release, reg 8xh */
  unsigned char mod_ad, car_ad; /* attack/decay, reg 6xh */
  unsigned char mod_20, car_20; /* tremolo/vibrato/sustain..., reg 2xh */
  unsigned char mod_40, car_40; /* reg 4xh */
  unsigned char feedconn;
};

struct mdr_opl_timbretemplate {
  struct {
    unsigned char ws;      /* waveform select 0..3 */
    unsigned char sustlev; /* sustain level 0..15 */
    unsigned char release; /* release level 0..15 */
    unsigned char attack;  /* attack rate 0..15 */
    unsigned char decay;   /* decay rate 0..15 */
    unsigned char tremolo; /* tremolo flag 0..1 */
    unsigned char vibrato; /* vibrato flag 0..1 */
    unsigned char sustain; /* sustain flag 0..1 */
    unsigned char ksr;     /* KSR (envelope scaling) flag 0..1 */
    unsigned char mult;    /* frequency multiplication factor 0..15 */
    unsigned char ksl;     /* Key Scale Level 0..3 */
    unsigned char outlev;  /* output level 0..63 */
  } carrier;
  struct {
    unsigned char ws;      /* waveform select 0..3 */
    unsigned char sustlev; /* sustain level 0..15 */
    unsigned char release; /* release level 0..15 */
    unsigned char attack;  /* attack rate 0..15 */
    unsigned char decay;   /* decay rate 0..15 */
    unsigned char tremolo; /* tremolo flag 0..1 */
    unsigned char vibrato; /* vibrato flag 0..1 */
    unsigned char sustain; /* sustain flag 0..1 */
    unsigned char ksr;     /* KSR (envelope scaling) flag 0..1 */
    unsigned char mult;    /* frequency multiplication factor 0..15 */
    unsigned char ksl;     /* Key Scale Level 0..3 */
    unsigned char outlev;  /* output level 0..63 */
  } modultr;
  unsigned char feedback;/* FeedBack Modulation Factor 0..7 */
  unsigned char conn;    /* Synthesis type: 0=FM / 1=Additive */
};

enum MDR_OPL_TIMER {
  MDR_OPL_TIMER_80US  = 2,
  MDR_OPL_TIMER_320US = 3
};


/* frequency groups, to be used with mdr_opl_noteon() and mdr_opl_notebend().
 * There are 7 frequency groups to choose from. Each group supports a different
 * span of frequencies. Higher groups have wider spans, but at the cost of larger
 * difference between adjacent notes:
 *
 * Block     Note 0      Note 1023      Step gap between adjacent notes
 * FGROUP0   0.047 Hz     48.503 Hz     0.048 Hz
 * FGROUP1   0.094 Hz     97.006 Hz     0.095 Hz
 * FGROUP2   0.189 Hz    194.013 Hz     0.190 Hz
 * FGROUP3   0.379 Hz    388.026 Hz     0.379 Hz
 * FGROUP4   0.758 Hz    776.053 Hz     0.759 Hz
 * FGROUP5   1.517 Hz   1552.107 Hz     1.517 Hz
 * FGROUP6   3.034 Hz   3104.215 Hz     3.034 Hz
 * FGROUP7   6.068 Hz   6208.431 Hz     6.069 Hz
 *
 * This shows that block 7 is capable of reaching the highest note (6.2kHz) but
 * since there are 6 Hz between notes the accuracy suffers. Example: note A-4
 * is 440Hz but in this block, the two closest frequency numbers are 72 and 73,
 * which create tones at 437Hz and 443Hz respectively, neither of which is
 * particularly accurate. Blocks 3 and below are unable to reach as high as
 * 440Hz, but block 4 can. With block 4, frequency numbers 579 and 580 produce
 * 439.4 Hz and 440.2 Hz, considerably closer to the intended frequency.
 *
 * In other words, when calculating notes, the best accuracy is achieved by
 * selecting the lowest possible block number that can reach the desired note
 * frequency.
 *
 * More details: https://moddingwiki.shikadi.net/wiki/OPL_chip#A0-A8:_Frequency_Number
 */

enum mdr_opl_fgroup_t {
  MDR_OPL_FGROUP0 = 0,
  MDR_OPL_FGROUP1 = 1 << 2,
  MDR_OPL_FGROUP2 = 2 << 2,
  MDR_OPL_FGROUP3 = 3 << 2,
  MDR_OPL_FGROUP4 = 4 << 2,
  MDR_OPL_FGROUP5 = 5 << 2,
  MDR_OPL_FGROUP6 = 6 << 2,
  MDR_OPL_FGROUP7 = 7 << 2
};

/* Hardware detection and initialization. Must be called before any other
 * OPL function. Returns 0 on success, non-zero otherwise. */
int mdr_opl_init(void);

/* close OPL device */
void mdr_opl_close(void);

/* turns off all notes */
void mdr_opl_clear(void);

/* loads an instrument described by properties in a timbre_t struct into
 * the defined voice channel. The OPL2 chip supports up to 9 voice channels,
 * from 0 to 8. The timbre struct can be freed right after this call. */
void mdr_opl_loadinstrument(unsigned char voice, const struct mdr_opl_timbre *timbre);

/* generate a timbre structure based on a timbre template. this is a
 * convenience function meant to provide a human-compatible (more readable)
 * way of generating a timbre struct. */
int mdr_opl_timbre_gen(struct mdr_opl_timbre *timbre, const struct mdr_opl_timbretemplate *tpl);

/* Triggers a note on selected voice channel.
 * freqid is a value between 0 and 1023. The following formula can be used to
 * determine the freq number for a given note frequency (Hz) and block:
 *
 *   freqid = frequency * 2^(20 - block) / 49716
 *
 * The note will be kept "pressed" until mdr_opl_noteoff() is called. */
void mdr_opl_noteon(unsigned char voice, unsigned short freqid, enum mdr_opl_fgroup_t fgroup);

/* changes the frequency of the note currently playing on voice channel, this
 * can be used for pitch bending. */
void mdr_opl_notebend(unsigned char voice, unsigned short freqid, enum mdr_opl_fgroup_t fgroup);

/* releases a note on selected voice. */
void mdr_opl_noteoff(unsigned char voice);

/* adjusts volume of a voice. volume goes from 63 (mute) to 0 (loudest) */
void mdr_opl_voicevolume(unsigned char voice, unsigned char volume);

/* this is a LOW-LEVEL function that writes a data byte into the reg register
 * of the OPL chip. Use this only if you know exactly what you are doing. */
void mdr_opl_regwr(unsigned char reg, unsigned char data);


/*****************************************************************************
 *                          IMF AUDIO FILES PLAYBACK                         *
 *                                                                           *
 * It is possible to mix IMF playback calls with manual notes, but you must  *
 * take care to use only voices not used by your IMF audio. Typically games  *
 * tend to use the voice #0 for sound effects and voices #1 to #8 for music. *
 *                                                                           *
 * The IMF API comes in two version: the normal one, or "imfeasy". The easy  *
 * version is easier to use, but requires to have the entire IMF audio file  *
 * loaded in memory, while the normal (non-easy) allows for more flexibility *
 * in this regard, potentially allowing for playback of huge IMF files.      *
 *****************************************************************************/

/*** EASY INTERFACE ***/

/* playback initialization, easy interface. imf points to the start of the IMF
 * file. The imf pointer must not be freed as long as playback is ongoing.
 * imflength is the size (in bytes) of the IMF data.
 * clock must be an incrementing value that wraps to 0 after 65535. The clock
 * speed will control the playback's tempo.
 * loopscount tells how many times the song will have to be looped (0 means
 * "loop forever").
 * returns 0 on success, non-zero otherwise. */
int mdr_opl_imfeasy_init(void *imf, unsigned short imflength, unsigned short clock, unsigned char loopscount);

/* Playback of an IMF file preloaded via mdr_opl_imfeasy_init(). This function
 * must be called repeatedly at a high frequency for best playback quality.
 * Returns 0 on success, 1 if playback ended, -1 on error. */
int mdr_opl_imfeasy_play(unsigned short clock);

/*** ADVANCED INTERFACE ***/

/* playback initialization, this function must be called immediately before
 * playback. imf points to the start of the IMF file and must contain at least
 * the first 6 bytes of the audio file.
 * clock must be an incrementing value that wraps to 0 after 65535.
 * the clock speed will control the playback's tempo.
 * returns the amount of consumed bytes (0, 4 or 6) */
unsigned short mdr_opl_imf_init(void *imf, unsigned short clock);

/* Playback, advanced version. Feeds the IMF playback routine with IMF data.
 * Returns the amount of bytes that have been consumed, hence the next call
 * should provide an imf pointer advanced by this many bytes (and imflen
 * decreased accordingly). Such approach might not be the most intuitive, but
 * it allows to load an imf song partially and provide only short chunks of
 * data for playback instead of having to buffer the entire song in memory.
 * For a simpler call that requires to buffer the entire IMF file in memory,
 * see mdr_opl_imf_playeasy().
 * This function must be called repeatedly at a high frequency for best
 * playback quality. */
unsigned short mdr_opl_imf_play(void *imf, unsigned short imflen, unsigned short clock);


/*****************************************************************************
 * OPL TIMER FUNCTIONS                                                       *
 *****************************************************************************/

/* configures and starts a timer given type so it emits a tick every count
 * periods. Two timer types are available:
 *   MDR_OPL_TIMER_80US  - with a period of 80us
 *   MDR_OPL_TIMER_320US - with a period of 320us
 * count may range from 0 to 255, but 0 means "256 periods".
 *
 * You may use only one timer at a time.
 *
 * EXAMPLE: setting up MDR_OPL_TIMER_80US with a count of 25 would make the
 *          timer tick every 2ms (25 * 80us). */
void mdr_opl_timer_set(enum MDR_OPL_TIMER timertype, unsigned char count);

/* returns 1 if timer tick occured, 0 otherwise. After a tick has been
 * returned, this function will return 0 until next tick.
 *
 * it is important to note that there is no way to know whether one tick
 * passed since last time, or more, so it is up to you to make sure you call
 * this function fast enough. */
unsigned char mdr_opl_timer_tick(void);

#endif
