/*
 * IR_TSOP2236_byTEOR.h
 *
 *  Created on: 6 wrz 2020
 *      Author: Teodor
 *
*/

#ifndef INC_IR_TSOP2236_BYTEOR_H_
#define INC_IR_TSOP2236_BYTEOR_H_

#include "stm32f1xx_hal.h"
#include "stm32f1xx_it.h"

#define PINIR_Pin GPIO_PIN_15    //<< Define your input pin here
#define PINIR_GPIO_Port GPIOB    //<<Define your input port here
#define PINIR_EXTI_IRQn EXTI15_10_IRQn  //Choose your exti line handler


//extern uint32_t t2;
extern uint32_t data_ir;
extern uint32_t zCzas_IR;
extern uint8_t flaga_IR;

extern void delay_100us(uint32_t delay_100us);



#endif /* INC_IR_TSOP2236_BYTEOR_H_ */
