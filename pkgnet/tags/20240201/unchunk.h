/*
 * unpacks a http "chunked" transfer into a raw data stream.
 * this file is part of the pkgnet tool from the SvarDOS project.
 *
 * Copyright (C) 2021 Mateusz Viste
 */

#ifndef UNCHUNK_H
#define UNCHUNK_H

struct unchunk_state {
  char partial_hdr[16];  /* a small buffer for storing partial chunk headers, if these are transmitted in separate parts */
  long bytesleft;        /* how many bytes are expected yet in the ongoing chunk */
};

/* transforms a http CHUNKED stream into actual data, returns the amount of
 * raw data to read or -1 on error. st MUST be zeroed before first call. */
int unchunk(unsigned char *buff, int bufflen, struct unchunk_state *st);

#endif
