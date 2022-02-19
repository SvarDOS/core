/*
 * This file is part of the pkgnet package - the SvarDOS package manager.
 * Copyright (C) Mateusz Viste 2013-2021
 */


#ifndef libtcp_hdr
#define libtcp_hdr

struct net_tcpsocket; /* opaque struct, exact implementation in net.c */

/* resolves name and fills resovled addr into ip. on failure it retries r times
 * (r=0 means "try only once"). returns 0 on success. */
int net_dnsresolve(char *ip, const char *name, int r);

/* must be called before using libtcp. returns 0 on success, or non-zero if network subsystem is not available. */
int net_init(void);

/* initiates a connection to an IP host and returns a socket pointer (or NULL
 * on error) - note that connection is NOT established at this point!
 * use net_isconnected() to know when the connection is connected. */
struct net_tcpsocket *net_connect(const char *ip, unsigned short port);

/* checks whether or not a socket is connected. returns:
 *  0 = not connected,
 *  1 = connected
 * -1 = error */
int net_isconnected(struct net_tcpsocket *s);

/* Sends data on socket 'socket'.
Returns the number of bytes sent on success, and <0 otherwise. The error code can be translated into a human error message via libtcp_strerr(). */
int net_send(struct net_tcpsocket *socket, const void *line, long len);

/* Reads data from socket 'sock' and write it into buffer 'buff', until end of connection. Will fall into error if the amount of data is bigger than 'maxlen' bytes.
Returns the amount of data read (in bytes) on success, or a negative value otherwise. The error code can be translated into a human error message via libtcp_strerr(). */
int net_recv(struct net_tcpsocket *socket, void *buff, long maxlen);

/* Close the 'sock' socket. */
void net_close(struct net_tcpsocket *socket);

/* Returns an info string about the networking engine being used */
const char *net_engine(void);

#endif
