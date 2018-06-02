#include <stm32f10x.h>
#include <stm32f10x_gpio.h>
#include <stm32f10x_rcc.h>
#include <stm32f10x_rtc.h>
#include <stm32f10x_bkp.h>
#include <stm32f10x_usart.h>
#include "sys_init.h"

volatile uint8_t PWR_FLAG = 0;

void send_to_uart(uint8_t data)
{
	while(!(USART1->SR & 0x40));
	USART1->DR=data;
}

void RTCAlarm_IRQHandler(void)
{
	if (RTC_GetITStatus(RTC_IT_ALR) != RESET)
	{
		/* Clear EXTI line17 pending bit */
		EXTI_ClearITPendingBit(EXTI_Line17);

		/* Check if the Wake-Up flag is set */
		if (PWR_GetFlagStatus(PWR_FLAG_WU) != RESET)
		{
			/* Clear Wake Up flag */
			PWR_ClearFlag(PWR_FLAG_WU);
		}

		/* Wait until last write operation on RTC registers has finished */
		RTC_WaitForLastTask();

		/* Clear RTC Alarm interrupt pending bit */
		RTC_ClearITPendingBit(RTC_IT_ALR);

		/* Wait until last write operation on RTC registers has finished */
		RTC_WaitForLastTask();

		init_clock();
		USART_ON();
		uint32_t T;
		T = RTC_GetCounter();
		send_to_uart((T>>24)&0x000000FF);
		send_to_uart((T>>16)&0x000000FF);
		send_to_uart((T>>8)&0x000000FF);
		send_to_uart(T&0x000000FF);
		USART_OFF();
		GPIOB->ODR ^= GPIO_Pin_9;
	}
}

void NVIC_Configuration(void)
{
	NVIC_InitTypeDef NVIC_InitStructure;

	/* Configure one bit for preemption priority */
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
	/* Enable the RTC Alarm Interrupt */
	NVIC_InitStructure.NVIC_IRQChannel = RTCAlarm_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}

/******************************************************************************/

void EXTI_Configuration(void)
{
	EXTI_InitTypeDef EXTI_InitStructure;
	/* Configure EXTI Line17(RTC Alarm) to generate an interrupt on rising edge */
	EXTI_ClearITPendingBit(EXTI_Line17);
	EXTI_InitStructure.EXTI_Line = EXTI_Line17;
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure);

	/*EXTI_ClearITPendingBit(EXTI_Line0);
	EXTI_InitStructure.EXTI_Line = EXTI_Line0;
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure);*/
}

void RTC_Configuration(void)
{
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE); /* Enable PWR and BKP clocks */

  PWR_BackupAccessCmd(ENABLE); /* Allow access to BKP Domain */

  if (PWR_GetFlagStatus(PWR_FLAG_SB) != RESET) /* Check if the StandBy flag is set */
  {
    PWR_ClearFlag(PWR_FLAG_SB); /* Clear StandBy flag */
    RTC_WaitForSynchro(); /* Wait for RTC APB registers synchronisation */
    /* No need to configure the RTC as the RTC configuration(clock source, enable,
       prescaler,...) is kept after wake-up from STANDBY */
  }
  else
  {
    /* StandBy flag is not set */
    BKP_DeInit(); /* Reset Backup Domain */
    RCC_LSEConfig(RCC_LSE_ON); /* Enable LSE */
    while (RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET) {};
    RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);
    RCC_RTCCLKCmd(ENABLE); /* Enable RTC Clock */
    RTC_WaitForSynchro(); /* Wait for RTC registers synchronization */
    RTC_WaitForLastTask(); /* Wait until last write operation on RTC registers has finished */
    RTC_SetPrescaler(32767); /* RTC period = RTCCLK/RTC_PR = (32.768 KHz)/(32767+1) */
    RTC_WaitForLastTask(); /* Wait until last write operation on RTC registers has finished */
  }
  RTC_ITConfig(RTC_IT_ALR, ENABLE); /* Enable the RTC Alarm */
  RTC_WaitForLastTask(); /* Wait until last write operation on RTC registers has finished */
}

void USART_ON(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;

	/* Configure USART Tx as alternate function push-pull */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;  // PA.09 USART1.TX
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	/* Configure USART Rx as input floating */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10; // PA.10 USART1.RX
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	USART_InitStructure.USART_BaudRate = 115200;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
	USART_Init(USART1, &USART_InitStructure);
	USART_Cmd(USART1, ENABLE);
	while (USART_GetFlagStatus(USART1, USART_FLAG_TC)==RESET) {};
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

	GPIO_InitTypeDef PORT;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB |
									 RCC_APB2Periph_GPIOC, ENABLE);

	PORT.GPIO_Pin = GPIO_Pin_All;
	PORT.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_Init(GPIOA, &PORT);
	GPIO_Init(GPIOB, &PORT);
	GPIO_Init(GPIOC, &PORT);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB |
							 RCC_APB2Periph_GPIOC, DISABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2ENR_ADC1EN | RCC_APB2ENR_ADC2EN , DISABLE);

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2ENR_AFIOEN, ENABLE);

	RCC_ClearFlag();

	PORT.GPIO_Pin = GPIO_Pin_0;
	PORT.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &PORT);

	PORT.GPIO_Pin = GPIO_Pin_9;
	PORT.GPIO_Speed = GPIO_Speed_50MHz;
	PORT.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOB, &PORT);

	NVIC_Configuration();
	EXTI_Configuration();
	RTC_Configuration();

	GPIOB->BSRR = GPIO_BSRR_BS9;

	//NVIC_EnableIRQ (EXTI0_IRQn);

	//RTC_ClearFlag(RTC_FLAG_SEC); /* Wait till RTC Second event occurs */
	//while(RTC_GetFlagStatus(RTC_FLAG_SEC) == RESET);

	while(1)
	{
		if (PWR_FLAG==0)
		{
			RTC_SetAlarm(RTC_GetCounter()+1);
			RTC_WaitForLastTask();
			PWR_EnterSTOPMode(PWR_Regulator_LowPower,PWR_STOPEntry_WFI);
		}
		else
		{
			Delay_ms(100);
			PWR_EnterSTANDBYMode();
		}
	}

}

/*void EXTI0_IRQHandler(void)
{
	EXTI->PR|=0x01;
	if (PWR_FLAG==0)
	{
		PWR_FLAG = 1;
	}
	else
	{
		PWR_FLAG = 0;
		init_clock();
	}
}
*/
