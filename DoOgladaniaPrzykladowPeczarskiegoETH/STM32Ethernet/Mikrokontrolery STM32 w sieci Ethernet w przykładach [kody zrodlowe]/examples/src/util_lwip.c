#include <util_delay.h>
#include <util_lwip.h>
#include <util_time.h>
#include <lwip/init.h>
#include <lwip/ip_frag.h>
#include <lwip/dhcp.h>
#include <lwip/dns.h>
/* Stałe LWIP_VERSION_MAJOR i LWIP_VERSION_MINOR są zdefiniowane
   w lwip/init.h. */
#if LWIP_VERSION_MAJOR == 1 && LWIP_VERSION_MINOR == 3
  #include <lwip/tcp.h>
#elif LWIP_VERSION_MAJOR == 1 && LWIP_VERSION_MINOR == 4
  #include <lwip/tcp_impl.h>
#else
  #error Unknown lwIP version
#endif
#include <netif/etharp.h>

/* Ta funkcja nie jest wołana w kontekście procedury
   obsługi przerwania. */
int LWIPinterfaceInit(struct netif *netif,
                      struct ethnetif *ethnetif) {
  struct ip_addr ipaddr, netmask, gw;
  struct netif *p;
  #if LWIP_DHCP
    int dhcpFlag;
  #endif
  IRQ_DECL_PROTECT(x);

  if (netif->ip_addr.addr == 0) {
    #if LWIP_DHCP
      ipaddr.addr = 0;
      netmask.addr = 0;
      gw.addr = 0;
      dhcpFlag = 1;
    #else
      return -1;
    #endif
  }
  else {
    ipaddr = netif->ip_addr;
    netmask = netif->netmask;
    gw = netif->gw;
    #if LWIP_DHCP
      dhcpFlag = 0;
    #endif
  }
  IRQ_PROTECT(x, LWIP_IRQ_PRIO);
  lwip_init();
  p = netif_add(netif,
                &ipaddr,
                &netmask,
                &gw,
                ethnetif, /* wskaźnik do prywatnych danych */
                &ETHinit, /* nasza funkcja inicjująca interfejs */
                &ethernet_input /* funkcja stosu lwIP przetwarzająca
                                   odebrane ramki ethernetowe */);
  IRQ_UNPROTECT(x);
  if (!p)
    return -1;

  IRQ_PROTECT(x, LWIP_IRQ_PRIO);
  netif_set_default(netif);
  IRQ_UNPROTECT(x);

  /* dhcp_start() i netif_set_up() ustawiają m.in.
     netif->flags |= NETIF_FLAG_UP; */
  #if LWIP_DHCP
    if (dhcpFlag) {
      err_t err;

      IRQ_PROTECT(x, LWIP_IRQ_PRIO);
      err = dhcp_start(netif);
      IRQ_UNPROTECT(x);
      if (err != ERR_OK)
        return -1;
    }
    else {
  #endif
      IRQ_PROTECT(x, LWIP_IRQ_PRIO);
      netif_set_up(netif);
      IRQ_UNPROTECT(x);
  #if LWIP_DHCP
    }
  #endif

  return 0;
}

/* Ta funkcja nie jest wołana w kontekście procedury obsługi
   przerwania. Ochrona jest tu przesadna, bo tylko czytamy dane,
   ale przypomina, że zdarzenia zachodzą współbieżnie.
   Czekaj maksymalnie timeout sekund lub tries prób. */
int DHCPwait(struct netif *netif, unsigned timeout, unsigned tries) {
  #if LWIP_DHCP
    volatile uint8_t netif_flags;
    IRQ_DECL_PROTECT(x);

    IRQ_PROTECT(x, LWIP_IRQ_PRIO);
    netif_flags = netif->flags;
    IRQ_UNPROTECT(x);
    if (netif_flags & NETIF_FLAG_DHCP) {
      ltime_t t;

      t = LocalTime() + timeout * 1000;
      while (t > LocalTime()) {
        volatile uint8_t dhcp_tries;
        volatile uint8_t dhcp_state;

        /* Nie należy sprawdzać zbyt często. */
        Delay(1000000);

        IRQ_PROTECT(x, LWIP_IRQ_PRIO);
        dhcp_tries = netif->dhcp->tries;
        dhcp_state = netif->dhcp->state;
        IRQ_UNPROTECT(x);

        if (dhcp_state == DHCP_BOUND)
          return 0;
        else if (dhcp_tries >= tries) {
          IRQ_PROTECT(x, LWIP_IRQ_PRIO);
          dhcp_stop(netif);
          IRQ_UNPROTECT(x);
          return -1;
        }
      }
      return -1;
    }
  #endif
  return 0;
}

/** Budziki lwIP **/

#if IP_REASSEMBLY == 1
  static ltime_t IPreassemblyTimer;
#endif
#if LWIP_ARP
  static ltime_t ARPtimer;
#endif
#if LWIP_TCP
  static ltime_t TCPtimer;
#endif
#if LWIP_DHCP
  static ltime_t DHCPfineTimer;
  static ltime_t DHCPcoarseTimer;
#endif
#if LWIP_DNS
  static ltime_t DNStimer;
#endif

/* SysTick ma wyższy priorytet niż lwIP - czas musi biegnąć podczas
   przetwarzania ramek. SysTick wywołuje cyklicznie funkcję zwrotną
   (ang. call back) LWIPtmr, która zgłasza przerwanie PendSV,
   a dopiero procedura obsługi tego przerwania obsługuje właściwe
   budziki w lwIP, gdyż chcemy, aby SysTick i budziki działały
   z różnymi priorytetami. */

/* Główny budzik wołany przez SysTick, wyzwala przerwanie PendSV. */
static void LWIPtmr(void) {
  SCB->ICSR |= SCB_ICSR_PENDSVSET;
}

/* Uaktywnia główny budzik. Nie jest wołana w kontekście procedury
   obsługi przerwania - ochrona jest też w funkcji TimerCallBack.
   Dobieramy początkowe wartości, aby uniknąć czasowej korelacji
   wołania budzików. */
void LWIPtimersStart() {
  IRQ_DECL_PROTECT(x);
  ltime_t localTime = LocalTime();

  IRQ_PROTECT(x, LWIP_IRQ_PRIO);
  #if IP_REASSEMBLY == 1
    IPreassemblyTimer = localTime + 53;
  #endif
  #if LWIP_ARP
    ARPtimer = localTime + 97;
  #endif
  #if LWIP_TCP
    TCPtimer = localTime + 11;
  #endif
  #if LWIP_DHCP
    DHCPfineTimer = localTime + 31;
    DHCPcoarseTimer = localTime;
  #endif
  #if LWIP_DNS
    DNStimer = localTime + 73;
  #endif
  SET_PRIORITY(PendSV_IRQn, LWIP_IRQ_PRIO, 0);
  TimerCallBack(LWIPtmr);
  IRQ_UNPROTECT(x);
}

/* Budzik aplikacji app_tmr jest wołany co app_tmr_interval ms. */
static app_callback_t app_tmr = 0;
static unsigned app_tmr_interval = 0;
static ltime_t appTimer = 0;

/* Uaktywnia budzik aplikacji. Nie jest wołana w kontekście
   procedury obsługi przerwania. */
void LWIPsetAppTimer(app_callback_t tmr, unsigned tmr_interval) {
  IRQ_DECL_PROTECT(x);

  IRQ_PROTECT(x, LWIP_IRQ_PRIO);
  app_tmr = tmr;
  app_tmr_interval = tmr_interval;
  appTimer = LocalTime();
  IRQ_UNPROTECT(x);
}

/* Procedura obsługi przerwania PendSV, wołająca budziki lwIP:
   ip_reass_tmr    IP_TMR_INTERVAL          1000 ms
   etharp_tmr      ARP_TMR_INTERVAL         5000 ms
   tcp_tmr         TCP_TMR_INTERVAL          250 ms
   dhcp_fine_tmr   DHCP_FINE_TIMER_MSECS     500 ms
   dhcp_coarse_tmr DHCP_COARSE_TIMER_MSECS 60000 ms
   dhcp_coarse_tmr DHCP_COARSE_TIMER_SECS     60 s
   dns_tmr         DNS_TMR_INTERVAL         1000 ms
   autoip_tmr      AUTOIP_TMR_INTERVAL       100 ms
   igmp_tmr        IGMP_TMR_INTERVAL         100 ms
   Zakładamy, że 64-bitowe liczniki czasu w praktyce nigdy nie
   przepełnią się. */
void PendSV_Handler() {
  ltime_t localTime = LocalTime();
  /* Wydaje się niepotrzebne.
  SCB->ICSR |= SCB_ICSR_PENDSVCLR;
  */

  #if IP_REASSEMBLY == 1
    if (localTime >= IPreassemblyTimer) {
      IPreassemblyTimer += IP_TMR_INTERVAL;
      ip_reass_tmr();
    }
  #endif
  #if LWIP_ARP
    if (localTime >= ARPtimer) {
      ARPtimer += ARP_TMR_INTERVAL;
      etharp_tmr();
    }
  #endif
  #if LWIP_TCP
    if (localTime >= TCPtimer) {
      TCPtimer += TCP_TMR_INTERVAL;
      tcp_tmr();
    }
  #endif
  #if LWIP_DHCP
    if (localTime >= DHCPfineTimer) {
      DHCPfineTimer += DHCP_FINE_TIMER_MSECS;
      dhcp_fine_tmr();
    }
    if (localTime >= DHCPcoarseTimer) {
      DHCPcoarseTimer += DHCP_COARSE_TIMER_MSECS;
      dhcp_coarse_tmr();
    }
  #endif
  #if LWIP_DNS
    if (localTime >= DNStimer) {
      DNStimer += DNS_TMR_INTERVAL;
      dns_tmr();
    }
  #endif
  /* Ten budzik jest zadeklarowany jako globalny i jest inicjowany
     w funkcji LWIPsetAppTimer. */
  if (app_tmr_interval > 0 && localTime >= appTimer) {
    appTimer += app_tmr_interval;
    if (app_tmr)
      app_tmr();
  }
}
