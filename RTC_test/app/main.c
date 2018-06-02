#include <stm32f10x.h>
#include <stm32f10x_gpio.h>
#include <stm32f10x_rcc.h>
#include <stm32f10x_rtc.h>
#include <stm32f10x_bkp.h>
#include "sys_init.h"

volatile uint8_t PWR_FLAG = 0;
volatile uint32_t RTC_Time = 19*3600 + 23*60 + 0;
volatile uint8_t reinitRTC = 0;

void send_to_uart(uint8_t data)
{
	while(!(USART1->SR & 0x40));
	USART1->DR=data;
}

void RTC_IRQHandler(void)
{
  if (RTC_GetITStatus(RTC_IT_SEC) != RESET)
  {
    RTC_ClearITPendingBit(RTC_IT_SEC);
    RTC_WaitForLastTask();

     // GPIOB->ODR ^= GPIO_Pin_13;
     // GPIOB->ODR ^= GPIO_Pin_14;

      uint8_t b0,b1,b2,b3;
      uint32_t T;
      T = RTC_GetCounter();
		b0 = (uint8_t)(T&0x000000FF);
		b1 = (uint8_t)((T&0x0000FF00)>>8);
		b2 = (uint8_t)((T&0x00FF0000)>>16);
		b3 = (uint8_t)((T&0xFF000000)>>24);
		send_to_uart(b3);
		send_to_uart(b2);
		send_to_uart(b1);
		send_to_uart(b0);
  }
}

void initRTC(uint8_t reinit)
{
	uint16_t i;
	for(i=0;i<5000;i++) { ; }

	PWR_BackupAccessCmd(ENABLE);

	if (reinit) BKP_DeInit();
	RCC_LSEConfig(RCC_LSE_ON);
	while (RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET) {};
	RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);
	RCC_RTCCLKCmd(ENABLE);
	RTC_WaitForSynchro();
	RTC_WaitForLastTask();
	RTC_ITConfig(RTC_IT_SEC, ENABLE);
	RTC_WaitForLastTask();
	RTC_SetPrescaler(32768); //25718
	RTC_WaitForLastTask();
	if (reinit) RTC_SetCounter(RTC_Time);
	RTC_WaitForLastTask();
	BKP_WriteBackupRegister(BKP_DR1, 0xA5A5);
	PWR_BackupAccessCmd(DISABLE);
}

int main(void)
{
	//init_clock();
	GPIO_InitTypeDef GPIO_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB |
			                         RCC_APB2Periph_GPIOC, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_All;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	GPIO_Init(GPIOC, &GPIO_InitStructure);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB |
							 RCC_APB2Periph_GPIOC, DISABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2ENR_ADC1EN | RCC_APB2ENR_ADC2EN , DISABLE);

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_USART1 | RCC_APB2Periph_AFIO, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2ENR_AFIOEN, ENABLE);

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);

	if ((PWR->CSR&0x00000002)==0)
	{
		reinitRTC = 1;
	}
	else
	{
		reinitRTC = 0;
	}
	initRTC(reinitRTC);

	//AFIO->MAPR |= AFIO_MAPR_SWJ_CFG_DISABLE;

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin= GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;
	GPIO_Init(GPIOB,&GPIO_InitStructure);

	GPIOB->ODR |= GPIO_Pin_10;
	GPIOB->ODR |= GPIO_Pin_11;
	GPIOB->ODR |= GPIO_Pin_15;

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
	NVIC_InitStructure.NVIC_IRQChannel = RTC_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	/*NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
	NVIC_InitStructure.NVIC_IRQChannel = RTCAlarm_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);*/

	AFIO->EXTICR[0] = 0x00000000;
	EXTI->IMR|=EXTI_IMR_MR0;
	EXTI->RTSR|=EXTI_RTSR_TR0;
	NVIC_EnableIRQ (EXTI0_IRQn);

	//GPIOB->ODR |= GPIO_Pin_8;
	//GPIOB->ODR |= GPIO_Pin_9;
/*	EXTI_InitTypeDef EXTI_InitStructure;
	EXTI_ClearITPendingBit(EXTI_Line17);
	EXTI_InitStructure.EXTI_Line = EXTI_Line17;
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure);
*/
	USART_InitTypeDef uart_struct;
	uart_struct.USART_BaudRate = 115200;
	uart_struct.USART_WordLength = USART_WordLength_8b;
	uart_struct.USART_StopBits = USART_StopBits_1;
	uart_struct.USART_Parity = USART_Parity_No;
	uart_struct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	uart_struct.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

	USART_Init(USART1, &uart_struct);
	USART_Cmd(USART1, ENABLE);

	//PWR_WakeUpPinCmd(DISABLE);

    while(1)
    {
    	//if (PWR_FLAG == 0)
    	//{
    		//RTC_Time = RTC_GetCounter();
    		//RTC_SetAlarm(RTC_Time + 1);
    		//PWR_EnterSTOPMode(PWR_Regulator_LowPower, PWR_STOPEntry_WFI);
    		//__WFI();
    		//GPIOB->ODR ^= GPIO_Pin_10;
    		//GPIOB->ODR ^= GPIO_Pin_15;
    	//	Delay_ms(500);
    	//}
    	//else
    	//{
       	//	RTC_Time = RTC_GetCounter();
    		//RTC_SetAlarm(RTC_Time + 5);
    	//	PWR_EnterSTOPMode(PWR_Regulator_LowPower, PWR_STOPEntry_WFI);
    		//PWR_WakeUpPinCmd(ENABLE);
    		//PWR_EnterSTANDBYMode();
		//}
    	//PWR_EnterSTOPMode(PWR_Regulator_ON, PWR_STOPEntry_WFI);
    	//__WFI();
    }
}

void EXTI0_IRQHandler(void)
{
	//GPIOB->ODR^=GPIO_Pin_8;
	if (PWR_FLAG == 0)
	{
		PWR_FLAG = 1;
	}
	else
	{
		PWR_FLAG = 0;
		//init_clock();
	}
	EXTI->PR|=0x01;
}
