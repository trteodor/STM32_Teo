#include "stm32f10x.h"
//Make sure about, in the startup file the SystemInit function is disable


//License: hmm BeerWare ;) 

// Copyright (c) 2021 Teodor Rosolowski
// trteodor@gmail.com

/* Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */


void USART2Config()
{
	//Make sure USART GPIO Port is enable
	RCC->APB2ENR |=  RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPBEN;
	
	RCC->APB1ENR |= RCC_APB1ENR_USART2EN;  //Enable USART periph
	
		GPIOA->CRL |= GPIO_CRL_MODE2;   //Config USART2_Pins alternate function
		GPIOA->CRL |= GPIO_CRL_MODE3;
	
		GPIOA->CRL |= GPIO_CRL_CNF2;
		GPIOA->CRL |= GPIO_CRL_CNF3;
																	//Remap isn't need
	//Config USART2
		USART2->SR=0;
		USART2->BRR = 8000000/115200;   //simple calculate but it isn't exact
		USART2->CR1 = USART_CR1_UE | USART_CR1_RXNEIE | USART_CR1_TE | USART_CR1_RE;
	//Enable interrupt
		NVIC_EnableIRQ(USART2_IRQn);
}
void USART2_IRQHandler(void){
	if ( USART2->SR & USART_SR_RXNE)
			{
			USART2->DR = USART2->DR;		
			}
}
int main(void) 
{
	USART2Config();
	
 while(1)
 {
	 __WFI; //sleep CPU;
 }
}
