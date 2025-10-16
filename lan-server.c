// Sequential socket server - accepting one client at a time.
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "utils.h"

typedef enum { DISCON, CON } ConnectionState;

void serve_connection(int sock_fd) {
  // Clients attempting to connect and send data will succeed even before the
  // connection is accept()-ed by the server. Therefore, to better simulate
  // blocking of other clients while one is being served, do this "ack" from the
  // server which the client expects to see before proceeding.
  if (send(sock_fd, "S", 1, 0) < 1) {
    perror_die("send");
  }

  ConnectionState state = DISCON;

  while (1) {
    uint8_t buf[1024];
    int len = recv(sock_fd, buf, sizeof buf, 0);
    if (len < 0) {
      perror_die("recv");
    } else if (len == 0) {
      break;
    }

    for (int i = 0; i < len; ++i) {
      switch (state) {
      case DISCON:
        if (buf[i] == 'C') {
          state = CON;
        }
        break;
      case CON:
        if (buf[i] == 'D') {
          state = DISCON;
        } else {
          buf[i] += 1;
          if (send(sock_fd, &buf[i], 1, 0) < 1) {
            perror("send error");
            close(sock_fd);
            return;
          }
        }
        break;
      }
    }
  }

  close(sock_fd);
}

int main(int argc, char** argv) {
  setvbuf(stdout, NULL, _IONBF, 0);

  int port_num = 9090;
  if (argc >= 2) {
    port_num = atoi(argv[1]);
  }
  printf("Serving on port %d\n", port_num);

  int sock_fd = listen_inet_socket(port_num);

  while (1) {
    struct sockaddr_in peer_addr;
    socklen_t peer_addr_len = sizeof(peer_addr);

    int new_sock_fd =
        accept(sock_fd, (struct sockaddr*)&peer_addr, &peer_addr_len);

    if (new_sock_fd < 0) {
      perror_die("ERROR on accept");
    }

    report_peer_connected(&peer_addr, peer_addr_len);
    serve_connection(new_sock_fd);
    printf("peer done\n");
  }

  return 0;
}
