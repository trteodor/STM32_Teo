#include <misc.h>
#include <stm32_eth.h>
#include <util_eth.h>
#include <netif/etharp.h>

/* Liczba buforów odbiorczych i nadawczych DMA */
#define ETH_RXBUFNB 4
#define ETH_TXBUFNB 4

/* Trzeba jakoś przekazać ten wskaźnik do ETH_IRQHandler(). */
static struct netif *p_netif;

/* Implementacja funkcjonalności low_level_init */
static int  ETHconfigurePHY(uint8_t);
static void ETHconfigureMAC(struct netif *);
static void ETHconfigureDMA(struct netif *, uint8_t, uint8_t);

/* Funkcje ze szkieletu sterownika dla lwIP. Zachowano oryginalne
   angielskie komentarze do tych funkcji. */
static err_t low_level_output(struct netif *, struct pbuf *);
static err_t ethernetif_input(struct netif *);
static struct pbuf * low_level_input(struct netif *);

/** KONFIGUROWANIE INTERFEJSU ETHERNETOWEGO **/

int ETHconfigurePHY(uint8_t phyAddress) {
  ETH_InitTypeDef e;

  /* W większości przypadków wystarczają ustawienia domyślne. */
  ETH_StructInit(&e);

  /* Parametry, których domyślne wartości zmieniamy. Domyślnie jest
     ETH_AutoNegotiation_Disable i 10 Mb/s half-duplex. */
  e.ETH_AutoNegotiation = ETH_AutoNegotiation_Enable;
  e.ETH_BroadcastFramesReception =
    ETH_BroadcastFramesReception_Enable;
  /* Zastosowanie tej opcji wymaga potestowania, domyślnie jest
     enabled, co umożliwia odbieranie ramek w half-duplex, gdy
     ustawiony jest sygnał TX_EN. */
  e.ETH_ReceiveOwn = ETH_ReceiveOwn_Disable;
  /* Wolimy domyślne ustawienie - enabled.
  e.ETH_RetryTransmission = ETH_RetryTransmission_Disable;
  */
  /* To jest ustawione domyślnie, a FCS jest odrzucana w sterowniku.
  e.ETH_AutomaticPadCRCStrip = ETH_AutomaticPadCRCStrip_Disable;
  */
  #if CHECKSUM_GEN_IP == 0
  e.ETH_ChecksumOffload = ETH_ChecksumOffload_Enable;
  e.ETH_DropTCPIPChecksumErrorFrame =
    ETH_DropTCPIPChecksumErrorFrame_Disable;
  #endif
  /* Gdy używana jest funkcjonalności "checksum offload", trzeba
     uaktywnić tryb "store and forward". Ten tryb gwarantuje, że
     cała ramka jest gromadzona w FIFO, tak że MAC może wstawić lub
     zweryfikować sumę kontrolną. Poniższe ustawienia są domyślne.
  e.ETH_ReceiveStoreForward = ETH_ReceiveStoreForward_Enable;
  e.ETH_TransmitStoreForward = ETH_TransmitStoreForward_Enable;
  */
  /* Optymalizuj wysyłanie i transakcje DMA. */
  e.ETH_SecondFrameOperate = ETH_SecondFrameOperate_Enable;
  e.ETH_FixedBurst = ETH_FixedBurst_Enable;
  e.ETH_RxDMABurstLength = ETH_RxDMABurstLength_32Beat;
  e.ETH_TxDMABurstLength = ETH_TxDMABurstLength_32Beat;
  /* To jest domyślne.
  e.ETH_DMAArbitration = ETH_DMAArbitration_RoundRobin_RxTx_1_1;
  */

  if (!ETH_Init(&e, phyAddress))
    return -1;

  #if ETH_BOARD == ZL3ETH
    #define PHYCR     0x19
    #define LED_CNFG0 0x0020
    #define LED_CNFG1 0x0040
    uint16_t phyreg;

    /* konfiguracja diod świecących na ZL3ETH
       zielona - link status:
       on = good link, off = no link, blink = activity
       pomarańczowa - speed:
       on = 100 Mb/s, off = 10 Mb/s */
    phyreg = ETH_ReadPHYRegister(phyAddress, PHYCR);
    phyreg &= ~(LED_CNFG0 | LED_CNFG1);
    ETH_WritePHYRegister(phyAddress, PHYCR, phyreg);
  #endif

  return 0;
}

void ETHconfigureMAC(struct netif *netif) {
  netif->hwaddr_len = ETHARP_HWADDR_LEN;
  netif->mtu = MAX_ETH_PAYLOAD;
  netif->flags = NETIF_FLAG_BROADCAST |
                 NETIF_FLAG_ETHARP;
  ETH_MACAddressConfig(ETH_MAC_Address0, netif->hwaddr);
}

void ETHconfigureDMA(struct netif *netif,
                     uint8_t priority, uint8_t subpriority) {
  static ETH_DMADESCTypeDef DMARxDscrTab[ETH_RXBUFNB] ALIGN4;
  static ETH_DMADESCTypeDef DMATxDscrTab[ETH_TXBUFNB] ALIGN4;

  /* Ramki VLAN nie będą poprawnie obsługiwane. Stała
     ETH_MAX_PACKET_SIZE, zdefiniowana w pliku stm32_eth.h, ma
     wartość 1520 i nie uwzględnia obecności znacznika VLAN. Powinna
     mieć wartość 1524. Stała ta jest powszechnie używana w pliku
     stm32_eth.c, m.in. jako domyślna długość bufora DMA w wołanych
     poniżej funkcjach ETH_DMARxDescChainInit()
     i ETH_DMATxDescChainInit().
     TODO: Zmienić wartość stałej ETH_MAX_PACKET_SIZE,
     zaimplementować obsługę ramek VLAN. */
  static uint8_t RxBuff[ETH_RXBUFNB][ETH_MAX_PACKET_SIZE] ALIGN4;
  static uint8_t TxBuff[ETH_TXBUFNB][ETH_MAX_PACKET_SIZE] ALIGN4;

  ETH_DMARxDescChainInit(DMARxDscrTab, &RxBuff[0][0], ETH_RXBUFNB);
  ETH_DMATxDescChainInit(DMATxDscrTab, &TxBuff[0][0], ETH_TXBUFNB);

  NVIC_InitTypeDef NVIC_InitStruct;
  int i;

  /* Trzeba jakoś przekazać ten wskaźnik do ETH_IRQHandler(). */
  p_netif = netif;

  NVIC_InitStruct.NVIC_IRQChannel = ETH_IRQn;
  NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = priority;
  NVIC_InitStruct.NVIC_IRQChannelSubPriority = subpriority;
  NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStruct);

  /* Zgłoś przerwanie po odebraniu ramki. */
  ETH_DMAITConfig(ETH_DMA_IT_NIS | ETH_DMA_IT_R, ENABLE);
  for (i = 0; i < ETH_RXBUFNB; ++i)
    ETH_DMARxDescReceiveITConfig(&DMARxDscrTab[i], ENABLE);

  #if CHECKSUM_GEN_IP == 0
    for (i = 0; i < ETH_TXBUFNB; ++i)
      ETH_DMATxDescChecksumInsertionConfig(&DMATxDscrTab[i],
                                ETH_DMATxDesc_CIC_IPV4Header);
  #endif
}

/**
 * Should be called at the beginning of the program to set up the
 * network interface.  It calls the function low_level_init() to do
 * the actual setup of the hardware.
 *
 * This function should be passed as a parameter to netif_add().
 *
 * @param netif the lwip network interface structure for this
 *              ethernetif
 * @return      ERR_OK if the interface is initialized,
 *              ERR_MEM if private data couldn't be allocated
 *              or any other err_t on error
 */
err_t ETHinit(struct netif *netif) {
  struct ethnetif *ethnetif = netif->state;

  /* Wołanie low_level_init() zostało rozbite na trzy funkcje. */
  if (ETHconfigurePHY(ethnetif->phyAddress) < 0)
    return ERR_IF;
  ETHconfigureMAC(netif);
  ETHconfigureDMA(netif, LWIP_IRQ_PRIO, 0);

  /* Tu jest miejsce dla różnych dodatkowych ustawień,
     np. nazwy interfejsu. */
  netif->name[0] = 's';
  netif->name[1] = 't';

  /* Inicjuj w strukturze netif pola związane z SNMP. Ostatni
     argument to przepływność łącza w bitach na sekundę.
     TODO: Zamiast magicznej liczby 100000000 trzeba wpisywać tu
     aktualną wartość odczytaną z PHY. */
  NETIF_INIT_SNMP(netif, snmp_ifType_ethernet_csmacd, 100000000);

  /* TODO: Dodać funkcjonalność LWIP_NETIF_HOSTNAME. */
  #if LWIP_NETIF_HOSTNAME
    netif->hostname = "lwip";
  #endif

  /* Inicjuj statystyki. */
  ethnetif->RX_packets = 0;
  ethnetif->TX_packets = 0;
  ethnetif->RX_errors = 0;
  ethnetif->TX_errors = 0;

  /* Jako funkcji output(), używaj bezpośrednio zdefiniowanej
     w stosie lwIP funkcji etharp_output(). Jeśli przed wysłaniem
     ramki trzeba wykonać jakieś działania, np. sprawdzić stan łącza,
     to można zdefiniować własną funkcję, która następnie wywoła
     funkcję etharp_output(). Jako funkcji linkoutput() używaj
     niskopoziomowej funkcji low_level_output(), która wysyła ramkę
     bezpośrednio do interfejsu sieciowego. */
  netif->output = etharp_output;
  netif->linkoutput = low_level_output;

  ETH_Start();
  netif->flags |= NETIF_FLAG_LINK_UP;
  return ERR_OK;
}

/** WYSYŁANIE RAMEK ETHERNETOWYCH **/

/* Zmienna zdefiniowana w stm32_eth.c, wskazuje na bieżący
   deskryptor nadawczy DMA. */
extern ETH_DMADESCTypeDef *DMATxDescToSet;

/**
 * This function should do the actual transmission of the packet.
 * The packet is contained in the pbuf that is passed to the
 * function.  This pbuf might be chained.
 *
 * @param netif the lwip network interface structure for this
 *              ethernetif
 * @param p     the MAC packet to send (e.g. IP packet including MAC
 *              addresses and type)
 * @return      ERR_OK if the packet could be sent or
 *              an err_t value if the packet couldn't be sent
 *
 * @note Returning ERR_MEM here if a DMA queue of your MAC is full
 *       can lead to strange results.  You might consider waiting for
 *       space in the DMA queue to become availale since the stack
 *       doesn't retry to send a packet dropped because of memory
 *       failure (except for the TCP timers).
 */
err_t low_level_output(struct netif *netif, struct pbuf *p) {
  ((struct ethnetif *)netif->state)->TX_packets++;

  if (p == NULL) {
    ((struct ethnetif*)netif->state)->TX_errors++;
    return ERR_ARG;
  }
  if (p->tot_len > ETH_MAX_PACKET_SIZE - ETH_CRC) {
    ((struct ethnetif *)netif->state)->TX_errors++;
    return ERR_BUF; /* Ramka nie mieści się w buforze DMA. */
  }
  if (DMATxDescToSet->Status & ETH_DMATxDesc_OWN) {
    ((struct ethnetif *)netif->state)->TX_errors++;
    return ERR_IF; /* Błąd, deskryptor jest zajęty. */
  }

  /* TODO: implementacja zero-copy */
  pbuf_copy_partial(p, (void *)DMATxDescToSet->Buffer1Addr,
                    p->tot_len, 0);

  /* Długość ramki znajduje się w bitach [12:0]. */
  DMATxDescToSet->ControlBufferSize =
    p->tot_len & ETH_DMATxDesc_TBS1;

  /* Cała ramka mieści się w jednym buforze, więc jest to
     pierwszy (bit FS) i ostatni segment (bit LS).
     Zwróć deskryptor do DMA - ustaw bit OWN. */
  DMATxDescToSet->Status |= ETH_DMATxDesc_FS |
                            ETH_DMATxDesc_LS |
                            ETH_DMATxDesc_OWN;

  /* Wznów przeglądanie (ang. polling) deskryptorów nadawczych
     i nadawanie ramek przez DMA. */
  if (ETH->DMASR & ETH_DMASR_TBUS) {
    ETH->DMASR = ETH_DMASR_TBUS;
    ETH->DMATPDR = 0;
  }

  /* Zmień deskryptor nadawczy DMA na następny w łańcuchu. */
  DMATxDescToSet =
    (ETH_DMADESCTypeDef *)DMATxDescToSet->Buffer2NextDescAddr;

  return ERR_OK;
}

/** ODBIERANIE RAMEK ETHERNETOWYCH **/

/* Zmienna zdefiniowana w pliku stm32_eth.c, wskazuje na bieżący
   deskryptor odbiorczy DMA. */
extern ETH_DMADESCTypeDef *DMARxDescToGet;

void ETH_IRQHandler(void) {
  /* Przerwanie trzeba wyzerować przed odczytaniem ramki. */
  ETH_DMAClearITPendingBit(ETH_DMA_IT_NIS | ETH_DMA_IT_R);
  while (!(DMARxDescToGet->Status & ETH_DMARxDesc_OWN))
    ethernetif_input(p_netif);
}

/**
 * This function should be called when a packet is ready to be read
 * from the interface.  It uses the function low_level_input() that
 * should handle the actual reception of bytes from the network
 * interface.  Then the type of the received packet is determined and
 * the appropriate input function is called.
 *
 * @param netif the lwip network interface structure for this
 *              ethernetif
 */
err_t ethernetif_input(struct netif *netif) {
  struct pbuf *p;
  err_t err;

  ((struct ethnetif *)netif->state)->RX_packets++;

  /* Alokuj nowy bufor i wczytaj do niego odebraną ramkę. */
  p = low_level_input(netif);

  /* Jeśli ramka nie może być odczytana, ignoruj to. */
  if (p == NULL) {
    ((struct ethnetif *)netif->state)->RX_errors++;
    return ERR_MEM;
  }

  /* Pzekaż odebraną ramkę do stosu lwIP. */
  err = netif->input(p, netif);
  if (err != ERR_OK) {
    ((struct ethnetif *)netif->state)->RX_errors++;
    pbuf_free(p);
  }

  /* Wszelkie błędy zwracane przez tę funkcję są ignorowane. Nie ma
     realnej możliwości ich obsłużenia w ETH_IRQHandler(). */
  return err;
}

/**
 * Should allocate a pbuf and transfer the bytes of the incoming
 * packet from the interface into the pbuf.
 *
 * @param netif the lwip network interface structure for this
 *              ethernetif
 * @return      a pbuf filled with the received packet (including MAC
 *              header) or NULL on memory error
 */
struct pbuf * low_level_input(struct netif *netif) {
  struct pbuf *p = NULL;
  uint32_t len;

  /* ETH_DMARxDesc_ES == 0 oznacza, że nie wystąpił błąd.
     ETH_DMARxDesc_LS == 1 && ETH_DMARxDesc_FS == 1 oznacza, że
     cała ramka jest w pojedynczym buforze. */
  if ((DMARxDescToGet->Status & ETH_DMARxDesc_ES) == 0 &&
      (DMARxDescToGet->Status & ETH_DMARxDesc_LS) != 0 &&
      (DMARxDescToGet->Status & ETH_DMARxDesc_FS) != 0) {
    len = ETH_GetDMARxDescFrameLength(DMARxDescToGet);
    if (len >= ETH_HEADER + MIN_ETH_PAYLOAD + ETH_CRC) {
      len -= ETH_CRC;
      /* TODO: implementacja zero-copy */
      p = pbuf_alloc(PBUF_RAW, len, PBUF_POOL);
      /* Funkcja pbuf_take jest odporna na p == NULL. */
      pbuf_take(p, (void *)DMARxDescToGet->Buffer1Addr, len);
    }
  }

  /* Zwróć deskryptor do DMA - ustaw bit OWN w deskryptorze
     odbiorczym i wyzeruj status. */
  DMARxDescToGet->Status = ETH_DMARxDesc_OWN;

  /* Zmień deskryptor odbiorczy DMA na następny w łańcuchu. */
  DMARxDescToGet =
    (ETH_DMADESCTypeDef *)DMARxDescToGet->Buffer2NextDescAddr;

  /* Jeśli odbieranie DMA zostało wstrzymane, wznów je. */
  if (ETH->DMASR & ETH_DMASR_RBUS) {
    ETH->DMASR = ETH_DMASR_RBUS;
    ETH->DMARPDR = 0;
  }

  return p;
}
