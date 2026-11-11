/*
 * MVCOMP library
 * Copyright (C) 2024-2026 Mateusz Viste
 *
 * MIT License.
 *
 * http://mateusz.fr/mvcomp/
 *
 * Packing and unpacking for the MVCOMP format (see mvcomp.txt). A stream
 * is raw words, little-endian, with no header and no stored length. The
 * bytes are the ones the mvcomp and mvucomp tools would write.
 *
 *   unsigned long n;
 *   n = mvcomp_pack(dst, dst_sz, src, src_len);
 *   n = mvcomp_unpack(dst, dst_sz, src, src_len);
 *
 * Both return a size, or MVCOMP_ERR. An empty input comes back as 0.
 * MVCOMP_BOUND(src_len) bytes is always enough for a packed result.
 * Link mvpack.c for the compressor and mvdepack.c for the decompressor.
 * mvdepack.asm is the same decoder in 8086 code, for a program that does
 * not want the C version. lib/readme.txt says which file to compile.
 *
 * A long input, or a 16-bit program whose buffer crosses a segment, is fed
 * in pieces. mvcomp_open allocates the compressor, including its 256KB match
 * index (halloc on 16-bit Watcom, malloc elsewhere). Compile mvpack.c in
 * the same memory model as the caller; the index is a huge block, so the
 * small model can use it. mvcomp_feed takes the next input bytes. It holds
 * up to 16 bytes back: a match looks at 16 bytes, and the last of those is
 * indexed only once the byte after it is known. mvcomp_finish writes that
 * tail and closes the literal run. mvcomp_reset starts another stream on
 * the same index.
 *
 * mvcomp_feed may accept only part of the input. Call it again with what
 * remains. It can buffer bytes before it writes them. A destination of
 * less than 34 bytes is too small for the largest token (a literal queue
 * followed by a match): if feed returns 0 and leaves *src_len unchanged,
 * pass a buffer of at least 34 bytes. On a 16-bit compiler a pointer must
 * cover its whole buffer with ordinary pointer arithmetic, so each piece
 * has to sit in one segment. Lengths are unsigned long because size_t is
 * 16 bits in that model.
 *
 * Unpacking one word at a time is what mvucomp does. The state is a byte
 * and can sit on the stack. There is no match index.
 */


#ifndef MVCOMP_H
#define MVCOMP_H

#ifdef __cplusplus
extern "C" {
#endif

/* Upper bound on the compressed size of n input bytes.
 * One extra byte per 31, plus two for a short tail. n is evaluated twice. */
#define MVCOMP_BOUND(n) ((unsigned long)(n) + (unsigned long)(n) / 31ul + 2ul)

#define MVCOMP_ERR (~0ul)

struct mvcomp;

/* NULL if the 256KB match index cannot be allocated. */
struct mvcomp *mvcomp_open(void);

void mvcomp_close(struct mvcomp *c);

/* Drop the bytes fed so far. The index stays allocated. */
void mvcomp_reset(struct mvcomp *c);

/* Compress the next bytes. *src_len is how many bytes src points at;
 * on return it is how many of those were not accepted.
 * Returns how many compressed bytes were written to dst.
 * Returns 0 and accepts nothing when c is NULL, dst is NULL, src is NULL
 * while *src_len is not 0, or the stream is already finished. */
unsigned long mvcomp_feed(struct mvcomp *c, void *dst, unsigned long dst_sz,
                          const unsigned char *src, unsigned long *src_len);

/* Finish the stream. *written is how many bytes this call stored in dst.
 * Returns 0 when the stream is finished.
 * Returns 1 when dst filled before the tail was written; call again.
 * Returns -1 when c or written is NULL. */
int mvcomp_finish(struct mvcomp *c, void *dst, unsigned long dst_sz,
                  unsigned long *written);

/* Compress one complete buffer. See the comment above. */
unsigned long mvcomp_pack(void *dst, unsigned long dst_sz,
                          const unsigned char *src, unsigned long src_len);


/* One decompressor. mvcomp_depack_init clears it. The raw-word count is
 * updated by mvcomp_depack_word; leave it alone otherwise. */
struct mvcomp_depack {
  unsigned char rawwords;
};

void mvcomp_depack_init(struct mvcomp_depack *d);

/* Nonzero when a literal is still waiting for raw words. */
int mvcomp_depack_pending(const struct mvcomp_depack *d);

/* Decode one little-endian word into dst. win is the first output byte
 * still available, and dst is at or after win, in the same buffer.
 * Returns how many bytes were written, or 0 if a back reference would
 * start before win. A literal run may continue in the next words. */
unsigned short mvcomp_depack_word(struct mvcomp_depack *d, unsigned char *dst,
                                  unsigned short word, const unsigned char *win);

/* Unpack one complete buffer. Returns the uncompressed size, or
 * MVCOMP_ERR if the stream is truncated, a back reference falls outside
 * the output, or dst_sz is too small. An empty input returns 0.
 * The same 16-bit pointer limit as mvcomp_pack applies. */
unsigned long mvcomp_unpack(void *dst, unsigned long dst_sz,
                            const void *src, unsigned long src_len);

#ifdef __cplusplus
}
#endif

#endif
