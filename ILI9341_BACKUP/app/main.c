#include <stdlib.h>
#include <stm32f10x.h>
#include <stm32f10x_rcc.h>
#include <stm32f10x_gpio.h>
#include <stm32f10x_spi.h>
#include "sys_init.h"
#include "ili9341.h"

void send_to_uart(uint8_t data)
{
	while(!(USART1->SR & 0x40));
	USART1->DR=data;
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

int main(void)
{
	init_clock();
	ili9341_init();

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

	//long C[6];
	long long Data[2] = {234,1013};
	char PStr[9] = "     mmHg";
	char TStr[6] = "     C";
	double Temp;

    while(1)
    {
    	//readCalibration(C);
    	//readData(C, Data);

    	TFT_fillRect(0, 0, 319, 239, TFT_rgb2color(0,0,0));

    	Temp = ((double)Data[0])/10;
		TempToStr(TStr, Temp);
		itoa((int)(Data[1]/1.3332),PStr,10);

		TFT_DrawString(TStr, 6, 20, 20, TFT_rgb2color(255,255,0), TFT_rgb2color(0,0,0),2);
		TFT_DrawString(PStr, 9, 20, 60, TFT_rgb2color(255,255,0), TFT_rgb2color(0,0,0),2);

		TFT_fillRect(100, 100, 200, 200, TFT_rgb2color(0,255,255));

    //	send_to_uart((uint8_t)(((Data[1])&0x0000FF00)>>8));
	//	send_to_uart((uint8_t)((Data[1])&0x000000FF));

		//send_to_uart((uint8_t)(((Data[0])&0x0000FF00)>>8));
		//send_to_uart((uint8_t)((Data[0])&0x000000FF));

    	Delay_ms(300);
    }
}
