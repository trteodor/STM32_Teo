/* Simple UDP server using socket implementation.
   Usage: linux_recv_bcast port */

#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

#define BSIZE 1024

int main(int argc, char *argv[]) {
  int                sock;
  ssize_t            n;
  struct sockaddr_in server_addr, client_addr;
  socklen_t          client_addr_len;
  in_port_t          server_port;
  char               buffer[BSIZE];

  if (argc != 2) {
    fprintf(stderr, "usage: %s port\n", argv[0]);
    return EXIT_FAILURE;
  }

  /* Create UDP socket. */
  sock = socket(PF_INET, SOCK_DGRAM, 0);
  if (sock < 0) {
    perror("socket");
    return EXIT_FAILURE;
  }

  /* Bind a local address. */
  server_port = (in_port_t) atoi(argv[1]);
  memset((void *)&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family      = AF_INET;
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  server_addr.sin_port        = htons(server_port);
  if (bind(sock, (struct sockaddr*)&server_addr,
           sizeof server_addr) < 0) {
    perror("bind");
    return EXIT_FAILURE;
  }

  printf("Listening on port %hu\n\n", ntohs(server_addr.sin_port));

  for (;;) {
    /* Wait for a message. */
    memset(buffer, 0, sizeof(buffer));
    n = recvfrom(sock, buffer, BSIZE, 0,
                 (struct sockaddr*)&client_addr, &client_addr_len);
    if (n <= 0) {
      perror("recv");
      exit(EXIT_FAILURE);
    }
    printf("From:%s:%hu\nContent:%.*s\n", inet_ntoa(client_addr.sin_addr),
           ntohs(client_addr.sin_port), (int)n, buffer);
  }
}
