#ifndef _LWIPOPTS_H
#define _LWIPOPTS_H 1

/** Opcje ogólne **/

/* Używaj zoptymalizowanych operacji odwracania kolejności bajtów,
   zdefiniowanych w pliku arch/cortex-m3.h. */
#define LWIP_PLATFORM_BYTESWAP  1

/* Włącz ochronę krytycznych obszarów kodu podczas przydzielania
   i zwalniania buforów. Stosowne makra są zdefiniowane w pliku
   arch/cortex-m3.h. Patrz też opis w tym pliku. */
#define SYS_LIGHTWEIGHT_PROT    1

/* Nie ma systemu operacyjnego. W wersji 1.4.0 biblioteki lwIP dodano
   moduł implementujący budziki. Nadal używamy własnej implementacji
   budzików i dlatego definiujemy NO_SYS_NO_TIMERS. */
#define NO_SYS                  1
#define NO_SYS_NO_TIMERS        1

/* Używaj tylko interfejsu niskopoziomowego (ang. low-level, core,
   raw API). Nie używaj interfejsu wysokopoziomowego (ang.
   higher-level, sequential, netconn API. Nie używaj interfejsu
   gniazd (ang. socket API). */
#define LWIP_NETCONN            0
#define LWIP_SOCKET             0

/* Optymalizuj - nie zbieraj informacji o błędach. */
#define LWIP_STATS              0

/** Opcje zarządzania pamięcią **/

/* MEM_LIBC_MALLOC == 1 -- używaj free, calloc, malloc i realloc
   dostarczanych przez bibliotekę C. Zmniejsza rozmiar programu,
   gdy korzysta się z tych funkcji również poza lwIP.
   MEM_LIBC_MALLOC == 0 -- używaj procedur zarządzania pamięcią
   dostarczanych przez lwIP.
   Z niejasnych powodów:
   - lwIP nie włącza deklaracji (#include <stdlib.h>) funkcji free,
   calloc, malloc i realloc (calloc i realloc chyba nie są używane
   przez lwIP), co jest potrzebne, gdy MEM_LIBC_MALLOC == 1 i nie
   zostały zdefiniowane własne wersje funkcji mem_free, mem_calloc,
   mem_malloc, mem_realloc;
   - definiuje w mem.h funkcję mem_realloc w ten sposób, że zwraca
   jako wynik wartość pierwszego argumentu, ale na szczęście nigdzie
   nie woła tej funkcji. */
#define MEM_LIBC_MALLOC         1
#define mem_free                protected_free
#define mem_calloc              protected_calloc
#define mem_malloc              protected_malloc
#define mem_realloc             protected_realloc

/* Wyrównywanie w architekturze 32-bitowej */
#define MEM_ALIGNMENT           4

/* Rozmiar puli buforów */
#define PBUF_POOL_SIZE          16

/* Rozmiar bufora, wyrównany do wielokrotności 4 bajtów, musi
   zmieścić cały segment TCP ze wszystkimi nagłówkami: TCP, IP, ETH.
   Będziemy używać sterownika "zero-copy", więc uwzględniamy FCS
   ramki ethernetowej. Na przyszłość uwzględniamy też dodatkowe 4
   bajty na znacznik VLAN. */
#define PBUF_POOL_BUFSIZE       1524

/* Rozmiar wewnętrznej sterty lwIP, alokowanej w sekcji .bss - jeśli
   aplikacja wysyła dużo danych, które wymagają kopiowania, sterta
   powinna być duża. Jeśli MEM_LIBC_MALLOC == 1, używana jest sterta
   C i ta pamięć nie jest alokowana. */
/* #define MEM_SIZE                (8 * 1024) */

/* Liczba struktur opisujących bufory (używane przez opcje PBUF_ROM
   i PBUF_REF) - jeśli aplikacja wysyła dużo danych statycznych, np.
   z ROM lub FLASH, ta liczba powinna być duża. */
#define MEMP_NUM_PBUF           16

/* Liczba jednocześnie otwartych deskryptorów UDP musi uwzględniać
   deskryptory używane przez DHCP i DNS. */
#define MEMP_NUM_UDP_PCB        5

/* Liczba jednocześnie otwartych połączeń TCP */
#define MEMP_NUM_TCP_PCB        2

/* Rozmiar kolejki przychodzących połączeń TCP czekających na
   otwarcie */
#define MEMP_NUM_TCP_PCB_LISTEN 3

/* Liczba jednocześnie kolejkowanych segmentów TCP */
#define MEMP_NUM_TCP_SEG        8

/* Liczba jednocześnie kolejkowanych datagramów IP, czekających na
   złożenie (całych datagramów, nie fragmentów) */
#define MEMP_NUM_REASSDATA      4

/* Liczba jednocześnie kolejkowanych ramek do wysłania, czekających
   na przetłumaczenie adresu za pomocą ARP - wymaga opcji
   ARP_QUEUEING. */
#define MEMP_NUM_ARP_QUEUE      3

/* Rozmiar pamięci podręcznej ARP */
#define ARP_TABLE_SIZE          4

/** Opcje dla poszczególnych protokołów **/

#define ARP_QUEUEING            1
#define ETHARP_TRUST_IP_MAC     1

#define IP_FORWARD              0
#define IP_OPTIONS_ALLOWED      1
#define IP_REASSEMBLY           1
#define IP_FRAG                 1
#define IP_REASS_MAXAGE         3
#define IP_REASS_MAX_PBUFS      10
#define IP_DEFAULT_TTL          64
/* W lwIP 1.3.2 poniższa stała ma domyślną wartość 1, a w lwIP 1.4.0
   ma domyślną wartość 0 i fragmentacja nie działa wtedy poprawnie. */
#define IP_FRAG_USES_STATIC_BUF 1

#define LWIP_ICMP               1
#define ICMP_TTL                64
#define LWIP_BROADCAST_PING     0
#define LWIP_MULTICAST_PING     0

#define LWIP_RAW                0

#define LWIP_DHCP               1

#define LWIP_UDP                1
#define UDP_TTL                 64

#define LWIP_TCP                1
#define TCP_TTL                 64

/* Maksymalny rozmiar segmentu TCP
   TCP_MSS = (Ethernet MTU - IP header size - TCP header size) */
#define TCP_MSS                 (1500 - 40)

/* Okno odbiorcze TCP musi mieć rozmiar co najmniej 2 * TCP_MSS. */
#define TCP_WND                 (4 * TCP_MSS)

/* Używamy sieci lokalnej nie zmieniającej kolejności datagramów.
   Mamy mało pamięci. Nie kolejkuj segmentów TCP przychodzących
   w złej kolejności. */
#define TCP_QUEUE_OOSEQ         0

/* Rozmiar bufora nadawczego TCP w bajtach */
#define TCP_SND_BUF             (4 * TCP_MSS)

/* Liczba struktur pbuf w buforze nadawczym TCP - musi być co
   najmniej 2 * TCP_SND_BUF / TCP_MSS. */
#define TCP_SND_QUEUELEN        (4 * TCP_SND_BUF / TCP_MSS)

#define LWIP_DNS                1
#define DNS_TABLE_SIZE          3
#define DNS_MAX_NAME_LENGTH     48
#define DNS_MAX_SERVERS         2
/* Adres serwera DNS można skonfigurować na kilka sposobów:
   - domyślny "208.67.222.222", zdefiniowany w pliku dns.c,
   - domyślny, zdefiniowany poniżej,
   - uzyskany za pomocą DHCP,
   - skonfigurowany za pomocą funkcji dns_setserver.
   Można też skonfigurować DNS_LOCAL_HOSTLIST, patrz plik opt.h. */

/* Definicja dla 1.3.2. Funkcja inet_addr jest przestarzała, ale jest
   nadal używana w pliku dns.c. Zamiast niej należy używać funkcji
   inet_aton. */
/* #define DNS_SERVER_ADDRESS      inet_addr("194.204.152.34") */

/* Definicja dla lwIP 1.4.0 */
#define DNS_SERVER_ADDRESS(ipaddr)                         \
 (ip4_addr_set_u32(ipaddr, ipaddr_addr("194.204.152.34")))

/** Opcje obliczania sum kontrolnych **/

/*
Funkcja generowania sprzętowo sum kontrolnych IP, UDP, TCP i ICMP
przez STM32F107 nie działa prawidłowo, gdy występuje fragmentacja IP.
Prawdopodobnie nie będzie też działać, gdy nagłówk IP będzie zawierał
dodatkowe opcje. Dlatego została wyłączona. Błąd można zreprodukować,
wysyłając ping o długości większej niż 1474 bajtów, np.
ping -c 1 -n -v -s 1476 192.168.51.84

Raport z tcpdump
/usr/sbin/tcpdump -p -vv -s 1524 -i eth0

17:04:59.007401 IP (tos 0x0, ttl  64, id 1253, offset 0,
flags [+], proto 1, length: 1500) 192.168.51.88 > 192.168.51.84:
icmp 1480: echo request seq 0
17:04:59.007437 IP (tos 0x0, ttl  64, id 1253, offset 1480,
flags [none], proto 1, length: 24) 192.168.51.88 > 192.168.51.84:
icmp
17:04:59.008768 IP (tos 0x0, ttl 255, id 1253, offset 0,
flags [+], proto 1, length: 1500) 192.168.51.84 > 192.168.51.88:
icmp 1480: echo reply seq 0
17:04:59.008803 IP (tos 0x0, ttl 255, id 1253, offset 1480,
flags [none], proto 1, length: 24) 192.168.51.84 > 192.168.51.88:
icmp

Sprzęt niepotrzebnie wstawia sumę kontrolną w dwóch ostatnich
oktetach drugiego segmentu. */

/* Generuj sprzętowo sumę kontrolną dla wysyłanych datagramów IP.
   To możemy włączyć, ponieważ lwIP prawdopodobnie nigdy nie
   wygeneruje nagłówka IP z dodatkowymi opcjami. */
#define CHECKSUM_GEN_IP       0

/* Sprawdzaj programowo sumę kontrolną odbieranych datagramów IP.
   Tak musi być, gdyż stos w żaden sposób nie potrafi skorzystać
   z informacji o stwierdzeniu błędnej sumy kontrolnej, przekazanej
   mu przez sprzęt. Mogą też pojawić się nagłówki IP z dodatkowymi
   opcjami. */
#define CHECKSUM_CHECK_IP       1

/* Generuj i sprawdzaj programowo sumy kontrolne UDP i TCP. */
#define CHECKSUM_GEN_UDP        1
#define CHECKSUM_GEN_TCP        1
#define CHECKSUM_CHECK_UDP      1
#define CHECKSUM_CHECK_TCP      1

#endif
