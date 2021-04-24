/* Simple UDP client able to send broadcast packets.
   Usage: linux_send_bcast hostname port */

#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define BUF_SIZE 64

int main(int argc, char * argv[]) {
  int                sock;
  int                optval;
  int                count;
  int                size;
  struct sockaddr_in remote_addr;
  struct sockaddr_in local_addr;
  struct hostent     *host;
  char               buf[BUF_SIZE];

  if (argc != 3) {
    fprintf(stderr, "usage: %s host port\n", argv[0]);
    return EXIT_FAILURE;
  }

  /* Create broadcast capable UDP socket. */
  sock = socket(PF_INET, SOCK_DGRAM, 0);
  optval = 1;
  setsockopt(sock, SOL_SOCKET, SO_BROADCAST,
             (void*)&optval, sizeof optval);

  /* Bind a local address. */
  local_addr.sin_family      = AF_INET;
  local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  local_addr.sin_port        = 0;
  if (bind(sock, (struct sockaddr*)&local_addr,
           sizeof local_addr) < 0) {
    perror("bind");
    return EXIT_FAILURE;
  }

  /* Compose the remote address. */
  host = gethostbyname(argv[1]);
  if (host == NULL) {
    fprintf(stderr, "unknown host: %s\n", argv[1]);
    return EXIT_FAILURE;
  }
  remote_addr.sin_family = AF_INET;
  memcpy(&remote_addr.sin_addr.s_addr, host->h_addr,
         (size_t)host->h_length);
  remote_addr.sin_port = (in_port_t)htons(atoi(argv[2]));

  for (count = 1;; ++count) {
    /* Build a packet. */
    size = snprintf(buf, BUF_SIZE, "%d %s:%hu\r\n",
                    count, inet_ntoa(remote_addr.sin_addr),
                    ntohs(remote_addr.sin_port));
    if (size >= BUF_SIZE)
      size = BUF_SIZE - 1;

    /* Send the packet. */
    if (sendto(sock, buf, size, 0,
               (struct sockaddr*)&remote_addr,
               sizeof remote_addr) < 0)
      perror("sendto");
    printf(buf);

    sleep(4);
  }
}
