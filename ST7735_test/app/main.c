#include <stdlib.h>
#include <stm32f10x.h>
#include <stm32f10x_rcc.h>
#include <stm32f10x_gpio.h>
#include <stm32f10x_spi.h>
#include "ST7735.h"
#include "sys_init.h"

void send_to_uart(uint8_t data)
{
	while(!(USART1->SR & 0x40));
	USART1->DR=data;
}
/*
char DgToChar(uint8_t dg)
{
	switch (dg)
	{
	case 0:
		return '0';
		break;
	case 1:
		return '1';
		break;
	case 2:
		return '2';
		break;
	case 3:
		return '3';
		break;
	case 4:
		return '4';
		break;
	case 5:
		return '5';
		break;
	case 6:
		return '6';
		break;
	case 7:
		return '7';
		break;
	case 8:
		return '8';
		break;
	case 9:
		return '9';
		break;
	}
}*/

void TempToStr(char Buff[], float Temp)
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

int main(void)
{
	init_clock();
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef uart_struct;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1 | RCC_APB2Periph_AFIO, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;		// INPUTS UART RX
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;	// OUTPUT UART TX
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	uart_struct.USART_BaudRate = 115200;
	uart_struct.USART_WordLength = USART_WordLength_8b;
	uart_struct.USART_StopBits = USART_StopBits_1;
	uart_struct.USART_Parity = USART_Parity_No;
	uart_struct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	uart_struct.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(USART1, &uart_struct);
	USART_Cmd(USART1, ENABLE);

	send_to_uart((uint8_t)(SystemCoreClock/1000000));
	spiInit(SPI2);
	send_to_uart(0x01);
	DisplaySensorInit();
	send_to_uart(0x01);
	//ST7735_backLight(0);
	int size = 2;

  	long C[6];
  	long long Data[2];
  	char PStr[9] = "     mmHg";
  	char TStr[6] = "     C";
  	float Temp;

	while(1)
	{
		//readCalibration(C);
		//readData(C, Data);

		fill_rect_with_color(0,0,160, 128 ,ColorRGB(255,255,0));

		/*Temp = ((float)Data[0])/10;
		TempToStr(TStr, Temp);
		itoa((int)(Data[1]/1.3332),PStr,10);

		display_string(20, 20, TStr, 6, ColorRGB(255,255,0), ColorRGB(0,0,0),size);
		display_string(20, 60, PStr, 9, ColorRGB(255,255,0), ColorRGB(0,0,0),size);

		send_to_uart((uint8_t)(((Data[1])&0x0000FF00)>>8));
		send_to_uart((uint8_t)((Data[1])&0x000000FF));

		send_to_uart((uint8_t)(((Data[0])&0x0000FF00)>>8));
		send_to_uart((uint8_t)((Data[0])&0x000000FF));*/

		Delay_ms(500);
	}
}
