#ifndef WSPROXY_H
#define WSPROXY_H
#include "util.h"
#include "sha1.h"
#include "base64.h"

#include <arpa/inet.h>
#include <netinet/in.h> 
#define WSPROXY_VERSION 1.0.0

extern int fd;


// handshake.c
extern char *h_hoststr;
extern int h_port;
extern char *start_handshake();
extern char *finish_handshake();

// net.c
extern void *get_in_addr(struct sockaddr *sa);
extern char *n_connect();
extern int tcpfd;

// ws.c
extern char *ws_send(char *payload, uint16_t pl_len);
extern int ws_recv();
#endif
