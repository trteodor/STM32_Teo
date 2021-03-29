#include "main.h"

#pragma   arm section code = "RAMCODE"
void TIM1_Cnf()
{
		 RCC->APB2ENR |= RCC_APB2ENR_TIM1EN;
		TIM1->PSC = 6399;
		TIM1->ARR = 5000;
		TIM1->DIER = TIM_DIER_UIE;
		TIM1->CR1 = TIM_CR1_CEN;	
		NVIC_EnableIRQ(TIM1_UP_IRQn);
}
#pragma arm section


int main(void) 
{
	//Frist config the PLL
	PLL_Config64MHZ();
	//SysTick
	SysTick_Config(64000/8);
	SysTick->CTRL &= ~SysTick_CTRL_CLKSOURCE_Msk; // SysTickClk/8
	//Enable the GPIOA and GPIOB port
		GPIO_Pin_Cfg(GPIOA,Px5,gpio_mode_output_PP_2MHz);		
		USART2Config();
	
	  TIM1_Cnf();
	
	 uint32_t zT_LED=0;
 while(1)
 {
	if(  (SysTime-zT_LED) > 500)
	{
		zT_LED=SysTime;
		//tooglePIN(GPIOA,Psm5);
		 __WFI; //sleep CPU!
	}
 }
}

void TIM1_UP_IRQHandler(void)
	{
 if (TIM1->SR & TIM_SR_UIF)
	 {
				TIM1->SR = ~TIM_SR_UIF;
		tooglePIN(GPIOA,Psm5);
	}
 }
