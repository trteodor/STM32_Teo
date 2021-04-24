/* Serwery NTP, źródło Wikipedia

  Oficjalne serwery NTP dostarczające czas urzędowy w Polsce:
    tempus1.gum.gov.pl, 212.244.36.227, atomowy zegar cezowy 5071A,
      Główny Urząd Miar;
    tempus2.gum.gov.pl, 212.244.36.228, atomowy zegar cezowy 5071A,
      Główny Urząd Miar.

  Serwery NTP grupy laboratoriów porównujących wzorce czasu:
    vega.cbk.poznan.pl, 150.254.183.15, atomowy zegar cezowy 5071A,
      Obserwatorium Astrogeodynamiczne w Borówcu koło Poznania;
    ntp.itl.waw.pl, 193.110.137.171, atomowy zegar cezowy 5071A,
      Instytut Łączności w Warszawie;
    ntp.elproma.com.pl, 83.19.137.3, rubidowy atomowy wzorzec firmy
      STANFORD Research, Łomianki koło Warszawy.

  Pule serwerów NTP:
    1.pl.pool.ntp.org (Polska),
    1.europe.pool.ntp.org (Europa),
    1.pool.ntp.org (Świat).
*/

#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define BSIZE   64

struct ntp_timestamp {
  uint32_t Seconds;
  uint32_t SecondsFraction;
};

struct ntp_msg {
  uint8_t LI_VN_Mode;
  uint8_t Stratum;
  uint8_t Poll;
  int8_t Precision;
  uint32_t RootDelay;
  uint32_t RootDispersion;
  uint8_t ReferenceIdentifier[4];
  struct ntp_timestamp ReferenceTimestamp;
  struct ntp_timestamp OriginateTimestamp;
  struct ntp_timestamp ReceiveTimestamp;
  struct ntp_timestamp TransmitTimestamp;
};

#define NTP_PORT        123
#define NTP_TO_UNIX     2208988800U

#define NTP_LI_NO_WARNING               0
#define NTP_LI_ALARM_CONDITION          3
#define NTP_VERSION                     4
#define NTP_MODE_CLIENT                 3
#define NTP_MODE_SERVER                 4
#define NTP_STRATUM_KISS_OF_DEATH       0
#define NTP_STRATUM_MAX                 15

#define ntp_leap_indicator(pdu) ((uint8_t)((pdu)->LI_VN_Mode >> 6))
#define ntp_version(pdu) ((uint8_t)(((pdu)->LI_VN_Mode >> 3) & 0x7))
#define ntp_mode(pdu) ((uint8_t)((pdu)->LI_VN_Mode & 0x7))
#define ntp_compose_li_vn_mode(li, vn, mode)            \
 (((li) << 6) | (((vn) << 3) & 0x38) | ((mode) & 0x7))

static void fraction_print(const char *name, uint32_t x) {
  printf("%-20s %9u %.9f\n", name, x, (double)x / 65536.);
}

static uint32_t foo(uint32_t x) {
  return (uint32_t)((double)x * 1e9 / 4294967296. + .5);
}

static void timestamp_print(const char *name,
                            struct ntp_timestamp ts) {
  struct tm tm;
  time_t    rtime;
  size_t    size;
  char      buf[BSIZE];

  rtime = ntohl(ts.Seconds) - NTP_TO_UNIX;
  localtime_r(&rtime, &tm);
  size = strftime(buf, BSIZE, "%Y.%m.%d %H:%M:%S", &tm);
  printf("%-20s %08x.%08x %.*s.%09u\n",
         name,
         ntohl(ts.Seconds),
         ntohl(ts.SecondsFraction),
         (int)size,
         buf,
         foo(ntohl(ts.SecondsFraction)));
}

int main(int argc, char * argv[]) {
  int                sock ;
  struct sockaddr_in client;
  struct sockaddr_in server1;
  struct sockaddr_in server2;
  struct hostent     *host;
  struct ntp_msg     msg;
  ssize_t            count;
  socklen_t          server_addr_len;
  uint32_t           msg_id;

  if (argc != 2) {
    fprintf(stderr, "SNTP client\nUsage:\n%s hostname\n", argv[0]);
    return 1;
  }

  /* Utwórz gniazdo UDP. */
  sock = socket(PF_INET, SOCK_DGRAM, 0);

  /* Zwiąż gniazdo z lokalnym adresem. */
  client.sin_family      = AF_INET;
  client.sin_addr.s_addr = ntohl(INADDR_ANY);
  client.sin_port        = 0; /* 0 oznacza dowolny port. */
  if (bind(sock, (struct sockaddr*) &client, sizeof client) < 0) {
    fprintf(stderr, "bind failed\n");
    return 1;
  }

  /* Użyj DNS-a, aby poznać adres serwera. */
  host = gethostbyname(argv[1]);
  if (host == NULL) {
    fprintf(stderr, "unknown host %s\n", argv[1]);
    return 1;
  }

  /* Skonfiguruj adres i numer portu serwera. */
  server1.sin_family = AF_INET;
  memcpy(&server1.sin_addr.s_addr,
         host->h_addr,
         (size_t)host->h_length);
  server1.sin_port = htons(NTP_PORT);

  /* Utwórz pakiet zawierający żądanie. */
  memset(&msg, 0, sizeof msg);
  msg.LI_VN_Mode = ntp_compose_li_vn_mode(NTP_LI_NO_WARNING,
                                          NTP_VERSION,
                                          NTP_MODE_CLIENT);
  msg.TransmitTimestamp.Seconds = htonl(time(0) + NTP_TO_UNIX);
  /* Pole TransmitTimestamp.SecondsFraction w całości lub części może
     zgodnie z RFC4330 być użyte do identyfikacji odpowiedzi. Wartość
     wysłana w TransmitTimestamp wraca w OriginateTimestamp. Jako
     msg_id najlepiej jest użyć jakiejś pseudolosowej, trudnej do
     przewidzenia wartości. */
  srand(msg.TransmitTimestamp.Seconds);
  msg_id = rand();
  msg.TransmitTimestamp.SecondsFraction = htonl(msg_id);

  /* Wyślij żądanie. */
  count = sendto(sock, &msg, sizeof msg, 0,
                 (struct sockaddr *)&server1, sizeof server1);
  if (count < 0) {
    perror("sendto");
    return 1;
  }
  else if (count != sizeof msg) {
    fprintf(stderr, "sendto failed\n");
    return 1;
  }
  printf("Request send to     %s:%hu\n",
         inet_ntoa(server1.sin_addr),
         ntohs(server1.sin_port));

  /* Odbierz odpowiedź. */
  server_addr_len = sizeof(server2);
  count = recvfrom(sock, &msg, sizeof msg, 0,
                   (struct sockaddr *)&server2, &server_addr_len);
  if (count < 0) {
    perror("recvfrom");
    return 1;
  }
  else if (count != sizeof msg) {
    fprintf(stderr, "recvfrom failed\n");
    return 1;
  }
  else if (server1.sin_addr.s_addr != server2.sin_addr.s_addr ||
           server1.sin_port != server1.sin_port) {
    fprintf(stderr, "Answer received from other server.\n");
  }
  printf("Reply received from %s:%hu\n",
         inet_ntoa(server2.sin_addr),
         ntohs(server2.sin_port));

  /* Sprawdź, czy otrzymano poprawną odpowiedź. */
  if (ntp_leap_indicator(&msg) == NTP_LI_ALARM_CONDITION ||
      ntp_version(&msg) != NTP_VERSION ||
      ntp_mode(&msg) != NTP_MODE_SERVER ||
      msg.Stratum == NTP_STRATUM_KISS_OF_DEATH ||
      msg.Stratum > NTP_STRATUM_MAX ||
      ntohl(msg.OriginateTimestamp.SecondsFraction) != msg_id ||
      (msg.TransmitTimestamp.Seconds == 0 &&
       msg.TransmitTimestamp.SecondsFraction == 0))
    printf("Wrong reply\n");

  /* Wypisz otrzymaną odpowiedź. */
  printf("Leap Indicator %hhu\n"
         "Version %hhu\n"
         "Mode %hhu\n"
         "Stratum %hhu\n"
         "Poll %hhu\n"
         "Precision %hhd\n",
         ntp_leap_indicator(&msg),
         ntp_version(&msg),
         ntp_mode(&msg),
         msg.Stratum,
         msg.Poll,
         msg.Precision);
  fraction_print("Root Delay", ntohl(msg.RootDelay));
  fraction_print("Root Dispersion", ntohl(msg.RootDispersion));
  if (msg.Stratum <= 1)
    printf("Reference Identifier %-.4s\n", msg.ReferenceIdentifier);
  else
    printf("Reference Identifier %hhu.%hhu.%hhu.%hhu\n",
           msg.ReferenceIdentifier[0],
           msg.ReferenceIdentifier[1],
           msg.ReferenceIdentifier[2],
           msg.ReferenceIdentifier[3]);
  timestamp_print("Reference Timestamp", msg.ReferenceTimestamp);
  timestamp_print("Originate Timestamp", msg.OriginateTimestamp);
  timestamp_print("Receive Timestamp",   msg.ReceiveTimestamp);
  timestamp_print("Transmit Timestamp",  msg.TransmitTimestamp);
  printf("Round Time Trip %lu\n",
         time(0) + NTP_TO_UNIX -
         ntohl(msg.OriginateTimestamp.Seconds));

  return 0;
}
