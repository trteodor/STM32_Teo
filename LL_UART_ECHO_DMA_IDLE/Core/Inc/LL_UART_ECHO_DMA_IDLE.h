/*
 * LL_UART_ECHO_DMA_IDLE.h
 *
 *  Created on: Mar 15, 2021
 *      Author: Teodor
 */

#ifndef INC_LL_UART_ECHO_DMA_IDLE_H_
#define INC_LL_UART_ECHO_DMA_IDLE_H_

#include "main.h"


//#define NbDataToReceive 25
#define UART_DMA_BUFF_SIZE      256


typedef struct __UART_DMA_Handle_Td
{
	USART_TypeDef      *Instance;

	uint32_t UART_DMA_RX_CHANNEL;
	uint32_t UART_DMA_TX_CHANNEL;

	uint8_t ubTransmissionComplete;
	uint8_t ubReceptionComplete;

	uint32_t ubNbDataToTransmit;
	uint32_t NbDataToReceive;
	uint32_t NbofRecData;


	uint8_t *UART_DMA_TX_Buffer;
	uint8_t *UART_DMA_RX_Buffer;	// DMA, UART BUFF

}UART_DMA_Handle_Td;;

extern UART_DMA_Handle_Td TUART2;

extern void TUART_END_RECEIVE_CALLBACK(UART_DMA_Handle_Td *USARTX);
extern void TUART_DMA_Trasmit(UART_DMA_Handle_Td *USARTX, uint8_t *txBuf);

extern void USART_IDLE_CallBack(UART_DMA_Handle_Td *USARTX);
extern void Init_LL_USART_IDLE();

extern void StartTransfers(UART_DMA_Handle_Td *USARTX);
extern void UART_DMA_TransmitComplete_Callback(UART_DMA_Handle_Td *USARTX);
extern void UART_DMA_ReceiveComplete_Callback(UART_DMA_Handle_Td *USARTX);

extern void Configure_DMA(UART_DMA_Handle_Td *USARTX);

extern void LL_UART_ECHO_DMA_IDLE(UART_DMA_Handle_Td *USARTX);

extern void LL_UART_ECHO_DMA_IDLE(UART_DMA_Handle_Td *USARTX);


#endif /* INC_LL_UART_ECHO_DMA_IDLE_H_ */
