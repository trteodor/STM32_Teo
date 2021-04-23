/*
 * OLED_SSD1306_Tasks.j
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
#include "i2c.h"

#include "font_8x5.h"
#include "OLED_SSD1306.h"
#include "GFX_BW.h"
#include "picture.h"

#include "BMPXX80.h"

extern void OLED_Init();
extern void OLED_Task();
extern void OLED_Button_CallBack(uint16_t GPIO_Pin);


#endif /* INC_OLED_SSD1306_TASK_H_ */
