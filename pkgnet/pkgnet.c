/*
 * pkgnet - pulls SvarDOS packages from the project's online repository
 * Copyright (C) 2021 Mateusz Viste
 *
 * PUBLISHED UNDER THE TERMS OF THE MIT LICENSE
 *
 * COPYRIGHT (C) 2016-2021 MATEUSZ VISTE, ALL RIGHTS RESERVED.
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
 * http://svardos.osdn.io
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "net.h"

#define PVER "20210121"
#define PDATE "2021"

#define HOSTADDR "svardos.osdn.io"


/* strips http headers and returns new buff len */
static int detecthttpheadersend(unsigned char *buff, int len) {
  static char lastbyteislf = 0; /* static because I must potentially remember it for next packet/call */
  int i;
  for (i = 0; i < len; i++) {
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
    /* end of headers! rewind the buffer and return new len */
    len -= (i + 1);
    if (len > 0) memmove(buff, buff + i + 1, len);
    return(len);
  }
  return(0);
}


static void help(void) {
  puts("pkgnet ver " PVER " -- Copyright (C) " PDATE " Mateusz Viste");
  puts("");
  puts("pkgnet is the SvarDOS package downloader.");
  puts("");
  puts("usage:  pkgnet search <term>");
  puts("        pkgnet pull <package>");
  puts("        pkgnet checkup [<package>]");
  puts("");
  puts("actions:");
  puts(" search   - asks remote repository for the list of matching packages");
  puts(" pull     - downloads package into current directory");
  puts(" checkup  - lists available updates (for all packages if no argument)");
  puts("");
}


/* parses command line arguments and fills outfname and url accordingly
 * returns 0 on success, non-zero otherwise */
static int parseargv(int argc, char * const *argv, char **outfname, char *url) {
  *outfname = NULL;
  *url = 0;
  if ((argc == 3) && (strcasecmp(argv[1], "search") == 0)) {
    sprintf(url, "/pkg.php?a=search&p=%s", argv[2]);
  } else if ((argc == 3) && (strcasecmp(argv[1], "pull") == 0)) {
    sprintf(url, "/pkg.php?a=pull&p=%s", argv[2]);
    *outfname = argv[2];
  } else if ((argc == 2) && (strcasecmp(argv[1], "checkup") == 0)) {
    puts("NOT SUPPORTED YET");
    return(-1);
  } else if ((argc == 3) && (strcasecmp(argv[1], "checkup") == 0)) {
    puts("NOT SUPPORTED YET");
    return(-1);
  } else {
    help();
    return(-1);
  }
  return(0);
}


int main(int argc, char **argv) {
  unsigned char buffer[4096];
  char url[64];
  struct net_tcpsocket *sock;
  time_t lastactivity;
  int headersdone = 0;
  int httpcode = -1;
  long flen = 0;
  unsigned short bsum = 0;
  char *outfname = NULL;
  FILE *fd = NULL;

  /* parse command line arguments */
  if (parseargv(argc, argv, &outfname, url) != 0) return(1);

  /* init network stack */
  if (net_init() != 0) {
    puts("ERROR: Network subsystem initialization failed");
    return(1);
  }

  puts(""); /* required because watt-32 likes to print out garbage sometimes ("configuring through DHCP...") */

  if (net_dnsresolve((char *)buffer, HOSTADDR) != 0) {
    puts("ERROR: DNS resolution failed");
    return(1);
  }

  sock = net_connect((char *)buffer, 80);
  if (sock == NULL) {
    puts("ERROR: failed to connect to " HOSTADDR);
    goto SHITQUIT;
  }

  /* wait for net_connect() to actually connect */
  for (;;) {
    int connstate;
    connstate = net_isconnected(sock, 1);
    if (connstate > 0) break;
    if (connstate < 0) {
      puts("ERROR: connection error");
      goto SHITQUIT;
    }
  }

  /* socket is connected - send the http request */
  snprintf((char *)buffer, sizeof(buffer), "GET %s HTTP/1.1\r\nHOST: " HOSTADDR "\r\nUSER-AGENT: pkgnet\r\nConnection: close\r\n\r\n", url);

  if (net_send(sock, (char *)buffer, strlen((char *)buffer)) != (int)strlen((char *)buffer)) {
    puts("ERROR: failed to send HTTP query to remote server");
    goto SHITQUIT;
  }

  lastactivity = time(NULL);
  for (;;) {
    int byteread = net_recv(sock, (char *)buffer, sizeof(buffer) - 1); /* -1 because I will append a NULL terminator */

    if (byteread < 0) break; /* end of connection */

    /*  */
    if (byteread == 0) {
      if (time(NULL) - lastactivity > 20) { /* TIMEOUT! */
        puts("ERROR: Timeout while waiting for data");
        goto SHITQUIT;
      }
      /* waiting for packets - release a CPU cycle in the meantime */
      _asm int 28h;
      /* */
      continue;
    }

    if (byteread > 0) {
      buffer[byteread] = 0;
      lastactivity = time(NULL);
      /* do I know the http code yet? */
      if (httpcode < 0) {
        httpcode = atoi((char *)buffer);
        /* on error, the answer should be always printed on screen */
        if ((httpcode == 200) && (outfname != NULL)) {
          fd = fopen(outfname, "wb");
          if (fd == NULL) {
            printf("ERROR: failed to create file %s", outfname);
            puts("");
            break;
          }
        }
      }
      /* skip headers: look for \r\n\r\n or \n\n within the stream */
      if (headersdone == 0) {
        byteread = detecthttpheadersend(buffer, byteread);
        headersdone = 1;
        if (byteread == 0) continue;
      }
      /* if downloading to file, write stuff to disk */
      if (fd != NULL) {
        int i;
        if (fwrite(buffer, 1, byteread, fd) != byteread) {
          printf("ERROR: failed to write data to file %s after %ld bytes", outfname, flen);
          puts("");
          break;
        }
        flen += byteread;
        /* update the bsd sum */
        for (i = 0; i < byteread; i++) {
          /* rotr16 */
          unsigned short bsumlsb = bsum & 1;
          bsum >>= 1;
          bsum |= (bsumlsb << 15);
          /* bsum += ch */
          bsum += buffer[i];
        }
      } else { /* otherwise dump to screen */
        printf("%s", buffer);
      }
    }
  }

  if (fd != NULL) {
    /* print bsum, size, filename */
    printf("Downloaded %ld KiB into %s (BSUM: %04X)", flen >> 10, outfname, bsum);
    puts("");
    fclose(fd);
  }

  net_close(sock);
  return(0);

  SHITQUIT:
  if (sock != NULL) net_abort(sock);
  return(1);
}
