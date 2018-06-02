#include <stm32f10x.h>
#include <stm32f10x_gpio.h>
#include <stm32f10x_rcc.h>
#include "sys_init.h"

void send_to_uart(uint8_t data)
{
	while(!(USART1->SR & 0x40));
	USART1->DR=data;
}

int main(void)
{
	init_clock();

	GPIO_InitTypeDef gpio_port;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_USART1 | RCC_APB2Periph_AFIO, ENABLE);

	gpio_port.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_10;
	gpio_port.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &gpio_port);

	gpio_port.GPIO_Pin = GPIO_Pin_9;
	gpio_port.GPIO_Speed = GPIO_Speed_50MHz;
	gpio_port.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOA, &gpio_port);

	USART_InitTypeDef uart_struct;
	uart_struct.USART_BaudRate = 115200;
	uart_struct.USART_WordLength = USART_WordLength_8b;
	uart_struct.USART_StopBits = USART_StopBits_1;
	uart_struct.USART_Parity = USART_Parity_No;
	uart_struct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	uart_struct.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

	USART_Init(USART1, &uart_struct);
	USART_Cmd(USART1, ENABLE);

    while(1)
    {
    	if ((GPIOA->IDR)&0x40) send_to_uart(0x01);
    	else send_to_uart(0x00);
    	Delay_ms(17);
    }
}
