/*
 * unpacks a http "chunked" transfer into a raw data stream.
 * this file is part of the pkgnet tool from the SvarDOS project.
 *
 * Copyright (C) 2021 Mateusz Viste
 */

#include <stdlib.h>    /* atol() */
#include <string.h>

#include "unchunk.h"


/* transforms a http CHUNKED stream into actual data, returns the amount of
 * raw data to read or -1 on error. st MUST be zeroed before first call. */
int unchunk(unsigned char *buff, int bufflen, struct unchunk_state *st) {
  int hdrstart, hdrend;

  /* if chunk header was being in progress, try to decode it now */
  if (st->bytesleft == -1) {
    int partial_hdr_len = strlen(st->partial_hdr);
    /* locate header terminator, if available */
    for (hdrend = 0; (hdrend < bufflen) && (buff[hdrend] != '\n'); hdrend++);
    if (partial_hdr_len + hdrend > 15) return(-1); /* error: chunk header too long */

    /* copy header to buffer */
    memcpy(st->partial_hdr + partial_hdr_len, buff, hdrend);
    st->partial_hdr[partial_hdr_len + hdrend] = 0;

    /* quit if header still no complete */
    if (hdrend >= bufflen) return(0);

    /* good, got whole header */
    bufflen -= hdrend + 1;
    memmove(buff, buff + hdrend + 1, bufflen);
    st->bytesleft = strtol(st->partial_hdr, NULL, 16);
  }

  AGAIN:

  if (bufflen <= st->bytesleft) {
    st->bytesleft -= bufflen;
    return(bufflen);
  }

  /* else bufflen > bytesleft */

  /* skip trailing \r\n after chunk */
  for (hdrstart = st->bytesleft; hdrstart < bufflen; hdrstart++) if ((buff[hdrstart] != '\r') && (buff[hdrstart] != '\n')) break;
  /* skip chunk size (look for its \n terminator) */
  for (hdrend = hdrstart; (hdrend < bufflen) && (buff[hdrend] != '\n'); hdrend++);

  if (hdrend < bufflen) { /* full header found */
    long newchunklen;
    /* read the header length */
    newchunklen = strtol((char *)buff + hdrstart, NULL, 16);
    /* move data over header to get a contiguous block of data */
    memmove(buff + st->bytesleft, buff + hdrend + 1, bufflen - (hdrend + 1));
    bufflen -= (hdrend + 1 - st->bytesleft);
    /* update bytesleft */
    st->bytesleft += newchunklen;
    /* loop again */
    goto AGAIN;
  } else { /* partial header */
    if ((bufflen - st->bytesleft) > 15) return(-1); /* error: chunk header appears to be longer than 15 characters */
    memset(st->partial_hdr, 0, sizeof(st->partial_hdr));
    memcpy(st->partial_hdr, buff + st->bytesleft, bufflen - st->bytesleft);
    bufflen -= (bufflen - st->bytesleft);
    st->bytesleft = -1; /* "header in progress" */
    return(bufflen);
  }
}
