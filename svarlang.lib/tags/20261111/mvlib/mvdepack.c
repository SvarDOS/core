/*
 * MVCOMP decompressor
 * Copyright (C) 2024-2026 Mateusz Viste
 *
 * MIT License.
 *
 * The C decoder used by mvucomp. One word in, one to sixteen bytes out.
 * A literal token can ask for up to 15 raw words after it; that count is
 * the rawwords field and is the only state between calls.
 *
 * mvdepack.asm is the same decoder in 8086 code.
 *
 * http://mateusz.fr/mvcomp/
 */


#include <stddef.h>
#include "mvcomp.h"


void mvcomp_depack_init(struct mvcomp_depack *d) {
  if (d == NULL) return;
  d->rawwords = 0;
}


int mvcomp_depack_pending(const struct mvcomp_depack *d) {
  if (d == NULL) return(0);
  return(d->rawwords != 0);
}


/* Words are little-endian. See mvcomp.txt for the full rules.
 *
 * Pending raw words are consumed first, low byte then high byte, and are
 * not classified. Otherwise the token is LLLL OOOO OOOO OOOO:
 *   OOOO OOOO OOOO  distance minus one; decoder rewinds 1 .. 4096 bytes
 *   LLLL            copy length minus one; decoder copies 2 .. 16 bytes
 *
 * A high nibble of zero is a literal start, 0000 RRRR BBBB BBBB:
 *   BBBB BBBB  first literal byte (the low byte, stored first)
 *   RRRR       raw words that follow (0 .. 15), each two literal bytes
 * The run length 1 + 2*RRRR is always odd and at most 31.
 */
unsigned short mvcomp_depack_word(struct mvcomp_depack *d, unsigned char *dst,
                                  unsigned short word, const unsigned char *win) {
  unsigned char *dst_start;

  if ((d == NULL) || (dst == NULL)) return(0);
  dst_start = dst;

  /* raw word? low byte first; dst may sit on an odd address */
  if (d->rawwords != 0) {
    dst[0] = (unsigned char)(word & 0xff);
    dst[1] = (unsigned char)(word >> 8);
    d->rawwords--;
    return(2);

  /* literal byte? */
  } else if ((word & 0xF000) == 0) {
    *dst = (unsigned char)(word & 0xff);
    d->rawwords = (unsigned char)(word >> 8);
    return(1);

  /* else it's a back reference */
  } else {
    unsigned short dist = (word & 0x0FFF) + 1;
    unsigned short left;
    const unsigned char *src;

    /* compare the distance as an integer so a short output does not
     * form a pointer before win */
    if (win == NULL) return(0);
    if ((size_t)(dst - win) < dist) return(0);
    src = dst - dist;
    left = (unsigned short)(word >> 12);
    for (;;) {
      *dst = *src;
      dst++;   /* Copy MUST be done from left to right so self-duplicating  */
      src++;   /* strings are properly decoded (see mvcomp.txt for details) */
      if (left == 0) break;
      left--;
    }
  }

  return((unsigned short)(dst - dst_start));
}


unsigned long mvcomp_unpack(void *dst, unsigned long dst_sz,
                            const void *src, unsigned long src_len) {
  struct mvcomp_depack d;
  const unsigned char *s;
  unsigned char *out;
  unsigned long left;
  unsigned long written;

  if (src_len == 0) return(0);
  if ((src == NULL) || (dst == NULL)) return(MVCOMP_ERR);
  /* a stream is a list of 16-bit words; a leftover byte is truncated */
  if ((src_len & 1ul) != 0) return(MVCOMP_ERR);

  mvcomp_depack_init(&d);
  s = (const unsigned char *)src;
  out = (unsigned char *)dst;
  left = src_len;
  written = 0;

  while (left >= 2ul) {
    unsigned short word;
    unsigned short need;
    unsigned short n;

    word = (unsigned short)s[0] | ((unsigned short)s[1] << 8);
    if (d.rawwords != 0) {
      need = 2;
    } else if ((word & 0xF000) == 0) {
      need = 1;
    } else {
      need = (unsigned short)((word >> 12) + 1);
    }
    if (written + need > dst_sz) return(MVCOMP_ERR);

    n = mvcomp_depack_word(&d, out, word, (const unsigned char *)dst);
    if (n == 0) return(MVCOMP_ERR);
    out += n;
    written += n;
    s += 2;
    left -= 2ul;
  }

  if (d.rawwords != 0) return(MVCOMP_ERR);
  return(written);
}
