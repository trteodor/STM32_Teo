/*
 * OLED_SSD1306_Tasks.j
 *
 *This file is an example of using a prepared GFX with a display based on the SSD1306 driver
 *
 *  Created on: Apr 22, 2021
 *      Author: Teodor
 *      trteodor@gmail.com
 */

#ifndef INC_OLED_SSD1306_TASK_H_
#define INC_OLED_SSD1306_TASK_H_

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "adc.h"
#include "dma.h"
#include "i2c.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

#include "font_8x5.h"
#include "OLED_SSD1306.h"
#include "GFX_BW.h"
#include "picture.h"

#include "BMPXX80.h"
#include "TSOP2236_new_T.h"
#include "hcsr04.h"
#include"mpu6050.h"
#include "rc522.h"

#include "arm_math.h"

extern void OLED_Init();
extern void OLED_Task();
extern void OLED_EXTI_CallBack(uint16_t GPIO_Pin);
extern void ADC_MicrophoneConvCpltCallBack();

#endif /* INC_OLED_SSD1306_TASK_H_ */
