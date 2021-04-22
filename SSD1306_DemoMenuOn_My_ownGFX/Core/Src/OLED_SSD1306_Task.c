/*
 * OLED_SSD1306_Tasks.c
 *
 *  Created on: Apr 22, 2021
 *      Author: Teodor
 */
#include "OLED_SSD1306_Task.h"
//


void OLED_SSD1306Task()
{
		  //Create Windows  //Ajj troche przekombinowane, jednak lepiej byloby zrobic glob var
	    static GFX_td *MainWindow=0;
	    static GFX_td *WindowS1=0;
	    static GFX_td *WindowVerScr=0;
	    static GFX_td *WindowHorScr=0;

		static int fristtime=1;
		if(fristtime)
		{
			fristtime=0;
			GFX_td *L_MainWindow    = GFX_CreateScreen();
			GFX_td *L_WindowS1      = GFX_CreateWindow(120,32);
			GFX_td *L_WindowVerScr  = GFX_CreateWindow(120,16);
			GFX_td *L_WindowHorScr  = GFX_CreateWindow(60,8);
			MainWindow=L_MainWindow;
			WindowS1=L_WindowS1;
			WindowVerScr=L_WindowVerScr;
			WindowHorScr=L_WindowHorScr;

					GFX_SetFont(font_8x5);
					GFX_SetFontSize(1);
					SSD1306_I2cInit(&hi2c1);
					SSD1306_Bitmap((uint8_t*)picture);
					HAL_Delay(200);
		}

		static uint32_t OL_Time=0;
		static uint16_t zmiana = 0;
		static char zmiana_c[20];
		static uint32_t zT1=0;

		static int scrollprocessVer=0;
//Init end
									//Loop Process
if( (OL_Time+50) < HAL_GetTick())
		{






			OL_Time=HAL_GetTick();
			  zmiana++;
				if(hi2c1.hdmatx->State == HAL_DMA_STATE_READY)
				{
					GFX_ClearBuffer(MainWindow,LCDWIDTH, LCDHEIGHT);


					sprintf(zmiana_c, "ILOSC: %02d", zmiana);
					GFX_DrawString(MainWindow,10,10, zmiana_c, WHITE, BLACK);

					GFX_ClearBuffer(WindowS1,WindowS1->WindowWidth, WindowS1->WindowHeigh);
					GFX_DrawString(WindowS1,0,0, "POZDRAWIAM ALLS", WHITE, BLACK);
					//GFX_DrawString(WindowS1,0,8, "UDALO SIE", WHITE, BLACK);

					GFX_PutWindow(WindowS1, MainWindow, 5, 20);
				  	GFX_PutWindow(WindowVerScr, MainWindow, 20, 36);
				  	GFX_PutWindow(WindowHorScr, MainWindow, 20, 56);


					if( (zT1+30) < HAL_GetTick() )
					{
						zT1=HAL_GetTick();
			//			  	GFX_WindowRotate(WindowS1, 24, 8, 1, 90);
			//			  	GFX_Window_ScrollRight(ImageIn, ImageOut, inrows, incol, color, numRowShift)
						//GFX_Window_Hor_ScrollRight(WindowS1, WindowVerScr, 40, 8, 1, 60);
			//	  			GFX_Window_VerScroll(GFX_td *ImageIn,GFX_td *ImageOut,int inrows, int incol,uint8_t color, int numRowShift)
				  		GFX_ClearBuffer(WindowVerScr,WindowVerScr->WindowWidth, WindowVerScr->WindowHeigh);
				  	//	while(scrollprocessVer<16)
				  	//	{
				  			GFX_Window_VerScrollFlow(WindowS1, WindowVerScr , 120, 16, 1, 16,scrollprocessVer,1);
				  		//	scrollprocessVer++;
				  		//}
				  		GFX_ClearBuffer(WindowHorScr,WindowHorScr->WindowWidth, WindowHorScr->WindowHeigh);
						GFX_Window_Hor_ScrollRight(WindowS1, WindowHorScr,120, 8,1, 90+7);
					}

				  		GFX_DrawCircle(MainWindow, 90, 10, 10, 1);
				  		HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, 1);
				  		SSD1306_Display(MainWindow);
				}
				  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, 0);
		}

//@@@


}

