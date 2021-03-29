/*
 * LL_UART_ECHO_DMA_IDLE.c
 *
 *  Created on: Mar 15, 2021
 *      Author: Teodor
 */

#include "LL_UART_ECHO_DMA_IDLE.h"

	UART_DMA_Handle_Td TUART2;

void Init_LL_USART_IDLE()
{
/*	uint8_t Wiad_pocz[]="Wprowadz swoja wiadomosc do UARTU, program ten dziala jako echo \n\r";

			for(int i=0; i<sizeof(Wiad_pocz)-1; i++)
		{
				while(!(LL_USART_IsActiveFlag_TXE(USART2)))
				{
				//here should be timeout;
				}
			LL_USART_TransmitData8(USART2,*(Wiad_pocz+i) );  //Nadawanie wiadomosci w trybie blokujacym
		}*/

			/*Konfiguracja struktury UARTU dla ktorego ma dzialac program ECHO z DMA
			 * Dla F1 na DMA1, mozna uruchomic wszystkie 3 UARTY wiec w strukturze nie ma modyfikacji DMA
			 * mozna to dodac w zaleznosci od potrzeby jest to bardzo latwe
			 */
	uint8_t rx_BUF[1655];
	uint8_t tx_BUF[1655]="EXAMPLES\n\r";

	TUART2.Instance=USART2;
	TUART2.UART_DMA_RX_CHANNEL=LL_DMA_CHANNEL_6;
	TUART2.UART_DMA_TX_CHANNEL=LL_DMA_CHANNEL_7;

	TUART2.NbofRecData=1655;
	TUART2.UART_DMA_RX_Buffer=rx_BUF;
	TUART2.UART_DMA_TX_Buffer= tx_BUF;
	TUART2.ubNbDataToTransmit=0;
	TUART2.ubReceptionComplete=1;  //This must be 1 initialized
	TUART2.ubTransmissionComplete=1; //This must be 1 initialized
	TUART2.NbDataToReceive=1655;

	  Configure_DMA(&TUART2);

	  StartTransfers(&TUART2);

	  TUART_DMA_Trasmit(&TUART2,(uint8_t*) "Wprowadz swoja wiadomosc do UARTU, program ten dziala jako echo  115200kb/s \n\r" );
}

void TUART_DMA_Trasmit(UART_DMA_Handle_Td *USARTX, uint8_t *txBuf)
{
	//But in this option i can't send the '\0' Mark
	uint16_t Lsize=0;
	for(uint16_t i=0; i<2000; i++)
	{
		if(txBuf[i]=='\0')
		{
			break;
		}
		Lsize++;;
	}

	LL_DMA_DisableChannel(DMA1, USARTX->UART_DMA_TX_CHANNEL);

  /* It should be */
	  LL_DMA_ConfigAddresses(DMA1, USARTX->UART_DMA_TX_CHANNEL,
	                        (uint32_t) txBuf,
	                         LL_USART_DMA_GetRegAddr(USARTX->Instance),
	                         LL_DMA_GetDataTransferDirection(DMA1,USARTX->UART_DMA_TX_CHANNEL));

	LL_DMA_SetDataLength(DMA1, USARTX->UART_DMA_TX_CHANNEL, Lsize);

	LL_DMA_EnableChannel(DMA1, USARTX->UART_DMA_TX_CHANNEL);
}

void TUART_END_RECEIVE_CALLBACK(UART_DMA_Handle_Td *USARTX)
{
 	LL_UART_ECHO_DMA_IDLE(&TUART2);   //Funkcja odsylajace przyslane dane
}
void LL_UART_ECHO_DMA_IDLE(UART_DMA_Handle_Td *USARTX)
{
	LL_DMA_DisableChannel(DMA1, USARTX->UART_DMA_TX_CHANNEL);

  /* It should be */
	  LL_DMA_ConfigAddresses(DMA1, USARTX->UART_DMA_TX_CHANNEL,
	                        (uint32_t) USARTX->UART_DMA_RX_Buffer,
	                         LL_USART_DMA_GetRegAddr(USARTX->Instance),
	                         LL_DMA_GetDataTransferDirection(DMA1,USARTX->UART_DMA_TX_CHANNEL));

	LL_DMA_SetDataLength(DMA1, USARTX->UART_DMA_TX_CHANNEL, USARTX->NbofRecData);

	LL_DMA_EnableChannel(DMA1, USARTX->UART_DMA_TX_CHANNEL);
}
void StartTransfers(UART_DMA_Handle_Td *USARTX)
{
	LL_USART_EnableDMAReq_RX(USARTX->Instance);
	LL_USART_EnableDMAReq_TX(USARTX->Instance);
	LL_DMA_EnableChannel(DMA1, USARTX->UART_DMA_RX_CHANNEL);
	LL_DMA_EnableChannel(DMA1, USARTX->UART_DMA_TX_CHANNEL);
	LL_USART_EnableIT_IDLE(USARTX->Instance);
}
void USART_IDLE_CallBack(UART_DMA_Handle_Td *USARTX)
{
	if(LL_USART_IsActiveFlag_IDLE(USARTX->Instance) )
	{
		LL_USART_ClearFlag_IDLE(USARTX->Instance);
		UART_DMA_ReceiveComplete_Callback(USARTX);
		//Hardware info end of transmit
		TUART_END_RECEIVE_CALLBACK(USARTX);
	}
}
void UART_DMA_TransmitComplete_Callback(UART_DMA_Handle_Td *USARTX)
{
 USARTX->ubTransmissionComplete=1;
}
void UART_DMA_ReceiveComplete_Callback(UART_DMA_Handle_Td *USARTX)
{
	LL_DMA_DisableChannel(DMA1, USARTX->UART_DMA_RX_CHANNEL);

 	 USARTX->ubReceptionComplete = 1;
 	USARTX->NbofRecData=USARTX->NbDataToReceive - LL_DMA_GetDataLength(DMA1, USARTX->UART_DMA_RX_CHANNEL);
 	LL_DMA_SetDataLength(DMA1, USARTX->UART_DMA_RX_CHANNEL, USARTX->NbDataToReceive);
	LL_DMA_EnableChannel(DMA1, USARTX->UART_DMA_RX_CHANNEL);
}
void Configure_DMA(UART_DMA_Handle_Td *USARTX)
{

  LL_DMA_ConfigTransfer(DMA1, USARTX->UART_DMA_TX_CHANNEL,
                        LL_DMA_DIRECTION_MEMORY_TO_PERIPH |
                        LL_DMA_PRIORITY_HIGH              |
                        LL_DMA_MODE_NORMAL                |
                        LL_DMA_PERIPH_NOINCREMENT         |
                        LL_DMA_MEMORY_INCREMENT           |
                        LL_DMA_PDATAALIGN_BYTE            |
                        LL_DMA_MDATAALIGN_BYTE);

  LL_DMA_ConfigAddresses(DMA1, USARTX->UART_DMA_TX_CHANNEL,
                         (uint32_t)USARTX->UART_DMA_TX_Buffer,
                         LL_USART_DMA_GetRegAddr(USARTX->Instance),
                         LL_DMA_GetDataTransferDirection(DMA1, USARTX->UART_DMA_TX_CHANNEL));
  LL_DMA_SetDataLength(DMA1, USARTX->UART_DMA_TX_CHANNEL, USARTX->ubNbDataToTransmit);


  LL_DMA_ConfigTransfer(DMA1, USARTX->UART_DMA_RX_CHANNEL,
                        LL_DMA_DIRECTION_PERIPH_TO_MEMORY |
                        LL_DMA_PRIORITY_HIGH              |
                        LL_DMA_MODE_NORMAL                |
                        LL_DMA_PERIPH_NOINCREMENT         |
                        LL_DMA_MEMORY_INCREMENT           |
                        LL_DMA_PDATAALIGN_BYTE            |
                        LL_DMA_MDATAALIGN_BYTE);
  LL_DMA_ConfigAddresses(DMA1, USARTX->UART_DMA_RX_CHANNEL,
                         LL_USART_DMA_GetRegAddr(USARTX->Instance),
						 (uint32_t) USARTX->UART_DMA_RX_Buffer,
                         LL_DMA_GetDataTransferDirection(DMA1, USARTX->UART_DMA_RX_CHANNEL));
  LL_DMA_SetDataLength(DMA1, USARTX->UART_DMA_RX_CHANNEL, USARTX->NbDataToReceive);

  /* (5) Enable DMA transfer complete*/
  LL_DMA_EnableIT_TC(DMA1, USARTX->UART_DMA_TX_CHANNEL);
  LL_DMA_EnableIT_TC(DMA1, USARTX->UART_DMA_RX_CHANNEL);
}
