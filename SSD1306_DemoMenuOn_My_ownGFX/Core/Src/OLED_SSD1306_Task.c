/*
 * OLED_SSD1306_Tasks.c
 *
 *  Created on: Apr 22, 2021
 *      Author: Teodor
 *      trteodor@gmail.com
 */

#include "OLED_SSD1306_Task.h"


//
#define Button1Flag 1
#define Button2Flag 2
#define Button3Flag 3
void OLED_Button_CallBack(uint16_t GPIO_Pin);
int DrawMainMenu(GFX_td *MainMenu, int value);
void DrawActiveElement(GFX_td *Window,int elnum, char* String);
void DrawDontActiveElement(GFX_td *Window,int elnum, char* String);
void DrawHead(GFX_td *Window,int elofnum,int allselnum, char* String);
int DrawLedMenu(GFX_td *MainMenu, int value);
void DrawGFXDemo();
void OLED_PickButton_Task();
void OLED_ShiftButton_Task();
void OLED_ActiveTask();
void DrawBMP280Sensor();

int ButtonFlag=0;
uint32_t ButtonDelay=0;
uint32_t OL_Time=0;
uint32_t ProgramState=1;

//Windows pointer
GFX_td *MainWindow=0;

GFX_td *WindowVerScrH=0;
GFX_td *WindowVerScr=0;
GFX_td *WindowHorScr=0;
GFX_td *WindowHorScrH=0;

GFX_td *WindowVerStr=0;
GFX_td *WindowVerStrH=0;

//end section


float temperature, huminidity;
int32_t pressure;

void OLED_Init()
{

					MainWindow= GFX_CreateScreen();  //Create Main Bufor Frame Pointer


					  BMP280_Init(&hi2c1, BMP280_TEMPERATURE_16BIT, BMP280_STANDARD, BMP280_FORCEDMODE);

					GFX_SetFont(font_8x5);
					GFX_SetFontSize(1);
					SSD1306_I2cInit(&hi2c1);
					SSD1306_Bitmap((uint8_t*)picture);
					HAL_Delay(200);
					DrawMainMenu(MainWindow,ProgramState);
					SSD1306_Display(MainWindow);
}
void OLED_Task()
{



	OLED_PickButton_Task();
	OLED_ShiftButton_Task();
	OLED_ActiveTask();

}


void OLED_ActiveTask()
{
	if( (OL_Time+30) < HAL_GetTick())
			{
				OL_Time=HAL_GetTick();

					if(hi2c1.hdmatx->State == HAL_DMA_STATE_READY) //niestety najlepiej sie odw do struktury
					{
						switch(ProgramState)
								{
								case 110:
								BMP280_ReadTemperatureAndPressure(&temperature, &pressure);
								DrawBMP280Sensor();
								break;
								case 201:
								DrawGFXDemo(MainWindow);
								break;
								}


					}
			}
}
void OLED_PickButton_Task()
{
	uint32_t zProgramState=ProgramState;

	if(ButtonFlag==Button2Flag)		//Pick Button
	{
		switch(ProgramState)
		{

					//1-100 Main Menu
		case 1:														//Demo
			WindowVerScrH      = GFX_CreateWindow(120,32);
			WindowVerScr  = GFX_CreateWindow(120,16);
			WindowHorScrH  = GFX_CreateWindow(80,16);
			WindowHorScr  = GFX_CreateWindow(80,16);
			WindowVerStr = GFX_CreateWindow(80,16);
			WindowVerStrH = GFX_CreateWindow(80,16);

			ProgramState=201;
			ButtonFlag=0;

		break;
		case 2:												//Led Menu
			ProgramState=101;
			ButtonFlag=0;
			DrawLedMenu(MainWindow,ProgramState);
		break;


		case 3:
		break;
		case 4:
				ProgramState=110;
					ButtonFlag=0;
		break;
		case 5:
		break;
		case 6:
		break;
		case 7:
		break;
		case 8:
		break;
		case 9:
		break;
											//Above 100 is the led menu
		case 101:
			HAL_GPIO_WritePin(LD_GR_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);  //zapal diode
			ButtonFlag=0;
		break;
		case 102:
			HAL_GPIO_WritePin(LD_GR_GPIO_Port, LD2_Pin, GPIO_PIN_SET);	//zgas diode
			ButtonFlag=0;
		break;
		case 103:
			ProgramState=1;
			ButtonFlag=0;
			DrawMainMenu(MainWindow,ProgramState);
			break;
			// 20 is the GFX Demo
		case 110:
			ProgramState=1;
			ButtonFlag=0;
			DrawMainMenu(MainWindow,ProgramState);
			break;

		case 201:
			ProgramState=1;
			ButtonFlag=0;
			free(WindowVerScr);
			free(WindowVerScrH);
			free(WindowHorScr);
			free(WindowHorScrH);
			free(WindowVerStr);
			free(WindowVerStrH);
			DrawMainMenu(MainWindow,ProgramState);
			break;
		default:
			ButtonFlag=0;
			break;
		}
		if(ProgramState!=zProgramState)
		{
			SSD1306_Display(MainWindow);
		}
	}
}
void OLED_ShiftButton_Task()
{
	uint32_t zProgramState=ProgramState;

	if(ButtonFlag==Button1Flag)
	{
		switch(ProgramState)
		{
		case 1:
			ProgramState=DrawMainMenu(MainWindow,2);
			ButtonFlag=0;
		break;
		case 2:
				ProgramState=DrawMainMenu(MainWindow,3);
				ButtonFlag=0;
			break;
		case 3:
			ProgramState=DrawMainMenu(MainWindow,4);
			ButtonFlag=0;
		break;
		case 4:
			ProgramState=DrawMainMenu(MainWindow,5);   //back on top
			ButtonFlag=0;
		break;
		case 5:
			ProgramState=DrawMainMenu(MainWindow,6);
			ButtonFlag=0;
					break;
		case 6:
			ProgramState=DrawMainMenu(MainWindow,7);
			ButtonFlag=0;
					break;
		case 7:
			ProgramState=DrawMainMenu(MainWindow,8);
			ButtonFlag=0;
					break;
		case 8:
			ProgramState=DrawMainMenu(MainWindow,9);
			ButtonFlag=0;
					break;
		case 9:
			ProgramState=DrawMainMenu(MainWindow,10);
			ButtonFlag=0;
					break;
		case 10:
			ProgramState=DrawMainMenu(MainWindow,1);
			ButtonFlag=0;
					break;




		case 101:
				ProgramState=DrawLedMenu(MainWindow,102);
				ButtonFlag=0;
		break;
		case 102:
				ProgramState=DrawLedMenu(MainWindow,103);
				ButtonFlag=0;
					break;
		case 103:
			ProgramState=DrawLedMenu(MainWindow,101);  //back on top
				ButtonFlag=0;
					break;
		default:
			ButtonFlag=0;
			break;
		}

		if(ProgramState!=zProgramState)
		{
			SSD1306_Display(MainWindow);
		}
	}
}

void OLED_Button_CallBack(uint16_t GPIO_Pin) //Called in interrupt exti
{
	if(ButtonDelay+200 <HAL_GetTick() )
	{
		ButtonDelay=HAL_GetTick();

		if(GPIO_Pin==BUT1_Pin)
		{
			ButtonFlag=Button1Flag;
		}
		if(GPIO_Pin==BUT2_Pin)
		{
			ButtonFlag=Button2Flag;
		}
		if(GPIO_Pin==BUT3_Pin)
		{
			ButtonFlag=Button3Flag;
		}
	}

}




int DrawLedMenu(GFX_td *MainMenu, int value)
{
	char head[20]="LedMenu";

	char el1[20]="Zapal";
	char el2[20]="Zgas";
	char el3[20]="Powrot";
	switch(value)
	{
	case 101:
		GFX_ClearBuffer(MainMenu,LCDWIDTH, LCDHEIGHT);

		DrawHead(MainMenu, 1,3,head );
		DrawActiveElement		(MainMenu, 	1, el1);
		DrawDontActiveElement	(MainMenu, 2, el2);
		DrawDontActiveElement	(MainMenu, 3, el3);

		break;
	case 102:
		GFX_ClearBuffer(MainMenu,LCDWIDTH, LCDHEIGHT);
		DrawHead(MainMenu, 2,3, head);
		DrawDontActiveElement	(MainMenu, 1, el1);
		DrawActiveElement	 	(MainMenu, 2, el2);
		DrawDontActiveElement	(MainMenu, 3, el3);

		break;
	case 103:
		GFX_ClearBuffer(MainMenu,LCDWIDTH, LCDHEIGHT);
		DrawHead(MainMenu, 3,3,head);
		DrawDontActiveElement	(MainMenu, 1, el1);
		DrawDontActiveElement	(MainMenu, 2, el2);
		DrawActiveElement		(MainMenu, 3, el3);
		break;
	}
	return value;
}
void DrawGFXDemo()
{
	static uint32_t Counter_D=0;
	static uint32_t scrollprocessVer=0;
	static uint32_t scrollprocessHor=0;
	static uint32_t scrollprocessStr=0;
	Counter_D++;


			GFX_SetFontSize(1);
			GFX_ClearBuffer(MainWindow,LCDWIDTH, LCDHEIGHT);

			char Counter_c[20];
			sprintf(Counter_c, "ILOSC: %lu", Counter_D);
			GFX_DrawString(MainWindow,10,10, Counter_c, WHITE, BLACK);
			GFX_DrawString(MainWindow, 10, 20, "POZDRAWIAM", WHITE, BLACK);





			//Scroll Effects
			GFX_DrawString(WindowVerStrH,0,0, "PIONOWO  ", WHITE, BLACK);
			GFX_WindowRotate(WindowVerStrH, 55, 8, WHITE, 90);
	  		if(scrollprocessStr<55)
	  		{
	  			GFX_Window_VerScrollFlow(WindowVerStrH, WindowVerStr , 8, 40, WHITE, 55,scrollprocessStr,1);
	  			scrollprocessStr++;
	  		}
	  		if(scrollprocessStr>54) scrollprocessStr=0;
	  		GFX_PutWindow(WindowVerStr, MainWindow, 110,5);


	  			GFX_DrawString(WindowVerScrH,0,0, "POZDRAWIAM ALLS", WHITE, BLACK);
	  			GFX_DrawString(WindowVerScrH,0,8, "To Dziala!", WHITE, BLACK);
		  		if(scrollprocessVer<16)
		  		{
		  			GFX_Window_VerScrollFlow(WindowVerScrH, WindowVerScr , 120, 16, 1, 16,scrollprocessVer,1);
		  			scrollprocessVer++;
		  		}

		  		if(scrollprocessVer>15) scrollprocessVer=0;
		  		GFX_PutWindow(WindowVerScr, MainWindow, 5, 36);


				GFX_DrawString(WindowHorScrH,0,0, "Teodor Test", WHITE, BLACK);
		  		if(scrollprocessHor<97)
		  		{
		  			GFX_Window_Hor_ScrollRight(WindowHorScrH, WindowHorScr,60, 8,1, 70,scrollprocessHor);
		  			scrollprocessHor++;
		  		}
		  		if(scrollprocessHor>96)scrollprocessHor=0;
		  		GFX_PutWindow(WindowHorScr, MainWindow, 20, 56);
		  	//End Section Scroll Effects

		  		SSD1306_Display(MainWindow);
}

void DrawBMP280Sensor()
{
	char head[20]="BMP280";
	char strhbuf[40];

		GFX_ClearBuffer(MainWindow,LCDWIDTH, LCDHEIGHT);
		DrawHead(MainWindow, 1,1,head );
		GFX_SetFontSize(1);
		GFX_DrawString(MainWindow, 5, 16, "Temperatura[C]:", WHITE, BLACK);
		sprintf(strhbuf,"%f",temperature);
		GFX_DrawString(MainWindow, 5, 24, strhbuf, WHITE, BLACK);
		GFX_DrawString(MainWindow, 5, 32, "Cisnienie[PA]:", WHITE, BLACK);
		sprintf(strhbuf,"%lu",pressure);
		GFX_DrawString(MainWindow, 5, 40, strhbuf, WHITE, BLACK);
		SSD1306_Display(MainWindow);
}

int DrawMainMenu(GFX_td *MainMenu, int value)
{
	char head[20]="MainMenu";

	char el1[20]="DEMOGFX";
	char el2[20]="LedState";
	char el3[20]="MIKR_FFT";
	char el4[20]="TMP_BMP";
	char el5[20]="AKC_6050";
	char el6[20]="FOT_Rez";
	char el7[20]="ODB_LM";
	char el8[20]="ODL_HC04";
	char el9[20]="RFID_522";
	char el10[20]="IR_2248";
#define conutel 10


	switch(value)
							{
							case 1:
										GFX_ClearBuffer(MainMenu,LCDWIDTH, LCDHEIGHT);
										DrawHead(MainMenu, 1,conutel, head);
										DrawActiveElement	(MainMenu, 1, el1);
										DrawDontActiveElement(MainMenu, 2, el2);
										DrawDontActiveElement(MainMenu, 3, el3);


								break;
							case 2:
									GFX_ClearBuffer(MainMenu,LCDWIDTH, LCDHEIGHT);

										DrawHead(MainMenu, 2,conutel, head);
										DrawDontActiveElement	(MainMenu, 1,el1);
										DrawActiveElement(MainMenu, 2, el2);
										DrawDontActiveElement(MainMenu, 3,el3);


								break;
							case 3:
									GFX_ClearBuffer(MainMenu,LCDWIDTH, LCDHEIGHT);

										DrawHead(MainMenu, 3,conutel,head);
										DrawDontActiveElement	(MainMenu, 1,el1);
										DrawDontActiveElement(MainMenu, 2,el2);
										DrawActiveElement(MainMenu,3, el3);


								break;
							case 4:
									GFX_ClearBuffer(MainMenu,LCDWIDTH, LCDHEIGHT);

										DrawHead(MainMenu, 4,conutel, head);
										DrawDontActiveElement	(MainMenu, 1, el2);
										DrawDontActiveElement(MainMenu, 2,el3);
										DrawActiveElement(MainMenu, 3, el4);
								break;
							case 5:
									GFX_ClearBuffer(MainMenu,LCDWIDTH, LCDHEIGHT);

										DrawHead(MainMenu, 5,conutel, head);
										DrawDontActiveElement	(MainMenu, 1, el3);
										DrawDontActiveElement(MainMenu, 2,el4);
										DrawActiveElement(MainMenu, 3, el5);
								break;
							case 6:
									GFX_ClearBuffer(MainMenu,LCDWIDTH, LCDHEIGHT);

										DrawHead(MainMenu, 6,conutel, head);
										DrawDontActiveElement	(MainMenu, 1, el4);
										DrawDontActiveElement(MainMenu, 2,el5);
										DrawActiveElement(MainMenu, 3, el6);
								break;
							case 7:
									GFX_ClearBuffer(MainMenu,LCDWIDTH, LCDHEIGHT);

										DrawHead(MainMenu, 7,conutel, head);
										DrawDontActiveElement	(MainMenu, 1, el5);
										DrawDontActiveElement(MainMenu, 2,el6);
										DrawActiveElement(MainMenu, 3, el7);
								break;
							case 8:
									GFX_ClearBuffer(MainMenu,LCDWIDTH, LCDHEIGHT);

										DrawHead(MainMenu, 8,conutel, head);
										DrawDontActiveElement	(MainMenu, 1, el6);
										DrawDontActiveElement(MainMenu, 2,el7);
										DrawActiveElement(MainMenu, 3, el8);
								break;
							case 9:
									GFX_ClearBuffer(MainMenu,LCDWIDTH, LCDHEIGHT);

										DrawHead(MainMenu, 9,conutel, head);
										DrawDontActiveElement	(MainMenu, 1, el7);
										DrawDontActiveElement(MainMenu, 2,el8);
										DrawActiveElement(MainMenu, 3, el9);
								break;
							case 10:
									GFX_ClearBuffer(MainMenu,LCDWIDTH, LCDHEIGHT);

										DrawHead(MainMenu, 10,conutel, head);
										DrawDontActiveElement	(MainMenu, 1, el8);
										DrawDontActiveElement(MainMenu, 2,el9);
										DrawActiveElement(MainMenu, 3, el10);
								break;
							default:
								break;

							}
	return value;
}

void DrawHead(GFX_td *Window,int elofnum,int allselnum, char* String)
{
	char *StringP=malloc(20);
	for(int i=0; i<20; i++)
	{
		(StringP[i]=String[i]);
	}
	GFX_SetFontSize(1);
	GFX_DrawString(Window,10,0,StringP, WHITE, BLACK);
	sprintf(StringP,"%i of %i", elofnum,allselnum);
	GFX_DrawString(Window,80,0,StringP, WHITE, BLACK);
	GFX_DrawLine(Window, 0, 15, 120, 15, WHITE);

	free(StringP);
}
void DrawActiveElement(GFX_td *Window,int elnum, char* String)
{
	int stringlong=0;
	for(; stringlong<50; stringlong++)
	{
		if(String[stringlong]=='\0')
		{
			break;
		}
	}
	GFX_SetFontSize(2);
	stringlong=stringlong*12;

	GFX_SetFontSize(2);
	GFX_DrawFillRectangle(Window, 19, elnum*16-1, stringlong, 16, WHITE);
	GFX_DrawString(Window,20,elnum*16, String, BLACK, INVERSE);
	GFX_DrawFillCircle(Window, 5, (elnum*16)+8, 5, WHITE);
}
void DrawDontActiveElement(GFX_td *Window,int elnum, char* String)
{
	int stringlong=0;
	for(; stringlong<50; stringlong++)
	{
		if(String[stringlong]=='\0')
		{
			break;
		}
	}
	GFX_SetFontSize(2);
	stringlong=stringlong*12;
	GFX_SetFontSize(2);
	GFX_DrawString(Window,20,elnum*16, String, WHITE, BLACK);
	GFX_DrawCircle(Window, 5, (elnum*16)+8, 5, WHITE);
}






