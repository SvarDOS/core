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
#include <string.h>
#include <time.h>

#include "net.h"

#define HOSTADDR "svardos.osdn.io"

/* strips http headers and returns new buff len */
static int detecthttpheadersend(char *buff, int len) {
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
    len -= i;
    if (len > 0) memmove(buff, buff + i, len);
    return(len);
  }
  return(0);
}


int main(int argc, char **argv) {
  char buffer[4096];
  char url[64];
  struct net_tcpsocket *sock;
  time_t lastactivity;
  int headersdone;

  /* prepare the query */
  snprintf(url, sizeof(url), "/pkgnet.php?action=xxx");

  /* init network stack */
  if (net_init() != 0) {
    puts("ERROR: Network subsystem initialization failed");
    return(1);
  }

  puts(""); /* required because watt-32 likes to print out garbage sometimes ("configuring through DHCP...") */

  if (net_dnsresolve(buffer, HOSTADDR) != 0) {
    puts("ERROR: DNS resolution failed");
    return(1);
  }

  sock = net_connect(buffer, 80);
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
  snprintf(buffer, sizeof(buffer), "GET %s HTTP/1.1\r\nHOST: " HOSTADDR "\r\nUSER-AGENT: pkgnet\r\nConnection: close\r\n\r\n", url);

  if (net_send(sock, buffer, strlen(buffer)) != (int)strlen(buffer)) {
    puts("ERROR: failed to send HTTP query to remote server");
    goto SHITQUIT;
  }

  lastactivity = time(NULL);
  headersdone = 0;
  for (;;) {
    int byteread = net_recv(sock, buffer, sizeof(buffer) - 1);

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
      /* skip headers: look for \r\n\r\n or \n\n within the stream */
      if (headersdone == 0) {
        byteread = detecthttpheadersend(buffer, byteread);
        headersdone = 1;
        if (byteread == 0) continue;
      }
      /* if downloading to file, write stuff to disk */
      printf("%s", buffer);
    }
  }

  net_close(sock);
  return(0);

  SHITQUIT:
  if (sock != NULL) net_abort(sock);
  return(1);
}
