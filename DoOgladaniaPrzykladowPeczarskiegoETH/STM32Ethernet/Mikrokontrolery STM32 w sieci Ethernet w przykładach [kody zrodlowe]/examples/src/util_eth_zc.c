#include <misc.h>
#include <stm32_eth.h>
#include <util_eth.h>
#include <netif/etharp.h>

/* Liczba buforów odbiorczych i nadawczych DMA,
   rozmiar buforów nadawczych */
#define ETH_RXBUFNB 4
#define ETH_TXBUFNB 4

/* Pierścienie deskryptorów DMA */
static ETH_DMADESCTypeDef DMArxRing[ETH_RXBUFNB] ALIGN4;
static ETH_DMADESCTypeDef DMAtxRing[ETH_TXBUFNB] ALIGN4;

/* Indeksy w pierścieniach deskryptorów DMA */
static int DMArxIdx;
static int DMAtxIdx;

/* Wskaźniki na bufory odbiorcze i bufory nadawcze */
static struct pbuf *rxBuffer[ETH_RXBUFNB];
static struct pbuf *txBuffer[ETH_TXBUFNB];

/* Informacja, czy bufor odbiorczy może być zwrócony do DMA. */
#define RX_BUF_USED_UP 1
static uint8_t rxBufferFlags[ETH_RXBUFNB];

/* Obsługa puli buforów odbiorczych i nadawczych */
static int AllocRxBuffers(void);
static void ReallocRxBuffers(void);
static void DMArxBufferSet(ETH_DMADESCTypeDef *, struct pbuf *);
static void InitTxBuffers(void);
static void FreeTxBuffers(struct netif *netif);
static void DMAtxBufferSet(ETH_DMADESCTypeDef *, struct pbuf *);

/* Trzeba jakoś przekazać ten wskaźnik do ETH_IRQHandler(). */
static struct netif *p_netif;

/* Implementacja funkcjonalności low_level_init */
static int  ETHconfigurePHY(uint8_t);
static void ETHconfigureMAC(struct netif *);
static void ETHconfigureDMA(struct netif *, uint8_t, uint8_t);

/* Funkcje ze szkieletu sterownika dla lwIP. Funkcja low_level_output
   jest wołana przez lwIP w celu wysłania ramki ethernetowej. Funkcja
   ethernetif_input jest wołana po otrzymaniu ramki. Funkcja ta
   najpierw woła low_level_input, aby odebrać ramkę, a następnie woła
   lwIP (funkcja netif->input) w celu przetworzenia tej ramki. */
static err_t low_level_output(struct netif *, struct pbuf *);
static void ethernetif_input(struct netif *);
static struct pbuf * low_level_input(struct netif *);

/** OBSŁUGA BUFORÓW ODBIORCZYCH **/

int AllocRxBuffers() {
  int i, j;

  for (i = 0; i < ETH_RXBUFNB; ++i) {
    rxBufferFlags[i] = 0;
    /* Bufory typu PBUF_POOL i rozmiaru do PBUF_POOL_BUFSIZE są
       alokowane w jednym kawałku. */
    rxBuffer[i] = pbuf_alloc(PBUF_RAW, PBUF_POOL_BUFSIZE, PBUF_POOL);
    if (rxBuffer[i] == NULL) {
      for (j = i - 1; j >= 0; --j)
        pbuf_free(rxBuffer[i]);
      return -1;
    }
  }
  return 0;
}

void ReallocRxBuffers() {
  static int idx = 0;

  /* Przeglądaj bufory cyklicznie, począwszy od następnego po
     ostatnio zwróconym do DMA. */
  while (rxBufferFlags[idx] & RX_BUF_USED_UP) {
    if (rxBuffer[idx] == NULL) {
      /* Uzupełnij bufory odbiorcze. Bufory typu PBUF_POOL i rozmiaru
         PBUF_POOL_BUFSIZE są alokowane w jednym kawałku. */
      rxBuffer[idx] = pbuf_alloc(PBUF_RAW, PBUF_POOL_BUFSIZE,
                                 PBUF_POOL);
      if (rxBuffer[idx] == NULL)
        return;
    }
    rxBufferFlags[idx] &= ~RX_BUF_USED_UP;
    DMArxBufferSet(&DMArxRing[idx], rxBuffer[idx]);
    if (DMArxRing[idx].ControlBufferSize & ETH_DMARxDesc_RER)
      idx = 0;
    else
      ++idx;
  }
}

void DMArxBufferSet(ETH_DMADESCTypeDef *rxDesc, struct pbuf *p) {
  /* Zakładamy, że bufor jest w jednym kawałku. Zwróć deskryptor
     i bufor do DMA - ustaw bit OWN i wyzeruj status. */
  rxDesc->ControlBufferSize &= ~(ETH_DMARxDesc_RBS1 |
                                 ETH_DMARxDesc_RBS2);
  rxDesc->ControlBufferSize |= p->len & ETH_DMARxDesc_RBS1;
  rxDesc->Buffer1Addr = (uint32_t)p->payload;
  rxDesc->Buffer2NextDescAddr = 0;
  rxDesc->Status = ETH_DMARxDesc_OWN;
}

/** OBSŁUGA BUFORÓW NADAWCZYCH **/

void InitTxBuffers() {
  int i;

  for (i = 0; i < ETH_TXBUFNB; ++i)
    txBuffer[i] = NULL;
}

void FreeTxBuffers(struct netif *netif) {
  int i;

  /* Buforów jest niewiele, można je przeglądać naiwnie. */
  for (i = 0; i < ETH_TXBUFNB; ++i) {
    if (txBuffer[i] &&
      (DMAtxRing[i].Status & ETH_DMATxDesc_OWN) == 0) {
      if (DMAtxRing[i].Status & ETH_DMATxDesc_ES)
        ((struct ethnetif *)netif->state)->TX_errors++;
      pbuf_free(txBuffer[i]);
      txBuffer[i] = NULL;
    }
  }
}

void DMAtxBufferSet(ETH_DMADESCTypeDef *txDesc, struct pbuf *p) {
  /* Zakładamy, że łańcuch buforów ma długość co najwyżej dwa i dane
     do wysłania można opisać w jednym deskryptorze, więc jest to
     pierwszy (bit FS) i ostatni segment (bit LS). Poinformuj
     przerwaniem o zakończeniu transmisji - ustaw bit IC. Zwróć bufor
     do DMA - ustaw bit OWN. */
  txDesc->ControlBufferSize = (uint32_t)p->len & ETH_DMATxDesc_TBS1;
  txDesc->Buffer1Addr = (uint32_t)p->payload;
  if (p->tot_len > p->len) {
    p = p->next;
    txDesc->ControlBufferSize |= ((uint32_t)p->len << 16) &
                                 ETH_DMATxDesc_TBS2;
    txDesc->Buffer2NextDescAddr = (uint32_t)(p->payload);
  }
  else
    txDesc->Buffer2NextDescAddr = 0;
  txDesc->Status |= ETH_DMATxDesc_IC | ETH_DMATxDesc_FS |
                    ETH_DMATxDesc_LS | ETH_DMATxDesc_OWN;
}

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
  NVIC_InitTypeDef NVIC_InitStruct;
  int i;

  /* Konfiguruj pierścień deskryptorów odbiorczych. */
  DMArxIdx = 0;
  for (i = 0; i < ETH_RXBUFNB; ++i) {
    /* Zeruj bit ETH_DMARxDesc_DIC, aby uaktywnić przerwanie.
       DMArxRing[i].ControlBufferSize &= ~ETH_DMARxDesc_DIC; */
    DMArxRing[i].ControlBufferSize = 0;
    DMArxBufferSet(&DMArxRing[i], rxBuffer[i]);
  }
  DMArxRing[ETH_RXBUFNB - 1].ControlBufferSize |= ETH_DMARxDesc_RER;
  ETH->DMARDLAR = (uint32_t)DMArxRing;

  /* Konfiguruj pierścień deskryptorów nadawczych. */
  DMAtxIdx = 0;
  for (i = 0; i < ETH_TXBUFNB; ++i) {
    #if CHECKSUM_GEN_IP == 0
      DMAtxRing[i].Status = ETH_DMATxDesc_CIC_IPV4Header;
    #else
      DMAtxRing[i].Status = 0;
    #endif
    DMAtxRing[i].ControlBufferSize = 0;
    DMAtxRing[i].Buffer1Addr = 0;
    DMAtxRing[i].Buffer2NextDescAddr = 0;
  }
  DMAtxRing[ETH_TXBUFNB - 1].Status |= ETH_DMATxDesc_TER;
  ETH->DMATDLAR = (uint32_t)DMAtxRing;

  /* Trzeba jakoś przekazać ten wskaźnik do ETH_IRQHandler(). */
  p_netif = netif;

  NVIC_InitStruct.NVIC_IRQChannel = ETH_IRQn;
  NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = priority;
  NVIC_InitStruct.NVIC_IRQChannelSubPriority = subpriority;
  NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStruct);

  /* Zgłaszaj przerwanie po odebraniu ramki i po zakończeniu
     wysyłania ramki. */
  ETH_DMAITConfig(ETH_DMA_IT_NIS | ETH_DMA_IT_R | ETH_DMA_IT_T,
                  ENABLE);
}

err_t ETHinit(struct netif *netif) {
  struct ethnetif *ethnetif = netif->state;

  if (ETHconfigurePHY(ethnetif->phyAddress) < 0)
    return ERR_IF;
  ETHconfigureMAC(netif);

  /* Przed konfiguracją DMA trzeba zaalokować bufory odbiorcze
     i zainicjować struktury obsługujące bufory nadawcze. */
  if (AllocRxBuffers() < 0)
    return ERR_MEM;
  InitTxBuffers();
  ETHconfigureDMA(netif, LWIP_IRQ_PRIO, 0);

  /* Tu jest miejsce dla różnych dodatkowych ustawień,
     np. nazwy interfejsu. */
  netif->name[0] = 's';
  netif->name[1] = 't';

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

/** PRZERWANIE ETHERNETU **/

void ETH_IRQHandler(void) {
  ETH_DMAClearITPendingBit(ETH_DMA_IT_NIS);
  if (ETH_GetDMAITStatus(ETH_DMA_IT_T)) {
    ETH_DMAClearITPendingBit(ETH_DMA_IT_T);
    FreeTxBuffers(p_netif);
  }
  if (ETH_GetDMAITStatus(ETH_DMA_IT_R)) {
    ETH_DMAClearITPendingBit(ETH_DMA_IT_R);
    while (!(DMArxRing[DMArxIdx].Status & ETH_DMARxDesc_OWN)) {
      ethernetif_input(p_netif);
      ReallocRxBuffers();
    }
  }
}

/** WYSYŁANIE RAMEK ETHERNETOWYCH **/

static int pbuf_chain_len(struct pbuf *p) {
  int len;

  len = p != NULL;
  while (p != NULL && p->tot_len != p->len) {
    ++len;
    p = p->next;
  }
  return len;
}

err_t low_level_output(struct netif *netif, struct pbuf *p) {
  ((struct ethnetif *)netif->state)->TX_packets++;

  if (p == NULL) {
    ((struct ethnetif *)netif->state)->TX_errors++;
    return ERR_ARG;
  }
  if (pbuf_chain_len(p) > 2 || /* Czy wystarczy jeden deskryptor? */
      txBuffer[DMAtxIdx] ||
      DMAtxRing[DMAtxIdx].Status & ETH_DMATxDesc_OWN) {
    ((struct ethnetif *)netif->state)->TX_errors++;
    return ERR_IF;
  }

  /* To będzie działać pod warunkiem, że nie wysyłamy danych
     z pamięci FLASH. */
  pbuf_ref(p);
  txBuffer[DMAtxIdx] = p;
  DMAtxBufferSet(&DMAtxRing[DMAtxIdx], p);

  /* Wznów przeglądanie (ang. polling) deskryptorów nadawczych
     i nadawanie ramek przez DMA. */
  if (ETH->DMASR & ETH_DMASR_TBUS) {
    ETH->DMASR = ETH_DMASR_TBUS;
    ETH->DMATPDR = 0;
  }

  /* Zmień deskryptor nadawczy DMA na następny w pierścieniu. */
  if (DMAtxRing[DMAtxIdx].Status & ETH_DMATxDesc_TER)
    DMAtxIdx = 0;
  else
    ++DMAtxIdx;

  return ERR_OK;
}

/** ODBIERANIE RAMEK ETHERNETOWYCH **/

void ethernetif_input(struct netif *netif) {
  struct pbuf *p;

  ((struct ethnetif *)netif->state)->RX_packets++;

  /* Uzyskaj bufor z odebraną ramką. Ignoruj błędy, gdyż nie ma
     realnej możliwości ich obsłużenia w ETH_IRQHandler(). */
  p = low_level_input(netif);

  if (p == NULL)
    ((struct ethnetif *)netif->state)->RX_errors++;
  /* Przekaż odebraną ramkę do stosu lwIP. Stos nie zwalnia bufora,
     jeśli wystąpił błąd. */
  else if (netif->input(p, netif) != ERR_OK) {
    pbuf_free(p);
    ((struct ethnetif *)netif->state)->RX_errors++;
  }
}

struct pbuf * low_level_input(struct netif *netif) {
  struct pbuf *p = NULL;
  uint32_t len;

  /* ETH_DMARxDesc_ES == 0 oznacza, że nie wystąpił błąd.
     ETH_DMARxDesc_LS == 1 && ETH_DMARxDesc_FS == 1 oznacza, że
     cała ramka jest opisana pojedynczym deskryptorem. */
  if ((DMArxRing[DMArxIdx].Status & ETH_DMARxDesc_ES) == 0 &&
      (DMArxRing[DMArxIdx].Status & ETH_DMARxDesc_LS) != 0 &&
      (DMArxRing[DMArxIdx].Status & ETH_DMARxDesc_FS) != 0) {
    len = ETH_GetDMARxDescFrameLength(&DMArxRing[DMArxIdx]);
    if (len >= ETH_HEADER + MIN_ETH_PAYLOAD + ETH_CRC) {
      len -= ETH_CRC;
      /* Zakładamy, że rxBuffer[DMArxIdx] != NULL. */
      p = rxBuffer[DMArxIdx];
      /* Dla buforów typu PBUF_POOL nie jest wykonywane kopiowane. */
      pbuf_realloc(p, len);
      /* Usuń bufor z puli. Bufor zostanie przekazany do lwIP. */
      rxBuffer[DMArxIdx] = NULL;
    }
  }
  rxBufferFlags[DMArxIdx] |= RX_BUF_USED_UP;

  /* Zmień deskryptor odbiorczy DMA na następny w pierścieniu. */
  if (DMArxRing[DMArxIdx].ControlBufferSize & ETH_DMARxDesc_RER)
    DMArxIdx = 0;
  else
    ++DMArxIdx;

  /* Jeśli odbieranie DMA zostało wstrzymane, wznów je. */
  if (ETH->DMASR & ETH_DMASR_RBUS) {
    ETH->DMASR = ETH_DMASR_RBUS;
    ETH->DMARPDR = 0;
  }

  return p;
}
