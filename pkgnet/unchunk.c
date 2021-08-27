/*
 * unpacks a http "chunked" transfer into a raw data stream.
 * this file is part of the pkgnet tool from the SvarDOS project.
 *
 * Copyright (C) 2021 Mateusz Viste
 */

#include <stdlib.h>    /* atol() */
#include <string.h>

#include "unchunk.h"

/* transforms a http CHUNKED stream into actual data, returns the amount of raw data to read */
int unchunk(unsigned char *buff, int bufflen) {
  static long bytesleft; /* how many bytes are expected yet in the ongoing chunk */
  static char partial_hdr[16]; /* a small buffer for storing partial chunk headers, if these are transmitted in separate parts */
  int hdrstart, hdrend;

  /* if chunk header was being in progress, try to decode it now */
  if (bytesleft == -1) {
    int partial_hdr_len = strlen(partial_hdr);
    /* locate header terminator, if available */
    for (hdrend = 0; (hdrend < bufflen) && (buff[hdrend] != '\n'); hdrend++);
    if (partial_hdr_len + hdrend > 15) return(-1); /* error: chunk header too long */

    /* copy header to buffer */
    memcpy(partial_hdr + partial_hdr_len, buff, hdrend);
    partial_hdr[partial_hdr_len + hdrend] = 0;

    /* quit if header still no complete */
    if (hdrend >= bufflen) return(0);

    /* good, got whole header */
    bufflen -= hdrend + 1;
    memmove(buff, buff + hdrend + 1, bufflen);
    bytesleft = strtol(partial_hdr, NULL, 16);
  }

  AGAIN:

  if (bufflen <= bytesleft) { /* bufflen <= bytesleft */
    bytesleft -= bufflen;
    return(bufflen);
  }

  /* else bufflen > bytesleft */

  /* skip trailing \r\n after chunk */
  for (hdrstart = bytesleft; hdrstart < bufflen; hdrstart++) if ((buff[hdrstart] != '\r') && (buff[hdrstart] != '\n')) break;
  /* skip chunk size (look for its \n terminator) */
  for (hdrend = hdrstart; (hdrend < bufflen) && (buff[hdrend] != '\n'); hdrend++);

  if (hdrend < bufflen) { /* full header found */
    long newchunklen;
    /* read the header length */
    newchunklen = strtol((char *)buff + hdrstart, NULL, 16);
    /* move data over header to get a contiguous block of data */
    memmove(buff + bytesleft, buff + hdrend + 1, bufflen - (hdrend + 1));
    bufflen -= (hdrend + 1 - bytesleft);
    /* update bytesleft */
    bytesleft += newchunklen;
    /* loop again */
    goto AGAIN;
  } else { /* partial header */
    if ((bufflen - bytesleft) > 15) return(-1); /* error: chunk header appears to be longer than 15 characters */
    memset(partial_hdr, 0, sizeof(partial_hdr));
    memcpy(partial_hdr, buff + bytesleft, bufflen - bytesleft);
    bufflen -= (bufflen - bytesleft);
    bytesleft = -1; /* "header in progress" */
    return(bufflen);
  }
}
