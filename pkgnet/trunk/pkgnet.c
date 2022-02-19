/*
 * pkgnet - pulls SvarDOS packages from the project's online repository
 *
 * PUBLISHED UNDER THE TERMS OF THE MIT LICENSE
 *
 * COPYRIGHT (C) 2016-2022 MATEUSZ VISTE, ALL RIGHTS RESERVED.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
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
 * http://svardos.org
 */

#include <direct.h> /* opendir() and friends */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "net.h"
#include "unchunk.h"

#include "svarlang.lib\svarlang.h"

#include "../../pkg/trunk/lsm.h"


#define PVER "20220219"
#define PDATE "2021-2022"

#define HOSTADDR "svardos.org"


/* convenience define that outputs nls strings to screen (followed by CR/LF) */
#define putsnls(x,y) puts(svarlang_strid((x << 8) | y))


/* returns length of all http headers, or 0 if uncomplete yet */
static unsigned short detecthttpheadersend(const unsigned char *buff) {
  char lastbyteislf = 0;
  unsigned short i;
  for (i = 0; buff[i] != 0; i++) {
    if (buff[i] == '\r') continue; /* ignore CR characters */
    if (buff[i] != '\n') {
      lastbyteislf = 0;
      continue;
    }
    /* cur byte is LF -> if last one was also LF then this is an empty line, meaning headers are over */
    if (lastbyteislf == 0) {
      lastbyteislf = 1;
      continue;
    }
    /* end of headers! return length of headers */
    return(i + 1); /* add 1 to skip the current \n character */
  }
  return(0);
}


static void help(void) {
  puts("pkgnet ver " PVER " -- Copyright (C) " PDATE " Mateusz Viste");
  puts("");
  putsnls(1, 0);  /* "pkgnet is the SvarDOS package downloader" */
  puts("");
  putsnls(1, 1);  /* "usage:  pkgnet search <term>" */
  putsnls(1, 2);  /* "        pkgnet pull <package>" */
  putsnls(1, 3);  /* "        pkgnet pull <package>-<version>" */
  putsnls(1, 4);  /* "        pkgnet pullsrc <package>" */
  putsnls(1, 5);  /* "        pkgnet pullsrc <package>-<version>" */
  putsnls(1, 6);  /* "        pkgnet checkup" */
  puts("");
  putsnls(1, 7);  /* "actions:" */
  puts("");
  putsnls(1, 8);  /* "search   - asks remote repository for the list of matching packages" */
  putsnls(1, 9);  /* "pull     - downloads package into current directory" */
  putsnls(1, 10); /* "checkup  - lists updates available for your system" */
  puts("");
  printf("Watt32 kernel: %s", net_engine());
  puts("");
}


/* parses command line arguments and fills outfname and url accordingly
 * returns 0 on success, non-zero otherwise */
static int parseargv(int argc, char * const *argv, char *outfname, char *url, int *ispost) {
  const char *lang = getenv("LANG");
  if (lang == NULL) lang = "";
  *outfname = 0;
  *url = 0;
  *ispost = 0;
  if ((argc == 3) && (strcasecmp(argv[1], "search") == 0)) {
    sprintf(url, "/repo/?a=search&p=%s&lang=%s", argv[2], lang);
  } else if ((argc == 3) && ((strcasecmp(argv[1], "pull") == 0) || (strcasecmp(argv[1], "pullsrc") == 0))) {
    unsigned short i;
    /* copy argv[2] into outfname, but stop at first '-' or null terminator
     * this trims any '-version' part in filename to respect 8+3 */
    for (i = 0; (argv[2][i] != 0) && (argv[2][i] != '-') && (i < 8); i++) {
      outfname[i] = argv[2][i];
    }
    /* add the extension (svp or zip) to filename and compute url */
    if (strcasecmp(argv[1], "pull") == 0) {
      sprintf(url, "/repo/?a=pull&p=%s&lang=%s", argv[2], lang);
      strcpy(outfname + i, ".svp");
    } else {
      sprintf(url, "/repo/?a=pullsrc&p=%s&lang=%s", argv[2], lang);
      strcpy(outfname + i, ".zip");
    }
  } else if ((argc == 2) && (strcasecmp(argv[1], "checkup") == 0)) {
    sprintf(url, "/repo/?a=checkup&lang=%s", lang);
    *ispost = 1;
  } else {
    help();
    return(-1);
  }
  return(0);
}


static int htget_headers(unsigned char *buffer, size_t buffersz, struct net_tcpsocket *sock, int *httpcode, int *ischunked)  {
  unsigned char *buffptr = buffer;
  unsigned short bufflen = 0;
  int byteread;
  time_t starttime = time(NULL);
  for (;;) {
    byteread = net_recv(sock, buffptr, buffersz - (bufflen + 1)); /* -1 because I will append a NULL terminator */

    if (byteread > 0) { /* got data */
      int hdlen;
      bufflen += byteread;
      buffptr += byteread;
      buffer[bufflen] = 0;
      hdlen = detecthttpheadersend(buffer);
      if (hdlen > 0) { /* full headers - parse http code and continue processing */
        int i;
        buffer[hdlen - 1] = 0;
        /* find the first space (HTTP/1.1 200 OK) */
        for (i = 0; i < 16; i++) {
          if ((buffer[i] == ' ') || (buffer[i] == 0)) break;
        }
        if (buffer[i] != ' ') return(-1);
        *httpcode = atoi((char *)(buffer + i + 1));
        /* switch all headers to low-case so it is easier to parse them */
        for (i = 0; i < hdlen; i++) if ((buffer[i] >= 'A') && (buffer[i] <= 'Z')) buffer[i] += ('a' - 'A');
        /* look out for chunked transfer encoding */
        {
        char *lineptr = strstr((char *)buffer, "\ntransfer-encoding:");
        if (lineptr != NULL) {
          lineptr += 19;
          /* replace nearest \r, \n or 0 by 0 */
          for (i = 0; ; i++) {
            if ((lineptr[i] == '\r') || (lineptr[i] == '\n') || (lineptr[i] == 0)) {
              lineptr[i] = 0;
              break;
            }
          }
          /* do I see the 'chunked' word? */
          if (strstr((char *)lineptr, "chunked") != NULL) *ischunked = 1;
        }
        }
        /* rewind the buffer */
        bufflen -= hdlen;
        memmove(buffer, buffer + hdlen, bufflen);
        return(bufflen); /* move to body processing now */
      }

    } else if (byteread < 0) { /* abort on error */
      return(-2); /* unexpected end of connection (while waiting for all http headers) */

    } else { /* else no data received - look for timeout and release a cpu cycle */
      if (time(NULL) - starttime > 20) return(-3); /* TIMEOUT! */
      _asm int 28h; /* release a CPU cycle */
    }
  }
}


/* provides body data of the POST query for checkup actions
   fills buff with data and returns data length.
   must be called repeateadly until zero-lengh is returned */
static unsigned short checkupdata(char *buff) {
  static char *dosdir = NULL;
  static DIR *dp;
  static struct dirent *ep;

  /* make sure I know %DOSDIR% */
  if (dosdir == NULL) {
    dosdir = getenv("DOSDIR");
    if ((dosdir == NULL) || (dosdir[0] == 0)) {
      putsnls(9, 0); /* "ERROR: %DOSDIR% not set" */
      return(0);
    }
  }

  /* if first call, open the package directory */
  if (dp == NULL) {
    sprintf(buff, "%s\\packages", dosdir);
    dp = opendir(buff);
    if (dp == NULL) {
      putsnls(9, 1); /* "ERROR: Could not access %DOSDIR%\\packages directory" */
      return(0);
    }
  }

  for (;;) {
    int tlen;
    char ver[20];
    ep = readdir(dp);   /* readdir() result must never be freed (statically allocated) */
    if (ep == NULL) {   /* no more entries */
      closedir(dp);
      return(0);
    }

    tlen = strlen(ep->d_name);
    if (tlen < 4) continue; /* files must be at least 5 bytes long ("x.lst") */
    if (strcasecmp(ep->d_name + tlen - 4, ".lst") != 0) continue;  /* if not an .lst file, skip it silently */
    ep->d_name[tlen - 4] = 0; /* trim out the ".lst" suffix */

    /* load the metadata from %DOSDIR\APPINFO\*.lsm */
    sprintf(buff, "%s\\appinfo\\%s.lsm", dosdir, ep->d_name);
    readlsm(buff, ver, sizeof(ver));

    return(sprintf(buff, "%s\t%s\n", ep->d_name, ver));
  }
}


/* fetch http data from ipaddr using url
 * write result to file outfname if not null, or print to stdout otherwise
 * fills bsum with the BSD sum of the data
 * is ispost is non-zero, then the request is a POST and its body data is
 * obtained through repeated calls to checkupdata()
 * returns the length of data obtained, or neg value on error */
static long htget(const char *ipaddr, const char *url, const char *outfname, unsigned short *bsum, int ispost, unsigned char *buffer, size_t buffersz) {
  struct net_tcpsocket *sock;
  time_t lastactivity, lastprogressoutput = 0;
  int httpcode = -1, ischunked = 0;
  int byteread;
  long flen = 0, lastflen = 0;
  FILE *fd = NULL;

  /* unchunk state variable is using a little part of the supplied buffer */
  struct unchunk_state *unchstate = (void *)buffer;
  buffer += sizeof(*unchstate);
  buffersz -= sizeof(*unchstate);
  memset(unchstate, 0, sizeof(*unchstate));

  sock = net_connect(ipaddr, 80);
  if (sock == NULL) {
    printf(svarlang_strid(0x0902), HOSTADDR); /* "ERROR: failed to connect to " HOSTADDR */
    puts("");
    goto SHITQUIT;
  }

  /* wait for net_connect() to actually connect */
  for (;;) {
    int connstate = net_isconnected(sock);
    if (connstate > 0) break;
    if (connstate < 0) {
      printf(svarlang_strid(0x0902), HOSTADDR); /* "ERROR: failed to connect to " HOSTADDR */
      puts("");
      goto SHITQUIT;
    }
    _asm int 28h;  /* DOS idle */
  }

  /* socket is connected - send the http request (MUST be HTTP/1.0 because I do not support chunked transfers!) */
  if (ispost) {
    snprintf((char *)buffer, buffersz, "POST %s HTTP/1.1\r\nHOST: " HOSTADDR "\r\nUSER-AGENT: pkgnet/" PVER "\r\nTransfer-Encoding: chunked\r\nConnection: close\r\n\r\n", url);
  } else {
    snprintf((char *)buffer, buffersz, "GET %s HTTP/1.1\r\nHOST: " HOSTADDR "\r\nUSER-AGENT: pkgnet/" PVER "\r\nConnection: close\r\n\r\n", url);
  }

  if (net_send(sock, buffer, strlen((char *)buffer)) != (int)strlen((char *)buffer)) {
    putsnls(9, 3); /* "ERROR: failed to send a HTTP query to remote server" */
    goto SHITQUIT;
  }

  /* send chunked data for POST queries */
  if (ispost) {
    unsigned short blen;
    int hlen;
    char *hbuf = (char *)buffer + buffersz - 16;
    do {
      blen = checkupdata((char *)buffer);
      if (blen == 0) { /* last item contains the message trailer */
        hlen = sprintf(hbuf, "0\r\n");
      } else {
        hlen = sprintf(hbuf, "%X\r\n", blen);
      }
      if (net_send(sock, hbuf, hlen) != hlen) {
        putsnls(9, 4); /* "ERROR: failed to send POST data to remote server" */
        goto SHITQUIT;
      }
      /* add trailing CR/LF to buffer as required by chunked mode */
      buffer[blen++] = '\r';
      buffer[blen++] = '\n';
      if (net_send(sock, buffer, blen) != blen) {
        putsnls(9, 4); /* "ERROR: failed to send POST data to remote server" */
        goto SHITQUIT;
      }
    } while (blen != 2);
  }

  /* receive and process HTTP headers */
  byteread = htget_headers(buffer, buffersz, sock, &httpcode, &ischunked);

  /* transmission error? */
  if (byteread < 0) {
    printf(svarlang_strid(0x0905), byteread); /* "ERROR: TCP communication error #%d" */
    puts("");
    goto SHITQUIT;
  }

  /* open destination file if required and if no server-side error occured */
  if ((httpcode >= 200) && (httpcode <= 299) && (*outfname != 0)) {
    fd = fopen(outfname, "wb");
    if (fd == NULL) {
      printf(svarlang_strid(0x0906), outfname); /* "ERROR: failed to create file %s" */
      puts("");
      goto SHITQUIT;
    }
  }

  /* read body of the answer (and chunk-decode it if needed) */
  lastactivity = time(NULL);
  for (;; byteread = net_recv(sock, buffer, buffersz - 1)) { /* read 1 byte less because I need to append a NULL terminator when printf'ing the content */

    if (byteread > 0) { /* got data */
      lastactivity = time(NULL);

      /* unpack data if server transmits in chunked mode */
      if (ischunked) byteread = unchunk(buffer, byteread, unchstate);

      /* if downloading to file, write stuff to disk */
      if (fd != NULL) {
        int i;
        if (fwrite(buffer, 1, byteread, fd) != byteread) {
          printf(svarlang_strid(0x0907), outfname, flen); /* "ERROR: failed to write data to file %s after %ld bytes" */
          puts("");
          break;
        }
        flen += byteread;
        /* update progress once a sec */
        if (lastprogressoutput != lastactivity) {
          lastprogressoutput = lastactivity;
          printf("%ld KiB (%ld KiB/s)     \r", flen >> 10, (flen >> 10) - (lastflen >> 10)); /* trailing spaces are meant to avoid leaving garbage on screen if speed goes from, say, 1000 KiB/s to 9 KiB/s */
          lastflen = flen;
          fflush(stdout); /* avoid console buffering */
        }
        /* update the bsd sum */
        for (i = 0; i < byteread; i++) {
          /* rotr16 */
          unsigned short bsumlsb = *bsum & 1;
          *bsum >>= 1;
          *bsum |= (bsumlsb << 15);
          *bsum += buffer[i];
        }
      } else { /* otherwise dump to screen */
        buffer[byteread] = 0;
        printf("%s", buffer);
      }

    } else if (byteread < 0) { /* end of connection */
      break;

    } else { /* check for timeout (byteread == 0) */
      if (time(NULL) - lastactivity > 20) { /* TIMEOUT! */
        putsnls(9, 8); /* "ERROR: Timeout while waiting for data" */
        goto SHITQUIT;
      }
      /* waiting for packets - release a CPU cycle in the meantime */
      _asm int 28h;
    }
  }

  goto ALLGOOD;

  SHITQUIT:
  flen = -1;

  ALLGOOD:
  if (fd != NULL) fclose(fd);
  net_close(sock);
  return(flen);
}


/* checks if file exists, returns 0 if not, non-zero otherwise */
static int fexists(const char *fname) {
  FILE *fd = fopen(fname, "rb");
  if (fd == NULL) return(0);
  fclose(fd);
  return(1);
}


int main(int argc, char **argv) {
  unsigned short bsum = 0;
  long flen;
  int ispost; /* is the request a POST? */

  struct {
    unsigned char buffer[5000];
    char ipaddr[64];
    char url[64];
    char outfname[16];
  } *mem;

  svarlang_autoload("PKGNET");

  /* allocate memory */
  mem = malloc(sizeof(*mem));
  if (mem == NULL) {
    putsnls(9, 9); /* "ERROR: out of memory" */
    return(1);
  }

  /* parse command line arguments */
  if (parseargv(argc, argv, mem->outfname, mem->url, &ispost) != 0) return(1);

  /* if outfname requested, make sure that file does not exist yet */
  if ((mem->outfname[0] != 0) && (fexists(mem->outfname))) {
    printf(svarlang_strid(0x090A), mem->outfname); /* "ERROR: file %s already exists" */
    puts("");
    return(1);
  }

  /* init network stack */
  if (net_init() != 0) {
    putsnls(9, 11); /* "ERROR: Network subsystem initialization failed" */
    return(1);
  }

  puts(""); /* required because watt-32 likes to print out garbage sometimes ("configuring through DHCP...") */

  if (net_dnsresolve(mem->ipaddr, HOSTADDR, 2) != 0) {
    putsnls(9, 12); /* "ERROR: DNS resolution failed" */
    return(1);
  }

  flen = htget(mem->ipaddr, mem->url, mem->outfname, &bsum, ispost, mem->buffer, sizeof(mem->buffer));
  if (flen < 1) return(1);

  if (mem->outfname[0] != 0) {
    /* print bsum, size, filename */
    printf(svarlang_strid(0x0200), flen >> 10, mem->outfname, bsum); /* Downloaded %ld KiB into %s (BSUM: %04X) */
    puts("");
  }

  return(0);
}
