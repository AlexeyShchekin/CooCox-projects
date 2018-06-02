#include <stm32f10x.h>
#include <stm32f10x_gpio.h>
#include <stm32f10x_rcc.h>
#include <stm32f10x_rtc.h>
#include <stm32f10x_bkp.h>
#include "sys_init.h"

volatile uint8_t PWR_FLAG = 0;
volatile uint8_t WKP_FLAG = 0;

void send_to_uart(uint8_t data)
{
	while(!(USART1->SR & 0x40));
	USART1->DR=data;
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

    WKP_FLAG = 1;
    GPIOB->ODR^=GPIO_Pin_9;
  }
}

/******************************************************************************/

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

  EXTI_ClearITPendingBit(EXTI_Line0);
  	EXTI_InitStructure.EXTI_Line = EXTI_Line0;
  	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
  	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
  	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
  	EXTI_Init(&EXTI_InitStructure);
}

/******************************************************************************/

void RTC_Configuration(void)
{
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE); /* Enable PWR and BKP clocks */

  PWR_BackupAccessCmd(ENABLE); /* Allow access to BKP Domain */

  if ((BKP->DR1&0x01)==0x00)
  {
    /* StandBy flag is not set */
    BKP_DeInit(); /* Reset Backup Domain */
   // BKP->DR1 = 0x01;
    //RCC_LSICmd(ENABLE);
    RCC_LSEConfig(RCC_LSE_ON); /* Enable LSE */
    while (RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET) {};
    RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);
    RCC_RTCCLKCmd(ENABLE); /* Enable RTC Clock */
    RTC_WaitForSynchro(); /* Wait for RTC registers synchronization */
    RTC_WaitForLastTask(); /* Wait until last write operation on RTC registers has finished */
    RTC_SetPrescaler(32767 ); /* RTC period = RTCCLK/RTC_PR = (32.768 KHz)/(32767+1) */
    RTC_WaitForLastTask(); /* Wait until last write operation on RTC registers has finished */
    BKP->DR1 |= 0x01;
  }
  else
  {
	  RTC_WaitForSynchro();
  }
  RTC_ITConfig(RTC_IT_ALR, ENABLE); /* Enable the RTC Alarm */
  RTC_WaitForLastTask(); /* Wait until last write operation on RTC registers has finished */
}

/******************************************************************************/

int main(void)
{
	init_clock();
	GPIO_InitTypeDef GPIO_InitStructure;
	//NVIC_InitTypeDef NVIC_InitStructure;

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
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2ENR_AFIOEN, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2;		// INPUT BUTTONS
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;		// BATTERY ADC
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

  RCC_ClearFlag();
  /* NVIC configuration */
  NVIC_Configuration();
  /* Configure EXTI Line to generate an interrupt on falling edge */
  EXTI_Configuration();
  USART_ON();
  send_to_uart(0x86);
  /* Configure RTC clock source and prescaler */
  RTC_Configuration();
  PWR_FLAG = 0;
  send_to_uart(0x87);

  GPIO_InitStructure.GPIO_Mode=GPIO_Mode_Out_PP;
  	GPIO_InitStructure.GPIO_Pin= GPIO_Pin_9;
  	GPIO_InitStructure.GPIO_Speed=GPIO_Speed_2MHz;
  	GPIO_Init(GPIOB,&GPIO_InitStructure);
  	GPIOB->BSRR = GPIO_BSRR_BS9;

  	//AFIO->EXTICR[0] = 0x00000000;
  	//NVIC_EnableIRQ (EXTI0_IRQn);

  RTC_ClearFlag(RTC_FLAG_SEC); /* Wait till RTC Second event occurs */

  while(RTC_GetFlagStatus(RTC_FLAG_SEC) == RESET);

  uint32_t T;
  send_to_uart(0x89);
  //USART_OFF();
  while(1)
  {
	  if (WKP_FLAG == 1)
	  {
		  init_clock();
		 // USART_ON();
		  T = RTC_GetCounter();
		  send_to_uart((T>>24)&0xFF);
		  send_to_uart((T>>16)&0xFF);
		  send_to_uart((T>>8)&0xFF);
		  send_to_uart((T)&0xFF);
		  //USART_OFF();
		 // Delay_ms(500);
		  WKP_FLAG = 0;
	  }
	  if (PWR_FLAG==0)
	  {
		  RTC_SetAlarm(RTC_GetCounter()+1);
		  RTC_WaitForLastTask();
		  PWR_EnterSTOPMode(PWR_Regulator_LowPower, PWR_STOPEntry_WFI);
	  }
	  else if (PWR_FLAG==1)
	  {
		  Delay_ms(500);
		  PWR_WakeUpPinCmd(ENABLE);
		  PWR_EnterSTANDBYMode();
	  }
  }
}

void EXTI0_IRQHandler(void)
{
/*	if (PWR_FLAG==0)
	{
		PWR_FLAG = 1;
	}
	else
	{
		PWR_FLAG = 0;
		init_clock();
	}*/
	EXTI->PR|=0x01;
}
