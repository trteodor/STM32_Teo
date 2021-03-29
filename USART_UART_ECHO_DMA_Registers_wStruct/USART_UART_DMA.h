
#ifndef __USART_UART_DMA_H__
#define __USART_UART_DMA_H__
#include "main.h"

typedef struct __UART_DMA_Handle_Td
{
	USART_TypeDef     		  *Instance;
	DMA_Channel_TypeDef 		*UART_DMA_RX_CHANNEL;
	DMA_Channel_TypeDef 		*UART_DMA_TX_CHANNEL;
	uint8_t 								*UART_DMA_TX_Buffer;
	uint8_t 								*UART_DMA_RX_Buffer;	// DMA, UART BUFF
	
	uint8_t ubTransmissionComplete;
	uint8_t ubReceptionComplete;
	uint32_t ubNbDataToTransmit;
	uint32_t NbDataToReceive;
	uint32_t NbofRecData;


}UART_DMA_Handle_Td;;

extern UART_DMA_Handle_Td TUART2;

extern void USART2Config(void);


#endif


