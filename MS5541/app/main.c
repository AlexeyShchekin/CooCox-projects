#include <stm32f10x.h>
#include <stm32f10x_gpio.h>
#include <stm32f10x_rcc.h>
//#include <stm32f10x_rtc.h>
#include <stm32f10x_spi.h>
#include "sys_init.h"
#include "MS5541.h"

void send_to_uart(uint8_t data)
{
	while(!(USART1->SR & 0x40));
	USART1->DR=data;
}

int main(void)
{
	init_clock();

	InitSensor();

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

	long C[6];
	long long Data[2];

    while(1)
    {
    	readCalibration(C);
    	readData(C, Data);
    	/*send_to_uart((uint8_t)((result1&0x0000FF00)>>8));
		send_to_uart((uint8_t)(result1&0x000000FF));

		send_to_uart((uint8_t)((result2&0x0000FF00)>>8));
		send_to_uart((uint8_t)(result2&0x000000FF));

		send_to_uart((uint8_t)((result3&0x0000FF00)>>8));
		send_to_uart((uint8_t)(result3&0x000000FF));

		send_to_uart((uint8_t)((result4&0x0000FF00)>>8));
		send_to_uart((uint8_t)(result4&0x000000FF));

    	send_to_uart(SystemCoreClock/1000000UL);

		send_to_uart((uint8_t)((c1&0x0000FF00)>>8));
		send_to_uart((uint8_t)(c1&0x000000FF));

		send_to_uart((uint8_t)((c2&0x0000FF00)>>8));
		send_to_uart((uint8_t)(c2&0x000000FF));

		send_to_uart((uint8_t)((c3&0x0000FF00)>>8));
		send_to_uart((uint8_t)(c3&0x000000FF));

		send_to_uart((uint8_t)((c4&0x0000FF00)>>8));
		send_to_uart((uint8_t)(c4&0x000000FF));

		send_to_uart((uint8_t)((c5&0x0000FF00)>>8));
		send_to_uart((uint8_t)(c5&0x000000FF));

		send_to_uart((uint8_t)((c6&0x0000FF00)>>8));
		send_to_uart((uint8_t)(c6&0x000000FF));

		send_to_uart((uint8_t)((D1&0x0000FF00)>>8));
		send_to_uart((uint8_t)(D1&0x000000FF));

		send_to_uart((uint8_t)((D2&0x0000FF00)>>8));
		send_to_uart((uint8_t)(D2&0x000000FF));*/

		send_to_uart((uint8_t)(((Data[1])&0x0000FF00)>>8));
		send_to_uart((uint8_t)((Data[1])&0x000000FF));

		send_to_uart((uint8_t)(((Data[0])&0x0000FF00)>>8));
		send_to_uart((uint8_t)((Data[0])&0x000000FF));

    	Delay_ms(300);
    }
}
