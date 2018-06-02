#include <stm32f10x.h>
#include <stm32f10x_gpio.h>
#include <stm32f10x_rcc.h>
#include <stm32f10x_bkp.h>
#include <stm32f10x_rtc.h>
#include "sys_init.h"

int main(void)
{
	init_clock();

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2ENR_AFIOEN, ENABLE);

	GPIO_InitTypeDef GPIO_InitStructure;

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOB,&GPIO_InitStructure);

	GPIOB->BSRR |= GPIO_BSRR_BS0 | GPIO_BSRR_BS1;

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOA,&GPIO_InitStructure);

	GPIOA->BSRR |= GPIO_BSRR_BS0 | GPIO_BSRR_BS1 | GPIO_BSRR_BS2 | GPIO_BSRR_BS3 | GPIO_BSRR_BS4 | GPIO_BSRR_BS5 | GPIO_BSRR_BS6 | GPIO_BSRR_BS7;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE); /* Enable PWR and BKP clocks */
	PWR_BackupAccessCmd(ENABLE);

	BKP_DeInit();
	RCC_LSEConfig(RCC_LSE_ON); /* Enable LSE */
	while (RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET) {};
	RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);
	RCC_RTCCLKCmd(ENABLE); /* Enable RTC Clock */
	RTC_WaitForSynchro(); /* Wait for RTC registers synchronization */
	RTC_WaitForLastTask(); /* Wait until last write operation on RTC registers has finished */
	RTC_SetPrescaler(32767); /* RTC period = RTCCLK/RTC_PR = (32.768 KHz)/(32767+1) */
	RTC_WaitForLastTask(); /* Wait until last write operation on RTC registers has finished */
	BKP_SetRTCCalibrationValue(0x00);

	GPIOB->BSRR |= GPIO_BSRR_BS15;
	int i;
    while(1)
    {
    	for (i=0;i<5;i++)
    	{
    		GPIOB->BSRR |= GPIO_BSRR_BS0 | GPIO_BSRR_BS1 | GPIO_BSRR_BS10 | GPIO_BSRR_BS11 | GPIO_BSRR_BS12 | GPIO_BSRR_BS13 | GPIO_BSRR_BS15;
    		GPIOA->BSRR |= GPIO_BSRR_BS0 | GPIO_BSRR_BS1 | GPIO_BSRR_BS2 | GPIO_BSRR_BS3 | GPIO_BSRR_BS4 | GPIO_BSRR_BS5 | GPIO_BSRR_BS6 | GPIO_BSRR_BS7;
    		Delay_ms(1000);
    		GPIOB->BSRR |= GPIO_BSRR_BR0 | GPIO_BSRR_BR1 | GPIO_BSRR_BR10 | GPIO_BSRR_BR11 | GPIO_BSRR_BR12 | GPIO_BSRR_BR13 | GPIO_BSRR_BR15;
    		GPIOA->BSRR |= GPIO_BSRR_BR0 | GPIO_BSRR_BR1 | GPIO_BSRR_BR2 | GPIO_BSRR_BR3 | GPIO_BSRR_BR4 | GPIO_BSRR_BR5 | GPIO_BSRR_BR6 | GPIO_BSRR_BR7;
    		Delay_ms(1000);
    	}
    }
}
