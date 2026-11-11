                         The MVCOMP library
                            Mateusz Viste

                          version 20261111


Code in this directory is what a program compiles in when it packs or
unpacks MVCOMP data itself, instead of running the mvcomp and mvucomp
tools. The format is described in ../mvcomp.txt. Include mvcomp.h. Link
mvpack.c to compress, mvdepack.c to decompress. The two are independent.


mvcomp.h

The declarations.

  unsigned long n;
  n = mvcomp_pack(dst, MVCOMP_BOUND(src_len), src, src_len);
  n = mvcomp_unpack(plain, plain_sz, packed, packed_len);

Each call returns a size, or MVCOMP_ERR. An empty input returns 0.
MVCOMP_BOUND(src_len) is large enough to hold the packed result.


mvpack.c

The compressor from the mvcomp tool. It keeps a 256 KiB match index. On
16-bit Watcom that block is a huge allocation (halloc), so compile this
file in the same memory model as the program. The small model is fine.
A buffer still has to sit in one segment, because a far pointer cannot
walk into the next one. Lengths are unsigned long: size_t is 16 bits in
that model.

mvcomp_pack compresses one complete buffer. A long input is fed in pieces:

  struct mvcomp *c;
  c = mvcomp_open();
  mvcomp_feed(c, dst, dst_sz, src, &src_len);
  mvcomp_finish(c, dst, dst_sz, &written);
  mvcomp_close(c);

mvcomp_feed holds up to 16 bytes back, so a match can see the byte after
its last copied byte. mvcomp_finish writes that tail. Feed may accept only
part of the input; call it again with what is left. If it returns 0 and
leaves the length unchanged, the destination was shorter than 34 bytes,
which is the largest token (a full literal queue followed by a match).
mvcomp_reset starts another stream on the same index.


mvdepack.c

The C decompressor used by mvucomp.

mvcomp_unpack expands one complete buffer. It returns MVCOMP_ERR when the
stream is truncated, a back reference starts before the output, or the
destination is too small.

A long stream is one little-endian word at a time, which is how mvucomp
walks a file:

  struct mvcomp_depack d;
  mvcomp_depack_init(&d);
  n = mvcomp_depack_word(&d, dst, word, win);

win is the oldest output byte still kept. dst is where this word is
written, at or after win, in the same buffer. The call returns how many
bytes it wrote, or 0 when a back reference would start before win. A
literal can ask for more raw words; mvcomp_depack_pending(&d) stays
nonzero until they have been consumed.


mvdepack.asm

The same decoder in 54 bytes of 8086 machine code, for a program that
does not want the C version.

  nasm -f bin mvdepack.asm -o mvdepack.bin

ES:SI is the compressed bytes, ES:DI is where they expand, and BX is the
compressed length in bytes. The two buffers may overlap; ../in-place.txt
says how far apart they have to be. On return DI is one past the last
uncompressed byte. The register rules are at the top of the file.
