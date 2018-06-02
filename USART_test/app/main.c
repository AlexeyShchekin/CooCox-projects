#include <stm32f10x.h>
#include <stm32f10x_gpio.h>
#include <stm32f10x_rcc.h>
#include "sys_init.h"

#define   get_led_state(pin) ((GPIOB->ODR & pin) != 0)
uint8_t DMA_BUSY = 0;

void Delay(void)
{
	volatile uint32_t i;
	for (i=0; i != 0x70000; i++);
}

void send_to_uart(uint8_t data)
{
	while(!(USART1->SR & 0x40));
	USART1->DR=data;
}

void send_str(char * string)
{
	uint8_t i = 0;
	while(string[i])
	{
		send_to_uart(string[i]);
		i++;
	}
	send_to_uart('\r');
	send_to_uart('\n');
}

void DMA1_Channel4_IRQHandler(void)
{
	if (DMA_GetITStatus(DMA1_IT_TC5))
	{
		DMA_ClearITPendingBit(DMA1_IT_TC5);

		USART_DMACmd(USART1, USART_DMAReq_Tx, DISABLE);
		DMA_Cmd(DMA1_Channel4, DISABLE);
		DMA_BUSY = 0;
		Delay_ms(1000);
	}
}

int main(void)
{
	init_clock();

	GPIO_InitTypeDef gpio_port;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_USART1 | RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

	gpio_port.GPIO_Pin = GPIO_Pin_10;
	gpio_port.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &gpio_port);

	gpio_port.GPIO_Pin = GPIO_Pin_9;
	gpio_port.GPIO_Speed = GPIO_Speed_50MHz;
	gpio_port.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOA, &gpio_port);

	gpio_port.GPIO_Pin = 0x300;
	gpio_port.GPIO_Speed = GPIO_Speed_2MHz;
	gpio_port.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOB, &gpio_port);

	GPIOB->CRL |= 0x400000;
	GPIOB->CRL &= ~0xB00000;

	/*USART1->BRR=0x1d4c;
	USART1->CR1 |= USART_CR1_UE;
	USART1->CR1 |= USART_CR1_TE;*/

	USART_InitTypeDef uart_struct;
	uart_struct.USART_BaudRate = 115200;
	uart_struct.USART_WordLength = USART_WordLength_8b;
	uart_struct.USART_StopBits = USART_StopBits_1;
	uart_struct.USART_Parity = USART_Parity_No;
	uart_struct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	uart_struct.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

	USART_Init(USART1, &uart_struct);
	USART_Cmd(USART1, ENABLE);

	uint8_t data[20];
	int i=0;
	for (i=0;i<20;i++) data[i] = i;

	DMA_InitTypeDef DMA_INI;

	DMA_DeInit(DMA1_Channel4);
	DMA_INI.DMA_PeripheralBaseAddr = (uint32_t)&(USART1->DR);
	DMA_INI.DMA_MemoryBaseAddr = (uint32_t)&data[0];
	DMA_INI.DMA_DIR = DMA_DIR_PeripheralDST;
	DMA_INI.DMA_BufferSize = 20;
	DMA_INI.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_INI.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_INI.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
	DMA_INI.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
	DMA_INI.DMA_Mode = DMA_Mode_Circular;
	DMA_INI.DMA_Priority = DMA_Priority_VeryHigh;
	DMA_INI.DMA_M2M = DMA_M2M_Disable;
	DMA_Init(DMA1_Channel4, &DMA_INI);

	while(1)
	{
		while (DMA_BUSY==1) {};
		DMA_BUSY = 1;

		USART_DMACmd(USART1, USART_DMAReq_Tx, ENABLE);
		DMA_ITConfig(DMA1_Channel4, DMA_IT_TC, ENABLE);
		NVIC_EnableIRQ(DMA1_Channel4_IRQn);

		DMA_Cmd(DMA1_Channel4, ENABLE);

		Delay_ms(1000);
	}
}
