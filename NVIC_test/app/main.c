#include <stm32f10x.h>
#include <stm32f10x_gpio.h>
#include <stm32f10x_rcc.h>
#include "sys_init.h"

volatile uint8_t PWRFLAG = 0;

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
	RCC_APB2PeriphClockCmd(RCC_APB2ENR_ADC1EN | RCC_APB2ENR_ADC2EN , DISABLE);

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA , ENABLE);
	//RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB , ENABLE);

	RCC_APB2PeriphClockCmd(RCC_APB2ENR_AFIOEN , ENABLE);

	/*GPIO_InitStructure.GPIO_Pin = (GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_15);
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = (GPIO_Pin_4);
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIOB->BSRR |= GPIO_BSRR_BS9 | GPIO_BSRR_BR10 | GPIO_BSRR_BR11 | GPIO_BSRR_BS12 | GPIO_BSRR_BS13 | GPIO_BSRR_BS15;
	GPIOA->BSRR |= GPIO_BSRR_BS4;*/

	GPIO_InitStructure.GPIO_Pin = (GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2);
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	USART1_ON();

	AFIO->EXTICR[0]&=0x00000000;
	AFIO->EXTICR[0]|=AFIO_EXTICR1_EXTI0_PA | AFIO_EXTICR1_EXTI1_PA | AFIO_EXTICR1_EXTI2_PA;
	EXTI->IMR|=(EXTI_IMR_MR0 | EXTI_IMR_MR1 | EXTI_IMR_MR2);
	EXTI->RTSR|=(EXTI_RTSR_TR0 | EXTI_RTSR_TR1 | EXTI_RTSR_TR2);
	NVIC_EnableIRQ (EXTI2_IRQn);
	NVIC_EnableIRQ (EXTI1_IRQn);
	NVIC_EnableIRQ (EXTI0_IRQn);

	send_to_uart(0x05);
	//GPIOB->ODR^=GPIO_Pin_8;
	//Delay_ms(1000);
	//GPIOB->ODR^=GPIO_Pin_8;
	while(1)
	{
		/*if ((GPIOA->IDR&0x0004)==0x0004)
		{
			send_to_uart(0x02);
		}
		if ((GPIOA->IDR&0x0002)==0x0002)
		{
			send_to_uart(0x01);
		}
		if ((GPIOA->IDR&0x0001)==0x0001)
		{
			send_to_uart(0x00);
		}*/

		//send_to_uart(GPIOA->IDR&0xFF);

		Delay_ms(100);
		if (PWRFLAG==0)
		{
			//PWR_EnterSTOPMode(PWR_Regulator_ON, PWR_STOPEntry_WFI);
			__WFI();
		}
		else
		{
			PWRFLAG = 0;
			PWR_WakeUpPinCmd(ENABLE);
			PWR_EnterSTANDBYMode();
		}
	}
}

void EXTI2_IRQHandler(void)
{
	send_to_uart(0x02);
	EXTI->PR|=0x04;
}

void EXTI1_IRQHandler(void)
{
	send_to_uart(0x01);
	EXTI->PR|=0x02;
}

void EXTI0_IRQHandler(void)
{
	send_to_uart(0x00);
	PWRFLAG = 1;
	EXTI->PR|=0x01;
}
