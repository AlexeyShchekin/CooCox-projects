#include <stm32f10x_gpio.h>
#include <stm32f10x_rcc.h>
#include <stm32f10x_spi.h>
#include <stm32f10x_i2c.h>
#include "sys_init.h"
#include <st7735.h>
//#include <garmin-digits.h>
#include "MS5803_I2C.h"

uint8_t PWR_FLAG = 0;

/*void ST7735_BigDig(uint8_t digit, uint16_t X, uint16_t Y, uint16_t color) {
	uint8_t i,j;
    uint8_t CH = color >> 8;
    uint8_t CL = (uint8_t)color;

	CS_L();
	ST7735_AddrSet(X,Y,X + 15,Y + 43);
	A0_H();
	for (j = 0; j < 44; j++) {
		for (i = 0; i < 16; i++) {
			if ((garmin_big_digits[(digit * 96) + i + (j / 8) * 16] >> (j % 8)) & 0x01) {
    			ST7735_write(CH);
    			ST7735_write(CL);
			} else {
    			ST7735_write(0x00);
    			ST7735_write(0x00);
			}
		}
	}
	CS_H();
}

void ST7735_MidDig(uint8_t digit, uint16_t X, uint16_t Y, uint16_t color) {
	uint8_t i,j;
    uint8_t CH = color >> 8;
    uint8_t CL = (uint8_t)color;

	CS_L();
	ST7735_AddrSet(X,Y,X + 11,Y + 23);
	A0_H();
	for (j = 0; j < 24; j++) {
		for (i = 0; i < 12; i++) {
			if ((garmin_mid_digits[(digit * 36) + i + (j / 8) * 12] >> (j % 8)) & 0x01) {
    			ST7735_write(CH);
    			ST7735_write(CL);
			} else {
    			ST7735_write(0x00);
    			ST7735_write(0x00);
			}
		}
	}
	CS_H();
}

void ST7735_SmallDig(uint8_t digit, uint16_t X, uint16_t Y, uint16_t color) {
	uint8_t i,j;
    uint8_t CH = color >> 8;
    uint8_t CL = (uint8_t)color;

	CS_L();
	ST7735_AddrSet(X,Y,X + 10,Y + 20);
	A0_H();
	for (j = 0; j < 21; j++) {
		for (i = 0; i < 11; i++) {
			if ((garmin_small_digits[(digit * 33) + i + (j / 8) * 11] >> (j % 8)) & 0x01) {
    			ST7735_write(CH);
    			ST7735_write(CL);
			} else {
    			ST7735_write(0x00);
    			ST7735_write(0x00);
			}
		}
	}
	CS_H();
}*/


uint8_t i;

#define CS_LOW		GPIOB->BSRR |= GPIO_BSRR_BR13;
#define CS_HIGH		GPIOB->BSRR |= GPIO_BSRR_BS13;
#define CLK_LOW		GPIOA->BSRR |= GPIO_BSRR_BR5;
#define CLK_HIGH	GPIOA->BSRR |= GPIO_BSRR_BS5;
#define MOSI_LOW	GPIOA->BSRR |= GPIO_BSRR_BR7;
#define MOSI_HIGH	GPIOA->BSRR |= GPIO_BSRR_BS7;

void HDEL(void)
{
	Delay_us(1);
}

void MOSI_OUT(uint8_t mask)
{
	if ((mask&0x80)==0x80) {MOSI_HIGH;}
	else {MOSI_LOW;}
}

void MOSI_OUT32(uint32_t mask)
{
	if ((mask&0x80000000)==0x80000000) {MOSI_HIGH;}
	else {MOSI_LOW;}
}

void WriteEnable(void)//0x06
{
	int i;
	uint8_t cmd = 0x06;
	CS_LOW;
	HDEL();

	for (i=0;i<8;i++)
	{
		MOSI_OUT(cmd);
		HDEL();
		CLK_HIGH;
		HDEL();
		CLK_LOW;
		cmd = cmd<<1;
	}

	HDEL();
	CS_HIGH;
}

uint16_t ReadStatusRegister(uint8_t cmd)
{
	int i;
	uint16_t value = 0x0000;
	CS_LOW;
	HDEL();

	for (i=0;i<8;i++)
	{
		MOSI_OUT(cmd);
		HDEL();
		CLK_HIGH;
		HDEL();
		CLK_LOW;
		cmd = cmd<<1;
	}
	for (i=0;i<16;i++)
	{
		HDEL();
		CLK_HIGH;
		value = value<<1;
		if ((GPIOA->IDR&GPIO_Pin_6)==GPIO_Pin_6) value |= 0x0001;
		HDEL();
		CLK_LOW;
	}

	HDEL();
	CS_HIGH;
	return value;
}

uint8_t ReadData(uint32_t addr)
{
	int i;
	uint8_t value = 0x00;
	addr = addr<<8;
	uint8_t cmd = 0x03;

	CS_LOW;
	HDEL();

	for (i=0;i<8;i++)
	{
		MOSI_OUT(cmd);
		HDEL();
		CLK_HIGH;
		HDEL();
		CLK_LOW;
		cmd = cmd<<1;
	}
	for (i=0;i<24;i++)
	{
		MOSI_OUT32(addr);
		HDEL();
		CLK_HIGH;
		HDEL();
		CLK_LOW;
		addr = addr<<1;
	}
	for (i=0;i<8;i++)
	{
		HDEL();
		CLK_HIGH;
		value = value<<1;
		if ((GPIOA->IDR&GPIO_Pin_6)==GPIO_Pin_6) value |= 0x01;
		HDEL();
		CLK_LOW;
	}

	HDEL();
	CS_HIGH;
	return value;
}

void SectorErase(uint32_t addr)
{
	int i;
	addr = addr<<8;
	uint8_t cmd = 0x20;

	CS_LOW;
	HDEL();

	for (i=0;i<8;i++)
	{
		MOSI_OUT(cmd);
		HDEL();
		CLK_HIGH;
		HDEL();
		CLK_LOW;
		cmd = cmd<<1;
	}
	for (i=0;i<24;i++)
	{
		MOSI_OUT32(addr);
		HDEL();
		CLK_HIGH;
		HDEL();
		CLK_LOW;
		addr = addr<<1;
	}

	HDEL();
	CS_HIGH;
}

void ProgramData(uint32_t addr,uint8_t value)
{
	int i;
	addr = addr<<8;
	uint8_t cmd = 0x02;

	CS_LOW;
	HDEL();

	for (i=0;i<8;i++)
	{
		MOSI_OUT(cmd);
		HDEL();
		CLK_HIGH;
		HDEL();
		CLK_LOW;
		cmd = cmd<<1;
	}
	for (i=0;i<24;i++)
	{
		MOSI_OUT32(addr);
		HDEL();
		CLK_HIGH;
		HDEL();
		CLK_LOW;
		addr = addr<<1;
	}
	for (i=0;i<8;i++)
	{
		MOSI_OUT(value);
		HDEL();
		CLK_HIGH;
		HDEL();
		CLK_LOW;
		value = value<<1;
	}

	HDEL();
	CS_HIGH;
}

void USART1_ON(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef uart_struct;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_USART1 | RCC_APB2Periph_AFIO, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;		// INPUTS UART RX
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;	// OUTPUT UART TX
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	uart_struct.USART_BaudRate = 256000;
	uart_struct.USART_WordLength = USART_WordLength_8b;
	uart_struct.USART_StopBits = USART_StopBits_1;
	uart_struct.USART_Parity = USART_Parity_No;
	uart_struct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	uart_struct.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(USART1, &uart_struct);
	USART_Cmd(USART1, ENABLE);
}

void send_to_uart(uint8_t data)
{
	while(!(USART1->SR & 0x40));
	USART1->DR=data;
}

void send16_to_uart(uint16_t data)
{
	send_to_uart((data>>8)&0xFF);
	send_to_uart(data&0xFF);
}

void send32_to_uart(uint32_t data)
{
	send_to_uart((data>>24)&0xFF);
	send_to_uart((data>>16)&0xFF);
	send_to_uart((data>>8)&0xFF);
	send_to_uart(data&0xFF);
}

void TempToStr(char Buff[], double Temp)
{
	char Bd[2];
	char Br[1];
	int Sd,Sr, sign;
	Sr = ((int)(Temp*10.0))%10;
	Sd = (int)(((int)(Temp*10.0))/10);
	if (Sd < 0)
	{
		sign = 1;
		Sd = -Sd;
	}
	else sign = 0;

	itoa(Sd,Bd,10);
	itoa(Sr,Br,10);
	if (sign==1)
	{
		Buff[0] = '-';
		Buff[1] = Bd[0];
		Buff[2] = Bd[1];
		Buff[3] = '.';
		Buff[4] = Br[0];
	}
	else
	{
		Buff[0] = Bd[0];
		Buff[1] = Bd[1];
		Buff[2] = '.';
		Buff[3] = Br[0];
		Buff[4] = ' ';
	}
}

void PressToStr(char Buff[], double Temp)
{
	char Bd[4];
	char Br[1];
	int Sd,Sr;
	Sr = ((int)(Temp*10.0))%10;
	Sd = (int)(((int)(Temp*10.0))/10);

	itoa(Sd,Bd,10);
	itoa(Sr,Br,10);

	Buff[0] = Bd[0];
	Buff[1] = Bd[1];
	Buff[2] = Bd[2];
	Buff[3] = Bd[3];
	Buff[4] = '.';
	Buff[5] = Br[0];
}

void EXTI_Configuration(void)
{
	EXTI_InitTypeDef EXTI_InitStructure;

  	EXTI_ClearITPendingBit(EXTI_Line0);
	EXTI_InitStructure.EXTI_Line = EXTI_Line0;
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising_Falling;
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure);

	EXTI_ClearITPendingBit(EXTI_Line1);
	EXTI_InitStructure.EXTI_Line = EXTI_Line1;
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising_Falling;
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure);

	EXTI_ClearITPendingBit(EXTI_Line2);
	EXTI_InitStructure.EXTI_Line = EXTI_Line2;
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising_Falling;
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure);
}

void DrawDiveDesktop(void)
{
	uint16_t color = RGB565(255,255,0);
	uint16_t backcolor = RGB565(0,0,0);
	ST7735_PutStr7x11_DMA(85,55,"Non-Deco:",color,backcolor,1);
	ST7735_PutStr5x7_DMA(120,25,"Dive:",color,backcolor,1);
	ST7735_PutStr5x7_DMA(65,25,"Speed:",color,backcolor,1);
}

void DrawSurfDesktop(void)
{
	uint16_t color = RGB565(255,255,0);
	uint16_t backcolor = RGB565(0,0,0);
	//ST7735_PutStr5x7_DMA(65,25,"Date:",color,backcolor,1);
	ST7735_PutStr5x7_DMA(120,25,"Surf:",color,backcolor,1);
	ST7735_PutStr7x11_DMA(105,55,"Time:",color,backcolor,1);
}

void DrawDesktop(void)
{
	ST7735_FillRect_DMA(0,0,159,127,RGB565(0,0,0));

	uint16_t color = RGB565(255,0,0);
	uint16_t backcolor = RGB565(0,0,0);

	ST7735_Rect(0,20,159,127,color);
	ST7735_HLine(0,159,51,color);
	ST7735_HLine(0,159,96,color);

	color = RGB565(0,255,0);
	int i;
	for (i=0;i<16;i++)
	{
		ST7735_Rect(10*i,0,10*(i+1)-1,19,color);
	}

	color = RGB565(255,255,0);

	ST7735_PutStr7x11_DMA(5,55,"Depth, m:",color,backcolor,1);

	ST7735_PutStr5x7_DMA(110,100,"CNS O :",color,backcolor,1);
	ST7735_PutStr5x7_DMA(140,102,"2",color,backcolor,1);

	ST7735_PutStr5x7_DMA(77,100,"O :",color,backcolor,1);
	ST7735_PutStr5x7_DMA(84,102,"2",color,backcolor,1);
	ST7735_PutStr5x7_DMA(5,100,"Max Depth:",color,backcolor,1);

	ST7735_PutStr5x7_DMA(15,25,"T, C:",color,backcolor,1);
	ST7735_PutStr5x7_DMA(27,21,"o",color,backcolor,1);

	/*
	char O2Printout[4];
	char MaxDepthPrintout[6];
	(FlToStr(CO2*100)+"%").toCharArray(O2Printout, 4);
	(FlToStr(10*PO2_lim/CO2-10.0)).toCharArray(MaxDepthPrintout, 6);
	TFTscreen.text(O2Printout, 75, 115);
	TFTscreen.text(MaxDepthPrintout, 20, 115);
	TFTscreen.setTextSize(2);*/
}

uint16_t readADC1(void)
{
	uint16_t result;
	GPIO_InitTypeDef GPIOinit;
	GPIOinit.GPIO_Mode = GPIO_Mode_AIN;
	GPIOinit.GPIO_Pin = GPIO_Pin_1;
	GPIO_Init(GPIOA, &GPIOinit);

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);

	ADC_InitTypeDef ADC_InitStructure;
	ADC_StructInit(&ADC_InitStructure);
	ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
	ADC_InitStructure.ADC_ScanConvMode = DISABLE;
	ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
	ADC_InitStructure.ADC_NbrOfChannel = 1;
	ADC_Init(ADC1, &ADC_InitStructure);
	ADC_Cmd(ADC1, ENABLE);

	ADC_RegularChannelConfig(ADC1, ADC_Channel_3, 1, ADC_SampleTime_55Cycles5);

	ADC_ResetCalibration(ADC1);
	while (ADC_GetResetCalibrationStatus(ADC1));
	ADC_StartCalibration(ADC1);
	while (ADC_GetCalibrationStatus(ADC1));

	ADC_SoftwareStartConvCmd(ADC1, ENABLE);
	while(ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET);
	result = ADC_GetConversionValue(ADC1);

	ADC_Cmd(ADC1, DISABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, DISABLE);

	GPIOinit.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIOinit.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIOinit);

   return result;
}

void SET_MOSI_FLASH(void)
{
	GPIO_InitTypeDef gpio;
	gpio.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_7;
	gpio.GPIO_Mode = GPIO_Mode_Out_PP;
	gpio.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &gpio);

	CS_HIGH;
	CLK_LOW;
}

void SET_MOSI_DISPLAY(void)
{
	GPIO_InitTypeDef gpio;
	gpio.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_7;
	gpio.GPIO_Mode = GPIO_Mode_AF_PP;
	gpio.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &gpio);
}

int main(void)
{
	init_clock();

	USART1_ON();
	  /* Configure EXTI Line to generate an interrupt on falling edge */
	EXTI_Configuration();

	AFIO->EXTICR[0] = 0x00000000;

	NVIC_EnableIRQ (EXTI0_IRQn);
	NVIC_EnableIRQ (EXTI1_IRQn);
	NVIC_EnableIRQ (EXTI2_IRQn);

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);
	GPIO_InitTypeDef GPIO_InitStructure;

	GPIO_InitStructure.GPIO_Pin = (GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_15);
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOB,&GPIO_InitStructure);

	GPIOB->BSRR |= GPIO_BSRR_BS9 | GPIO_BSRR_BS10 | GPIO_BSRR_BS11 | GPIO_BSRR_BS12 | GPIO_BSRR_BS13 | GPIO_BSRR_BS15;

	//MS5803_Init();

	ST7735_Init();
	ST7735_AddrSet(0,0,159,127);
	ST7735_Orientation(scr_CW);
	ST7735_Clear(0x0000);

	/*ST7735_PutStr5x7(0,0,"Hello ST7735 screen!",RGB565(255,0,0));
	ST7735_PutStr5x7(0,10,"This is 5x7 font",RGB565(0,255,0));
	ST7735_PutStr5x7(0,20,"Screen 128x160 pixels",RGB565(0,0,255));*/

	/*float T = 25.5;
	float P = 1013.2;
	char Tstr[5];
	char Pstr[6];
	TempToStr(Tstr,T);
	PressToStr(Pstr,P);
	ST7735_PutStr5x7_DMA(0,80,Tstr,RGB565(255,255,0),RGB565(0,0,255),1);
	ST7735_PutStr5x7_DMA(0,100,Pstr,RGB565(255,255,0),RGB565(0,0,255),1);

	Delay_ms(1000);*/

	float Data[2]={0.0,0.0};

	//MS5803_reset();
	//MS5803_begin();

	DrawDesktop();
	DrawSurfDesktop();
	//uint16_t adc;

	//USART1_ON();

	//SET_MOSI_FLASH();

	//WriteEnable();
	/*Delay_ms(10);
	SectorErase(0x00000000);
	Delay_ms(1000);
	WriteEnable();
	Delay_ms(10);
	ProgramData(0x00000000,0x76);*/
	Delay_ms(100);

	while(1)
	{
		//ST7735_FillRect_DMA(0,0,159,127,RGB565(255,255,0));
		//ST7735_FillRect_DMA(0,0,159,127,RGB565(255,255,0));

		//send16_to_uart(ReadStatusRegister(0x05));
		//send16_to_uart(ReadStatusRegister(0x35));
		//send_to_uart(ReadData(0x00000000));
		GPIOB->BSRR |= GPIO_BSRR_BS11;
		send16_to_uart(readADC1());
		GPIOB->BSRR |= GPIO_BSRR_BR11;
		send16_to_uart(readADC1());
		Delay_ms(1000);

		SET_MOSI_DISPLAY();
		char Tstr[5];
		char Pstr[6];
		//char adc_str[6];
		//MS5803_getFullData(CELSIUS,ADC_4096,Data);
		TempToStr(Tstr,Data[0]);
		PressToStr(Pstr,Data[1]);
		ST7735_PutStr7x11_DMA(10,37,Tstr,RGB565(255,255,255),RGB565(0,0,0),1);
		ST7735_PutStr10x16_DMA(75,75,"00:00:00",RGB565(255,255,255),RGB565(0,0,0),1);
		ST7735_PutStr5x7_DMA(10,75,Pstr,RGB565(255,255,255),RGB565(0,0,0),2);

		ST7735_PutStr5x7_DMA(60,25,"01 May",RGB565(255,255,255),RGB565(0,0,0),1);
		ST7735_PutStr5x7_DMA(53,40,"2016 SUN",RGB565(255,255,255),RGB565(0,0,0),1);

		ST7735_PutStr5x7_DMA(110,40,"00:00:00",RGB565(255,255,255),RGB565(0,0,0),1);

		/*ST7735_PutChar10x16(100,60,'A',RGB565(255,255,255),RGB565(0,0,255));
		ST7735_PutChar10x16(120,60,'a',RGB565(255,255,255),RGB565(0,0,255));
		ST7735_PutChar7x11(100,80,'A',RGB565(255,255,255),RGB565(0,0,255));
		ST7735_PutChar7x11(120,80,'a',RGB565(255,255,255),RGB565(0,0,255));*/

		if (PWR_FLAG==1)
		{
			//PWR_WakeUpPinCmd(ENABLE);
			//PWR_EnterSTANDBYMode();
			DrawDesktop();
			DrawDiveDesktop();
		}
		//adc = 0;
		//adc += readADC1();
		//adc += readADC1();
		//adc += readADC1();
		//adc += readADC1();
		//PressToStr(adc_str,(uint16_t)(adc/4));
		//ST7735_PutStr5x7_DMA(5,110,adc_str,RGB565(255,255,255),RGB565(0,0,0),1);
		Delay_ms(500);
		//ST7735_FillRect(0,0,159,127,RGB565(255,0,255));
		//ST7735_FillRect(0,0,159,127,RGB565(255,0,255));
		//Delay_ms(1000);
		//SET_MOSI_FLASH();
	}
}

void EXTI0_IRQHandler(void)
{
	EXTI_ClearITPendingBit(EXTI_Line0);

	if (PWR_FLAG==0)
	{
		PWR_FLAG = 1;
	}
	else
	{
		PWR_FLAG = 0;
		//init_clock();
	}

	EXTI_ClearFlag(EXTI_Line0);
}

void EXTI1_IRQHandler(void)
{
	EXTI_ClearITPendingBit(EXTI_Line1);

	if (PWR_FLAG==0)
	{
		PWR_FLAG = 1;
	}
	else
	{
		PWR_FLAG = 0;
		//init_clock();
	}

	EXTI_ClearFlag(EXTI_Line1);
}

void EXTI2_IRQHandler(void)
{
	EXTI_ClearITPendingBit(EXTI_Line2);

	if (PWR_FLAG==0)
	{
		PWR_FLAG = 1;
	}
	else
	{
		PWR_FLAG = 0;
		//init_clock();
	}

	EXTI_ClearFlag(EXTI_Line2);
}
