/*
 * OLED_SSD1306_Tasks.c
 *
 * This file is an example of using a prepared GFX with a display based on the SSD1306 driver
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
void DrawHCSR04();
void DrawRC5_TSOP4438();
void DrawMPU6050();
void DrawReflective();;
void DrawFotoResistor();
void mMFRC522ReadBlock();
void DrawMFRC522();
void ADC_MicrophoneConvCpltCallBack();
void DrawFFT_Micro();
void CalculateFFT();

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

//bmp280
float temperature, huminidity;
int32_t pressure;
//hcsr04
char buf[30];
uint8_t len;
float DistanceHCS04;
//TSOP2236
RC5Struct TSOP4836;
uint8_t TSOP_RecDat;
uint8_t TSOP_RecAddr;
uint16_t TSOP_NormRecData;
//MPU6050
MPU6050_t MPU6050;

//ADC
#define FFT_SAMPLES 0
struct ADC1Dat
{
uint32_t Microphone;
uint32_t FotoRez;
uint32_t CzujnikOdb;
}ADC1Dat;


typedef struct
{
	uint8_t OutFreqArray[10];
} FftData_t;

float complexABS(float real, float compl) {
	return sqrtf(real*real+compl*compl);
}

FftData_t FftData;
uint16_t AdcMicrophone[FFT_SAMPLES];
float FFTInBuffer[FFT_SAMPLES];
float FFTOutBuffer[FFT_SAMPLES];
arm_rfft_fast_instance_f32 FFTHandler;


arm_rfft_fast_instance_f32 FFTHandler;
FftData_t FftData;
int Freqs[FFT_SAMPLES];
int FreqPoint = 0;
int Offset = 45; // variable noise floor offset








//MFRC522
uint8_t str[MAX_LEN];
uint8_t UID[5];
uint8_t  FACTORY_KEY[] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
uint8_t W[]="POZDRAWIAM";
uint8_t R[16];
uint8_t MicrophoneDataReady=0;


void OLED_Init()
{
					HAL_TIM_Base_Start(&htim3);
					  HAL_ADC_Start_DMA(&hadc1,(uint32_t*)&ADC1Dat, 3); //ADC for FotoRez and reflective sensor

					MainWindow= GFX_CreateScreen();  //Create Main Bufor Frame Pointer

					HAL_TIM_Base_Start_IT(&htim2);
					RC5_INIT(&TSOP4836);

					  MFRC522_Init();

					HCSR04_Init(&htim1);
					  BMP280_Init(&hi2c1, BMP280_TEMPERATURE_16BIT, BMP280_STANDARD, BMP280_FORCEDMODE);

					 MPU6050_Init(&hi2c1);

					GFX_SetFont(font_8x5);
					GFX_SetFontSize(1);
					SSD1306_I2cInit(&hi2c1);
					SSD1306_Bitmap((uint8_t*)picture);
					HAL_Delay(2020);
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
	if( (OL_Time+30) < HAL_GetTick())  //minimum delay time is around 80ms because better wait on end data transfer on i2c line OLED data with DMA
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
								case 120:
									   HCSR04_Read(&DistanceHCS04);
									  if( DistanceHCS04 < 50)
										  {
										  len = sprintf(buf, "DistanceHCS04: %.2f\n\r", DistanceHCS04);
										  //HAL_UART_Transmit(&huart2, (uint8_t*)buf, len, 20);
										  }
									DrawHCSR04();
									break;
								case 130:
									//RC5_ReadAddresAndData(&TSOP4836, &TSOP_RecDat, &TSOP_RecAddr);
									RC5_ReadNormal(&TSOP4836, &TSOP_NormRecData);
									DrawRC5_TSOP4438();
									break;
								case 140:			//Akcelerometer MPU6050
//									MPU6050_Read_All(&hi2c1, &MPU6050);
									MPU6050_Read_Temp(&hi2c1, &MPU6050);
									DrawMPU6050();
									break;
								case 150:			//FotoRes
								DrawFotoResistor();
								break;
								case 160:			//Refl
								DrawReflective();
								break;
								case 170:			//RC522
									mMFRC522ReadBlock();
									DrawMFRC522();
								break;
								case 180:			//Micro
									//CalculateFFT();
									DrawFFT_Micro();
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
					ProgramState=101;
					ButtonFlag=0;
					DrawLedMenu(MainWindow,ProgramState);

		case 3:
			ProgramState=180;
					ButtonFlag=0;

		break;
		case 4: 						//HCSR04 - active task
				ProgramState=110;
					ButtonFlag=0;
		break;
		case 5:  //Akcelerometer //ACtiv
			ProgramState=140;
								ButtonFlag=0;
		break;
		case 6:
			ProgramState=150; 		//FotoRez //ACtiv
									ButtonFlag=0;
		break;
		case 7:
			ProgramState=160;	//Refl Sensor  //ACtiv
									ButtonFlag=0;
		break;
		case 8:
			ProgramState=120;  //HCSR04
					ButtonFlag=0;
		break;
		case 9:
			ProgramState=170;  //MSRC522 -- RFID
					ButtonFlag=0;
		break;
		case 10:
			ProgramState=130;  //TSOP4438 -- active task
					ButtonFlag=0;
		break;
											//Above 100 is the led menu
		case 101:
			HAL_GPIO_WritePin(LD_GR_GPIO_Port, LD_GR_Pin, GPIO_PIN_RESET);  //zapal diode
			ButtonFlag=0;
		break;
		case 102:
			HAL_GPIO_WritePin(LD_GR_GPIO_Port, LD_GR_Pin, GPIO_PIN_SET);	//zgas diode
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
		case 120:  //HCSR04
			ProgramState=1;
			ButtonFlag=0;
			DrawMainMenu(MainWindow,ProgramState);
			break;

		case 130:
			ProgramState=1;
			ButtonFlag=0;
			DrawMainMenu(MainWindow,ProgramState);
			break;
		case 140:
			ProgramState=1;
			ButtonFlag=0;
			DrawMainMenu(MainWindow,ProgramState);
			break;
		case 150:
			ProgramState=1;
			ButtonFlag=0;
			DrawMainMenu(MainWindow,ProgramState);
			break;
		case 160:
			ProgramState=1;
			ButtonFlag=0;
			DrawMainMenu(MainWindow,ProgramState);
			break;
		case 170:
			ProgramState=1;
			ButtonFlag=0;
			DrawMainMenu(MainWindow,ProgramState);
			break;
		case 180:   //Microphone
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
			ProgramState=DrawMainMenu(MainWindow,9); //MFRC522
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

void OLED_EXTI_CallBack(uint16_t GPIO_Pin) //Called in interrupt exti
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
	}

	if(GPIO_Pin==GPIO_PIN_7)
	{
		RC5_IR_EXTI_GPIO_ReceiveAndDecodeFunction(&TSOP4836);
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
		GFX_DrawString(MainWindow, 5, 28, strhbuf, WHITE, BLACK);
		GFX_DrawString(MainWindow, 5, 40, "Cisnienie[PA]:", WHITE, BLACK);
		sprintf(strhbuf,"%lu",pressure);
		GFX_DrawString(MainWindow, 5, 52, strhbuf, WHITE, BLACK);
		SSD1306_Display(MainWindow);
}

void DrawHCSR04()
{
	char head[20]="HCSR_04";
	char strhbuf[40];

		GFX_ClearBuffer(MainWindow,LCDWIDTH, LCDHEIGHT);
		DrawHead(MainWindow, 1,1,head );
		GFX_SetFontSize(1);
		GFX_DrawString(MainWindow, 5, 16, "Zmierzona", WHITE, BLACK);
		GFX_DrawString(MainWindow, 5, 28, "Odleglosc[cm]:", WHITE, BLACK);
		sprintf(strhbuf,"%.2f\n\r", DistanceHCS04);
		GFX_DrawString(MainWindow, 5, 40, strhbuf, WHITE, BLACK);
		SSD1306_Display(MainWindow);
}

void DrawRC5_TSOP4438()
{
	char head[20]="TSOP4438";
	char strhbuf[40];

		GFX_ClearBuffer(MainWindow,LCDWIDTH, LCDHEIGHT);
		DrawHead(MainWindow, 1,1,head );
		GFX_SetFontSize(1);
		GFX_DrawString(MainWindow, 5, 16, "Ost. Odcz. Wart:", WHITE, BLACK);
		//sprintf(strhbuf,"0x%x", TSOP_RecAddr);
		//GFX_DrawString(MainWindow, 5, 28, strhbuf, WHITE, BLACK);
		//GFX_DrawString(MainWindow, 5, 40, "Wartosc:", WHITE, BLACK);
		//sprintf(strhbuf,"0x%x", TSOP_RecDat);
		//GFX_DrawString(MainWindow, 5, 52, strhbuf, WHITE, BLACK);
		GFX_DrawString(MainWindow, 5, 28, "16bit HEX Value", WHITE, BLACK);
		sprintf(strhbuf,"0x %x", TSOP_NormRecData);
		GFX_DrawString(MainWindow, 5, 40, strhbuf, WHITE, BLACK);
		SSD1306_Display(MainWindow);
}

void DrawMPU6050()
{
	char head[20]="MPU6050";
	char strhbuf[40];

		GFX_ClearBuffer(MainWindow,LCDWIDTH, LCDHEIGHT);
		DrawHead(MainWindow, 1,1,head );
		GFX_SetFontSize(1);
		GFX_DrawString(MainWindow, 5, 16, "Tempratura[C]", WHITE, BLACK);
		sprintf(strhbuf,"T:%.2f", MPU6050.Temperature);
		GFX_DrawString(MainWindow, 5, 28, strhbuf, WHITE, BLACK);

		SSD1306_Display(MainWindow);
}
void DrawReflective()
{
	char head[20]="Reflective";
	char strhbuf[40];

		GFX_ClearBuffer(MainWindow,LCDWIDTH, LCDHEIGHT);
		DrawHead(MainWindow, 1,1,head );
		GFX_SetFontSize(1);

		GFX_DrawString(MainWindow, 5, 16, "Wartosc ADC:", WHITE, BLACK);
		sprintf(strhbuf,"%lu ", ADC1Dat.CzujnikOdb);
		GFX_DrawString(MainWindow, 5, 28, strhbuf, WHITE, BLACK);

		SSD1306_Display(MainWindow);
}

void DrawMFRC522()
{
	char head[20]="MFRC522";
	char strhbuf[40];

		GFX_ClearBuffer(MainWindow,LCDWIDTH, LCDHEIGHT);
		DrawHead(MainWindow, 1,1,head );
		GFX_SetFontSize(1);

		GFX_DrawString(MainWindow, 5, 16, "UID:", WHITE, BLACK);
		sprintf(strhbuf,"%02X:%02X:%02X:%02X:%02X",UID[0],UID[1],UID[2],UID[3],UID[4]);
		GFX_DrawString(MainWindow, 5, 28, strhbuf, WHITE, BLACK);

		GFX_DrawString(MainWindow, 5, 40, "Sek0 Block2:", WHITE, BLACK);
		sprintf(strhbuf,"%s", R);
		GFX_DrawString(MainWindow, 5, 52,strhbuf, WHITE, BLACK);

		SSD1306_Display(MainWindow);
}

void mMFRC522ReadBlock()
{
	if((MFRC522_Request(PICC_REQIDL, str)==MFRC522_OK))
	{
		if (MFRC522_Anticoll(str)==MFRC522_OK)
		{
			memcpy(UID, str, 5);
			MFRC522_SelectTag(str);
			MFRC522_Auth(PICC_AUTHENT1A,2,FACTORY_KEY,UID);
		//	MFRC522_WriteBlock((uint8_t)2 , W);
			MFRC522_ReadBlock( 2, R);
			MFRC522_DeAuth();
		}
			HAL_Delay(1);
	}
}


void CalculateFFT()
{
	if(MicrophoneDataReady)
	{
		arm_rfft_fast_init_f32(&FFTHandler, FFT_SAMPLES);


		MicrophoneDataReady=0;

		 for(uint32_t i = 0; i < FFT_SAMPLES; i++)
			  {
				  FFTInBuffer[i] =  (float)AdcMicrophone[i];
			  }

			  arm_rfft_fast_f32(&FFTHandler, FFTInBuffer, FFTOutBuffer, 0);

				FreqPoint = 0;
				// calculate abs values and linear-to-dB
				for (int i = 0; i < FFT_SAMPLES; i = i+2)
				{
					Freqs[FreqPoint] = (int)(20*log10f(complexABS(FFTOutBuffer[i], FFTOutBuffer[i+1]))) - Offset;

					if(Freqs[FreqPoint] < 0)
					{
						Freqs[FreqPoint] = 0;
					}
					FreqPoint++;
				}

				FftData.OutFreqArray[0] = (uint8_t)Freqs[1]; // 22 Hz
				FftData.OutFreqArray[1] = (uint8_t)Freqs[2]; // 63 Hz
				FftData.OutFreqArray[2] = (uint8_t)Freqs[3]; // 125 Hz
				FftData.OutFreqArray[3] = (uint8_t)Freqs[6]; // 250 Hz
				FftData.OutFreqArray[4] = (uint8_t)Freqs[12]; // 500 Hz
				FftData.OutFreqArray[5] = (uint8_t)Freqs[23]; // 1000 Hz
				FftData.OutFreqArray[6] = (uint8_t)Freqs[51]; // 2200 Hz
				FftData.OutFreqArray[7] = (uint8_t)Freqs[104]; // 4500 Hz
				FftData.OutFreqArray[8] = (uint8_t)Freqs[207]; // 9000 Hz
				FftData.OutFreqArray[9] = (uint8_t)Freqs[344]; // 15000 Hz
	}
}


void DrawFFT_Micro()
{
	char head[20]="MicroPhone";
	char strhbuf[40];

		GFX_ClearBuffer(MainWindow,LCDWIDTH, LCDHEIGHT);
		DrawHead(MainWindow, 1,1,head );
		GFX_SetFontSize(1);

		GFX_DrawString(MainWindow, 5, 16, "Wartosc ADC:", WHITE, BLACK);
		sprintf(strhbuf,"%lu ", ADC1Dat.Microphone);
		GFX_DrawString(MainWindow, 5, 28, strhbuf, WHITE, BLACK);
	/*for(uint8_t i = 0; i < 10; i++) // Each frequency
	{
	  GFX_DrawFillRectangle(MainWindow,10+(i*11), 64-FftData.OutFreqArray[i], 10, FftData.OutFreqArray[i], WHITE);
	}*/
	SSD1306_Display(MainWindow);
}


void ADC_MicrophoneConvCpltCallBack()
{
	MicrophoneDataReady=1;
}

void DrawFotoResistor()
{
	char head[20]="FotoResist";
	char strhbuf[40];

		GFX_ClearBuffer(MainWindow,LCDWIDTH, LCDHEIGHT);
		DrawHead(MainWindow, 1,1,head );
		GFX_SetFontSize(1);

		GFX_DrawString(MainWindow, 5, 16, "Wartosc ADC:", WHITE, BLACK);
		sprintf(strhbuf,"%lu ", ADC1Dat.FotoRez);
		GFX_DrawString(MainWindow, 5, 28, strhbuf, WHITE, BLACK);

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
	char el10[20]="IR_4438";
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
	GFX_DrawString(Window,70,0,StringP, WHITE, BLACK);
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






