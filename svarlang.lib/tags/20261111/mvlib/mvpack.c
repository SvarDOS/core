/*
 * MVCOMP compressor
 * Copyright (C) 2024-2026 Mateusz Viste
 *
 * MIT License.
 *
 * Fast compressor used by the mvcomp tool. A back reference costs two bytes
 * at every length, and the tail of a match is still a match, so the match
 * rules already produce a minimum-cost stream. Each match is found with an
 * index: the two bytes under the cursor are the hash bucket, and the chain
 * is the earlier occurrences inside the 4096-byte window, nearest first.
 *
 * mvslow.c repeats those decisions with a linear scan.
 *
 * The match index is 256KB. On 16-bit Watcom a normal far pointer covers one
 * 64KB segment, which is only the first quarter of the table, so the index
 * is a huge block from halloc. Elsewhere it is a plain malloc.
 *
 * http://mateusz.fr/mvcomp/
 */


#include <stdlib.h>
#include <string.h>
#include "mvcomp.h"

#if defined(__WATCOMC__) && !defined(__386__)
#include <malloc.h>
#define MVCOMP_HUGE __huge
#else
#define MVCOMP_HUGE
#endif

#define MV_HASH 65536ul
#define MV_WIN  4096u
/* History plus input that has been accepted but not yet coded.
 * Room past the window so a full history and a 16-byte lookahead still
 * leave space to take another byte. */
#define MV_CAP  (MV_WIN + 512u)


struct mvcomp {
  unsigned long MVCOMP_HUGE *head; /* 0 = empty, else stream position + 1 */
  unsigned long prev[MV_WIN];
  unsigned long pos;
  unsigned char win[MV_CAP];
  unsigned short hist; /* coded bytes kept in front of win */
  unsigned short fill; /* valid bytes in win */
  unsigned char litq[32];
  unsigned char litq_len;
  int done;
};


#if defined(__WATCOMC__) && !defined(__386__)
static unsigned long MVCOMP_HUGE *mv_index_alloc(void) {
  return(halloc((long)MV_HASH, sizeof(unsigned long)));
}

static void mv_index_free(unsigned long MVCOMP_HUGE *p) {
  hfree(p);
}
#else
static unsigned long *mv_index_alloc(void) {
  return(malloc(MV_HASH * sizeof(unsigned long)));
}

static void mv_index_free(unsigned long *p) {
  free(p);
}
#endif


static void mv_index_clear(unsigned long MVCOMP_HUGE *head) {
  unsigned long i;
  for (i = 0; i < MV_HASH; i++) head[i] = 0;
}


/* write qlen literal bytes. returns how many bytes were stored.
 * the queue is at most 31, and that encodes to 32 bytes. */
static unsigned mv_dump(unsigned char *dst, const unsigned char *q, unsigned qlen) {
  unsigned n = 0;

  for (;;) {
    unsigned words;
    unsigned bytes;

    if (qlen == 0) return(n);
    qlen--; /* now it's between 0 and 30 */
    dst[n] = q[0];
    dst[n + 1] = (unsigned char)(qlen / 2);
    q++;
    n += 2;
    if (qlen == 0) return(n);

    /* raw words, then one byte might still be left over */
    words = qlen / 2;
    bytes = words * 2;
    if (bytes != 0) {
      memcpy(dst + n, q, bytes);
      q += bytes;
      n += bytes;
      qlen -= bytes;
    }
  }
}


/* compare up to n bytes. returns how many leading bytes match. */
static unsigned short mv_compare(const unsigned char *s1, const unsigned char *s2, unsigned short n) {
  unsigned short i;
  for (i = 0; (i < n) && (s1[i] == s2[i]); i++);
  return(i);
}


/* the bucket is the two bytes themselves, so a chain holds only real pairs */
static unsigned mv_hash(const unsigned char *p) {
  return((unsigned)p[0] << 8) | p[1];
}


static void mv_note(struct mvcomp *c, const unsigned char *p, unsigned long pos) {
  unsigned h;
  h = mv_hash(p);
  c->prev[pos & (MV_WIN - 1u)] = c->head[h];
  c->head[h] = pos + 1ul;
}


/* note count bytes at s, but never a byte that has no successor to hash */
static void mv_note_span(struct mvcomp *c, const unsigned char *s, unsigned long pos,
                         unsigned count, unsigned long avail) {
  unsigned i;
  for (i = 0; (i < count) && ((unsigned long)i + 1ul < avail); i++) {
    mv_note(c, s + i, pos + i);
  }
}


/* nearest match of cap bytes, or else the nearest longest.
 * 0 if shorter than minmatch. */
static int mv_find(struct mvcomp *c, const unsigned char *src, unsigned long pos,
                   unsigned short maxoffset, unsigned short cap, unsigned short minmatch,
                   unsigned short *matchlen, unsigned short *offset) {
  unsigned long enc;
  unsigned short guard;
  unsigned short best;
  unsigned short bestdist;

  best = 0;
  bestdist = 0;
  enc = c->head[mv_hash(src)];
  for (guard = 0; (enc != 0) && (guard < MV_WIN); guard++) {
    unsigned long at;
    unsigned long dist;
    unsigned short n;

    at = enc - 1ul;
    if (at >= pos) break;
    dist = pos - at;
    if (dist > maxoffset) break;
    enc = c->prev[at & (MV_WIN - 1u)];
    n = mv_compare(src, src - dist, cap);
    if (n == cap) {
      *matchlen = cap;
      *offset = (unsigned short)dist;
      return(1);
    }
    if (n > best) {
      best = n;
      bestdist = (unsigned short)dist;
    }
  }
  if (best < minmatch) return(0);
  *matchlen = best;
  *offset = bestdist;
  return(1);
}


/* code the bytes at src. *len is how many are visible; on return it is how
 * many were not consumed. reserve bytes at the end are left alone unless
 * they are eaten by a match that was decided while at least 16 bytes were
 * still visible. reserve 0 finishes the input and flushes the literal queue.
 * history is the bytes immediately before src. */
static unsigned long mv_emit(struct mvcomp *c, unsigned char *dst, unsigned long dst_sz,
                             const unsigned char *src, unsigned long *len,
                             unsigned long reserve) {
  unsigned long complen = 0;

  while (*len > reserve) {
    unsigned short matchlen;
    unsigned short minmatch;
    unsigned short offset;
    unsigned short maxoffset;

    matchlen = 16;
    if (*len < matchlen) matchlen = (unsigned short)(*len);

    /* same rules as mvslow.c */
    if (c->litq_len & 1) {
      minmatch = 3;
    } else if (c->litq_len > 0) {
      goto NOMATCH;
    } else {
      minmatch = 2;
    }

    if (matchlen >= minmatch) {
      maxoffset = MV_WIN;
      if ((unsigned long)maxoffset > c->pos) maxoffset = (unsigned short)c->pos;
      if (mv_find(c, src, c->pos, maxoffset, matchlen, minmatch, &matchlen, &offset)) goto FOUND;
    }

    NOMATCH:

    /* a full queue dumps as 32 bytes. don't take the byte that fills it
     * unless that dump fits. */
    if ((c->litq_len == 30) && (complen + 32ul > dst_sz)) return(complen);

    mv_note_span(c, src, c->pos, 1, *len);
    c->litq[c->litq_len++] = *src;
    src++;
    c->pos++;
    *len -= 1;
    if (c->litq_len == 31) {
      unsigned n;
      n = mv_dump(dst, c->litq, c->litq_len);
      dst += n;
      complen += n;
      c->litq_len = 0;
    }
    continue;

    FOUND:

    {
      unsigned char tmp[40];
      unsigned need = 0;

      if (c->litq_len != 0) need = mv_dump(tmp, c->litq, c->litq_len);
      if (complen + need + 2ul > dst_sz) return(complen);
      if (need != 0) {
        memcpy(dst, tmp, need);
        dst += need;
        complen += need;
        c->litq_len = 0;
      }
      {
        unsigned short token;
        token = (unsigned short)(((unsigned)(matchlen - 1) << 12) | (unsigned)(offset - 1));
        dst[0] = (unsigned char)token;
        dst[1] = (unsigned char)(token >> 8);
      }
      dst += 2;

      mv_note_span(c, src, c->pos, matchlen, *len);
      src += matchlen;
      c->pos += matchlen;
      *len -= matchlen;
      complen += 2;
    }
  }

  if ((reserve == 0) && (c->litq_len != 0)) {
    unsigned char tmp[40];
    unsigned need;
    need = mv_dump(tmp, c->litq, c->litq_len);
    if (complen + need > dst_sz) return(complen);
    memcpy(dst, tmp, need);
    c->litq_len = 0;
    complen += need;
  }

  return(complen);
}


/* drop coded bytes that have fallen outside the 4096-byte window */
static void mv_slide(struct mvcomp *c) {
  unsigned short drop;

  if (c->hist <= MV_WIN) return;
  drop = (unsigned short)(c->hist - MV_WIN);
  memmove(c->win, c->win + drop, (size_t)(c->fill - drop));
  c->hist = (unsigned short)(c->hist - drop);
  c->fill = (unsigned short)(c->fill - drop);
}


static void mv_take(struct mvcomp *c, const unsigned char **src, unsigned long *left) {
  unsigned short space;
  unsigned long n;

  if (*left == 0) return;
  mv_slide(c);
  if (c->fill >= MV_CAP) return;
  space = (unsigned short)(MV_CAP - c->fill);
  n = *left;
  if (n > space) n = space;
  memcpy(c->win + c->fill, *src, (size_t)n);
  c->fill = (unsigned short)(c->fill + (unsigned short)n);
  *src += n;
  *left -= n;
}


/* reserve 0 codes the tail. Otherwise that many trailing bytes stay
 * unconsumed. The longest match is 16 bytes, and its last byte is indexed
 * only when the next byte is already visible, so callers pass 16 here and
 * wait until 17 bytes are in hand. That is the single-buffer rule. */
static unsigned long mv_run(struct mvcomp *c, unsigned char *dst, unsigned long dst_sz,
                            unsigned long reserve) {
  unsigned long len;
  unsigned long produced;
  const unsigned char *src;

  len = (unsigned long)c->fill - (unsigned long)c->hist;
  src = c->win + c->hist;
  produced = mv_emit(c, dst, dst_sz, src, &len, reserve);
  c->hist = (unsigned short)((unsigned long)c->fill - len);
  return(produced);
}


void mvcomp_reset(struct mvcomp *c) {
  if (c == NULL) return;
  mv_index_clear(c->head);
  c->pos = 0;
  c->hist = 0;
  c->fill = 0;
  c->litq_len = 0;
  c->done = 0;
}


struct mvcomp *mvcomp_open(void) {
  struct mvcomp *c;

  c = malloc(sizeof(*c));
  if (c == NULL) return(NULL);
  c->head = mv_index_alloc();
  if (c->head == NULL) {
    free(c);
    return(NULL);
  }
  mvcomp_reset(c);
  return(c);
}


void mvcomp_close(struct mvcomp *c) {
  if (c == NULL) return;
  if (c->head != NULL) mv_index_free(c->head);
  free(c);
}


unsigned long mvcomp_feed(struct mvcomp *c, void *dst, unsigned long dst_sz,
                          const unsigned char *src, unsigned long *src_len) {
  unsigned char *op = dst;
  unsigned long written = 0;
  unsigned long left;

  if ((c == NULL) || (src_len == NULL)) return(0);
  if (c->done) return(0);
  if ((*src_len != 0) && (src == NULL)) return(0);
  if (dst == NULL) return(0);
  left = *src_len;

  for (;;) {
    unsigned long pend;
    unsigned long pend_after;
    unsigned long left_before;
    unsigned long produced;
    unsigned long room;

    left_before = left;
    mv_take(c, &src, &left);
    pend = (unsigned long)c->fill - (unsigned long)c->hist;

    /* 16 bytes of match plus the following byte, or wait for finish */
    if (pend <= 16ul) {
      if ((left == 0) || (left == left_before)) break;
      continue;
    }

    produced = 0;
    room = dst_sz - written;
    if (room != 0) {
      produced = mv_run(c, op, room, 16ul);
      op += produced;
      written += produced;
    }
    pend_after = (unsigned long)c->fill - (unsigned long)c->hist;
    if ((produced == 0) && (left == left_before) && (pend_after == pend)) break;
  }

  *src_len = left;
  return(written);
}


int mvcomp_finish(struct mvcomp *c, void *dst, unsigned long dst_sz,
                  unsigned long *written) {
  unsigned long w;

  if (written == NULL) return(-1);
  *written = 0;
  if (c == NULL) return(-1);
  if (c->done) return(0);
  if ((dst == NULL) && ((c->fill != c->hist) || (c->litq_len != 0))) return(1);
  if (dst == NULL) {
    c->done = 1;
    return(0);
  }

  w = mv_run(c, (unsigned char *)dst, dst_sz, 0);
  *written = w;
  if (((unsigned long)c->fill == (unsigned long)c->hist) && (c->litq_len == 0)) {
    c->done = 1;
    return(0);
  }
  return(1);
}


unsigned long mvcomp_pack(void *dst, unsigned long dst_sz,
                          const unsigned char *src, unsigned long src_len) {
  struct mvcomp *c;
  unsigned char *op;
  unsigned long written;
  unsigned long w;
  int st;

  if (src_len == 0) return(0);
  if ((src == NULL) || (dst == NULL)) return(MVCOMP_ERR);

  c = mvcomp_open();
  if (c == NULL) return(MVCOMP_ERR);
  op = dst;
  written = 0;

  while (src_len > 0) {
    unsigned long left;
    left = src_len;
    w = mvcomp_feed(c, op, dst_sz - written, src, &left);
    if ((w == 0) && (left == src_len)) {
      mvcomp_close(c);
      return(MVCOMP_ERR);
    }
    src += src_len - left;
    op += w;
    written += w;
    src_len = left;
  }

  for (;;) {
    st = mvcomp_finish(c, op, dst_sz - written, &w);
    op += w;
    written += w;
    if (st == 0) {
      mvcomp_close(c);
      return(written);
    }
    if (w == 0) {
      mvcomp_close(c);
      return(MVCOMP_ERR);
    }
  }
}
