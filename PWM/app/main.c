#include <stm32f10x.h>
#include <stm32f10x_gpio.h>
#include <stm32f10x_rcc.h>
#include "sys_init.h"
#include "MS5803_I2C.h"

void send_to_uart(uint8_t data)
{
	while(!(USART1->SR & 0x40));
	USART1->DR=data;
}

void USART_ON(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef uart_struct;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO | RCC_APB2Periph_USART1, ENABLE);

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
}

void USART_OFF(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	while (USART_GetFlagStatus(USART1,USART_FLAG_TC)== RESET) {};
	USART_Cmd(USART1, DISABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, DISABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
}

int main()
{
	init_clock();
	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_All;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	GPIO_Init(GPIOC, &GPIO_InitStructure);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC, DISABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2ENR_ADC2EN , DISABLE);


	USART_ON();
	send_to_uart(0x01);
	MS5803_Init();
	send_to_uart((uint8_t)(SystemCoreClock/1000000L));

	MS5803_reset();
	MS5803_begin();
	float Data[2];

	while(1)
	{
		MS5803_getFullData(CELSIUS,ADC_4096,Data);

		uint16_t P = (uint16_t)(Data[1]);
		uint8_t T = (uint8_t)(Data[0]);
		send_to_uart((P>>8)&0xFF);
		send_to_uart((P)&0xFF);
		send_to_uart((T)&0xFF);

		Delay_ms(1000);
	}

}

/*void USART1_IRQHandler(void)
{
	if (USART_GetFlagStatus(USART1, USART_FLAG_RXNE)==SET)
	{
		uint8_t data = USART1->DR;
		if (data=='1')
		{
			acquireFlag = 1;
		}
	}
}*/

/*void EXTI0_IRQHandler(void)
{
	GPIOB->ODR^=GPIO_Pin_8;
	EXTI->PR|=0x01;
}*/
