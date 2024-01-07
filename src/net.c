#include "wsproxy.h"

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

int tcpfd;

void *get_in_addr(struct sockaddr *sa) {
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in *)sa)->sin_addr);
  }

  return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}
char *n_connect() {
  struct addrinfo hints, *result;
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;

  char *err;

  if (getaddrinfo(h_hoststr, NULL, &hints, &result) != 0) {
    asprintf(&err, "cannot resolve %s", h_hoststr);
    return err;
  }

  for (struct addrinfo *p = result; p != NULL; p = p->ai_next) {
    ((struct sockaddr_in *)result->ai_addr)->sin_port = htons(h_port);
    tcpfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
    if (tcpfd == -1) {
      err = aserror("create socket failed");
      continue;
    }

    if (connect(tcpfd, p->ai_addr, p->ai_addrlen) == -1) {
      err = aserror("connection failed");
      close(tcpfd);
      tcpfd = -1;
      continue;
    }
    break;
  }
  freeaddrinfo(result);
  if (!tcpfd) return err;
  return NULL;
}
