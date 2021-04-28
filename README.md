# STM32_Teo
/*How use it?
*  Its easy, follow this steps:
*  1. Call "RC5_IR_EXTI_GPIO_ReceiveAndDecodeFunction" function on evry falling edge of the Input PIN. For example from External interrupt
*  2. Call The "RC5_100usTimer" function exactly every 100us
*  3. Create New global RC5Struct object or allocate memory for this object in your main function/file
*  4. Call Init Function in main with pointer to object created in step 3
*  5. Read the Received data with function "RC5_ReadNormal" -- RC5_ReadNormal(&RC5Device,&RC5_RecDat)
*/