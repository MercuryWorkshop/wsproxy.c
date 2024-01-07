#include "wsproxy.h"

#include <arpa/inet.h>
#include <asm-generic/socket.h>
#include <endian.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define die(msg)                                                               \
  do {                                                                         \
    perror(msg);                                                               \
    exit(EXIT_FAILURE);                                                        \
  } while (0)

int listener;
int fd;

int s_listen() {
  listener = socket(AF_INET, SOCK_STREAM, 0);
  int true = 1;
  setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &true, sizeof(int));
  if (listener < 0)
    die("socket");

  struct sockaddr_in addr;
  bzero(&addr, sizeof(addr));

  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(8081);

  if ((bind(listener, (struct sockaddr *)&addr, sizeof(addr))) != 0)
    die("bind");

  if (listen(listener, 5) != 0)
    die("listen");

  return 0;
}

int main() {
  s_listen();

  struct sockaddr_in incoming;
  int len = sizeof(incoming);

  while (1) {
    fd = accept(listener, (struct sockaddr *)&incoming, &len);
    if (fd < 0) {
      perror("server: accept failed: ");
      goto free;
    }

    char s[INET_ADDRSTRLEN];

    inet_ntop(incoming.sin_family, get_in_addr((struct sockaddr *)&incoming), s,
              sizeof s);

    printf("server: got connection from %s\n", s);

    if (!fork()) { // child
      close(listener);
      char *err;

      err = start_handshake();
      if (err)
        goto refuse;
      err = n_connect();
      if (err)
        goto refuse;
      finish_handshake();

      for (;;) {
        fd_set fds;
        int maxfd;
        FD_ZERO(&fds);
        FD_SET(fd, &fds);
        FD_SET(tcpfd, &fds);

        struct timeval timeout;
        timeout.tv_sec = 5; // 5 seconds
        timeout.tv_usec = 0;

        int ready = select(tcpfd + 2, &fds, NULL, NULL, &timeout);
        if (ready == -1)
          die("select");
        else if (ready == 0) {

        } else {
          if (FD_ISSET(fd, &fds)) {
            if (ws_recv() == 1)
                goto closed;
          }

          if (FD_ISSET(tcpfd, &fds)) {
            char buffer[10000];
            memset(buffer, 0, sizeof buffer);
            fcntl(tcpfd, F_SETFL, O_NONBLOCK);
            if (read(tcpfd, buffer, sizeof buffer) == 0)
                goto closed;
            ws_send(buffer, sizeof buffer);
          }
        }
      }

      goto cleanup;
    refuse:
      fprintf(stderr, "server: dropping client: %s\n", err);
      free(err);

      dprintf(fd, "HTTP/1.1 400 Bad Request\r\n");
      dprintf(fd, "\r\n");
    cleanup:
      close(fd);
      exit(0);

    closed:
      printf("server: socket closed by peer");
      goto cleanup;
    }

  free:
    close(fd);
    continue;
  }

  close(listener);
  exit(0);
}
