#include "USART_UART_DMA.h"

UART_DMA_Handle_Td TUART2;

	uint8_t rx_BUF[1655]="ASDFGHH";
	uint8_t tx_BUF[1655]="EXAMPLES\n\r";
	

void TUART_DMA_Trasmit(UART_DMA_Handle_Td *USARTX, uint8_t *txBuf);

void USART2Config(void)
{
	RCC->APB1ENR |= RCC_APB1ENR_USART2EN;  //Enable USART periph
	//To trzeba byloby oprzec na ifach :/ 
		GPIOA->CRL |= GPIO_CRL_MODE2;   //Config USART2_Pins as alternate function
		GPIOA->CRL |= GPIO_CRL_MODE3;
		GPIOA->CRL &= ~GPIO_CRL_CNF3;
		GPIOA->CRL &= ~GPIO_CRL_CNF2;	
		GPIOA->CRL |= GPIO_CRL_CNF2;
		GPIOA->CRL |= GPIO_CRL_CNF3;
	
	//To by sie dalo zrobic w calosci na jednej strukturze i o tzw
	// offset kanalu ale wymagaloby to troche czasu a 
	//az tak dokladnej biblioteki budowac mi sie nie chce
	
		//USART struct config
	TUART2.Instance=USART2;
	TUART2.UART_DMA_RX_CHANNEL=DMA1_Channel6;
	TUART2.UART_DMA_TX_CHANNEL=DMA1_Channel7;

	TUART2.NbofRecData=0;
	TUART2.UART_DMA_RX_Buffer=rx_BUF;
	TUART2.UART_DMA_TX_Buffer= tx_BUF;
	TUART2.ubNbDataToTransmit=0;
	TUART2.ubReceptionComplete=1;  //This must be 1 initialized
	TUART2.ubTransmissionComplete=1; //This must be 1 initialized
	TUART2.NbDataToReceive=1655;
	
	
	//Config USART2
		USART2->SR=0;
		USART2->BRR = 279;
		USART2->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE | USART_CR1_IDLEIE ;
		USART2->CR3 |= USART_CR3_DMAR | USART_CR3_DMAT;
	
	
//DMA Config
	//DMA Channel 6 Receive
	RCC->AHBENR |= RCC_AHBENR_DMA1EN;
	
		NVIC_EnableIRQ(DMA1_Channel6_IRQn);
	TUART2.UART_DMA_RX_CHANNEL->CPAR = (uint32_t)&USART2->DR;
 	TUART2.UART_DMA_RX_CHANNEL->CMAR = (uint32_t) rx_BUF;
	TUART2.UART_DMA_RX_CHANNEL->CNDTR = TUART2.NbDataToReceive;
	TUART2.UART_DMA_RX_CHANNEL->CCR=0;
	TUART2.UART_DMA_RX_CHANNEL->CCR = DMA_CCR6_EN | DMA_CCR6_MINC | DMA_CCR6_TCIE;


		NVIC_EnableIRQ(DMA1_Channel7_IRQn);
	//DMA USART2 Channel 7 TX
//		TUART2.UART_DMA_TX_CHANNEL->CPAR = (uint32_t)&USART2->DR;
//		TUART2.UART_DMA_TX_CHANNEL->CMAR = (uint32_t) &tx_BUF;
//		TUART2.UART_DMA_TX_CHANNEL->CNDTR = 10;
//		TUART2.UART_DMA_TX_CHANNEL->CCR =  DMA_CCR7_EN | DMA_CCR7_MINC | DMA_CCR7_TCIE | DMA_CCR7_DIR;

		NVIC_EnableIRQ(USART2_IRQn);
		
		//volatile uint32_t delay;
//for(delay = 1000000; delay; delay--){};
	uint8_t str_tab[30]="EXAAdfsAMPLES\n\r";
		TUART_DMA_Trasmit(&TUART2, (uint8_t*) "Wprowadz swoja wiadomosc do UARTU, program ten dziala jako UART echo \n\r" );
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

		TUART2.UART_DMA_TX_CHANNEL->CPAR = (uint32_t)&USART2->DR;
		TUART2.UART_DMA_TX_CHANNEL->CMAR = (uint32_t) txBuf;
		TUART2.UART_DMA_TX_CHANNEL->CNDTR = Lsize;
		TUART2.UART_DMA_TX_CHANNEL->CCR =  DMA_CCR7_EN | DMA_CCR6_MINC | DMA_CCR6_TCIE | DMA_CCR6_DIR;
		//TUTAJ sa odwolania bez do rejestrow!!!
	//A juz mi sie nie chce bawic w te offsety:/ 
}


void USART_Tr(UART_DMA_Handle_Td *USARTX)
{
	USARTX->UART_DMA_TX_CHANNEL->CCR=0;
	USARTX->UART_DMA_TX_CHANNEL->CMAR = (uint32_t) &rx_BUF;
	USARTX->UART_DMA_TX_CHANNEL->CNDTR = USARTX->ubNbDataToTransmit;
	USARTX->UART_DMA_TX_CHANNEL->CCR =  DMA_CCR7_EN | DMA_CCR7_MINC | DMA_CCR7_TCIE | DMA_CCR7_DIR;
		//TUTAJ sa odwolania bez do rejestrow!!!
	//A juz mi sie nie chce bawic w te offsety:/ 
}
void ECHO(UART_DMA_Handle_Td *USARTX)
{
	USARTX->ubNbDataToTransmit= USARTX->NbofRecData;
	USART_Tr(USARTX);
}
void End_Rec(UART_DMA_Handle_Td *USARTX)
{
	USARTX->NbofRecData = USARTX->NbDataToReceive - USARTX->UART_DMA_RX_CHANNEL->CNDTR;
	
	ECHO(&TUART2); //ECHO!
	
	USARTX->UART_DMA_RX_CHANNEL->CCR=0;
	TUART2.UART_DMA_TX_CHANNEL->CPAR = (uint32_t)&USART2->DR;
	USARTX->UART_DMA_RX_CHANNEL->CNDTR = USARTX->NbDataToReceive;	
	USARTX->UART_DMA_RX_CHANNEL->CCR = DMA_CCR6_EN | DMA_CCR6_MINC | DMA_CCR6_TCIE;
		//TUTAJ sa odwolania bez do rejestrow!!!
	//A juz mi sie nie chce bawic w te offsety:/ 
}
void USART_CallBack(UART_DMA_Handle_Td *USARTX)
{
	if ( USART2->SR & USART_SR_IDLE)
		{
		  __IO uint32_t tmpreg;
			tmpreg = USARTX->Instance->SR;
		
			End_Rec(&TUART2);		
			(void) tmpreg; //micro delay
			tmpreg = USARTX->Instance->DR;
			(void) tmpreg;	//micro delay
		}
}
void USART2_TX_CallBack(UART_DMA_Handle_Td *USARTX)
{
			if(DMA1->ISR & DMA_ISR_TCIF7)
	{	
		DMA1->IFCR = DMA_IFCR_CTCIF7 | DMA_IFCR_CHTIF7 | DMA_IFCR_CGIF7;
		//TUTAJ sa odwolania bez do rejestrow!!!
	}
}
void USART2_RX_CallBack(UART_DMA_Handle_Td *USARTX)
{
		if(DMA1->ISR & DMA_ISR_TCIF6)
	{
		DMA1->IFCR = DMA_IFCR_CTCIF6 | DMA_IFCR_CHTIF6 | DMA_IFCR_CGIF6;
		End_Rec(&TUART2);
	}
	//TUTAJ sa odwolania bez do rejestrow!!!
	//A juz mi sie nie chce bawic w te offsety:/ 
}



void DMA1_Channel7_IRQHandler()
{
	USART2_TX_CallBack(&TUART2);

}
void DMA1_Channel6_IRQHandler()
{
	USART2_RX_CallBack(&TUART2);
}
void USART2_IRQHandler(void)
	{
		USART_CallBack(&TUART2);
	}
	
