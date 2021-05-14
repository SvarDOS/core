/*
 * This file is part of the pkgnet package - the SvarDOS package manager.
 * Copyright (C) Mateusz Viste 2013-2021
 *
 * Provides all network functions used by pkgnet, wrapped around the
 * Watt-32 TCP/IP stack.
 */

#include <stdlib.h>

/* Watt32 */
#include <tcp.h>

#include "net.h" /* include self for control */


struct net_tcpsocket {
  tcp_Socket *sock;
  tcp_Socket _sock; /* watt32 socket */
};


int net_dnsresolve(char *ip, const char *name) {
  unsigned long ipnum;
  ipnum = resolve(name); /* I could use WatTCP's lookup_host() here to do all
                            the job for me, unfortunately lookup_host() issues
                            wild outs() calls putting garbage on screen... */
  if (ipnum == 0) return(-1);
  _inet_ntoa(ip, ipnum); /* convert to string */
  return(0);
}


static int dummy_printf(const char * format, ...) {
  if (format == NULL) return(-1);
  return(0);
}


/* must be called before using libtcp. returns 0 on success, or non-zero if network subsystem is not available. */
int net_init(void) {
  tzset();
  _printf = dummy_printf;  /* this is to avoid watt32 printing its stuff to console */
  return(sock_init());
}


struct net_tcpsocket *net_connect(const char *ipstr, unsigned short port) {
  struct net_tcpsocket *resultsock;
  unsigned long ipaddr;

  /* convert ip to value */
  ipaddr = _inet_addr(ipstr);
  if (ipaddr == 0) return(NULL);

  resultsock = calloc(sizeof(struct net_tcpsocket), 1);
  if (resultsock == NULL) return(NULL);
  resultsock->sock = &(resultsock->_sock);

  /* explicitely set user-managed buffer to none (watt32 will use its own internal buffer) */
  sock_setbuf(resultsock->sock, NULL, 0);

  if (!tcp_open(resultsock->sock, 0, ipaddr, port, NULL)) {
    sock_abort(resultsock->sock);
    free(resultsock);
    return(NULL);
  }

  return(resultsock);
}


int net_isconnected(struct net_tcpsocket *s) {
  if (tcp_tick(s->sock) == 0) return(-1);
  if (sock_established(s->sock) == 0) return(0);
  return(1);
}


/* Sends data on socket 'socket'.
   Returns the number of bytes sent on success, and < 0 otherwise */
int net_send(struct net_tcpsocket *socket, const void *line, long len) {
  /* call this to let Watt-32 handle its internal stuff */
  if (tcp_tick(socket->sock) == 0) return(-1);
  /* send bytes */
  return(sock_write(socket->sock, line, len));
}


/* Reads data from socket 'sock' and write it into buffer 'buff', until end of connection. Will fall into error if the amount of data is bigger than 'maxlen' bytes.
Returns the amount of data read (in bytes) on success, or a negative value otherwise. The error code can be translated into a human error message via libtcp_strerr(). */
int net_recv(struct net_tcpsocket *socket, void *buff, long maxlen) {
  /* call this to let WatTCP handle its internal stuff */
  if (tcp_tick(socket->sock) == 0) return(-1);
  return(sock_fastread(socket->sock, buff, maxlen));
}


/* Close the 'sock' socket. */
void net_close(struct net_tcpsocket *socket) {
  /* I could use sock_close() and sock_wait_closed() if I'd want to be
   * friendly, but it's much easier on the tcp stack to send a single RST and
   * forget about the connection (esp. if the peer is misbehaving) */
  sock_abort(socket->sock);
  free(socket);
}


const char *net_engine(void) {
  return(wattcpVersion());
}
