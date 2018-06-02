#include <stm32f10x.h>
#include <stm32f10x_gpio.h>
#include <stm32f10x_rcc.h>
#include <stm32f10x_rtc.h>
#include <stm32f10x_bkp.h>
#include "sys_init.h"

volatile uint32_t msTicks;

void SysTick_Handler(void)
{
	msTicks++;
}

void Delay_ms(uint32_t dlyTicks)
{
	uint32_t curTicks;

	curTicks = msTicks;
	while ((msTicks - curTicks) < dlyTicks);
}

void init_clock(void)
{
	__IO uint32_t StartUpCounter = 0;
	FlagStatus HSEStatus = RESET;

	RCC->CR |= (uint32_t)0x00000001;
	RCC->CFGR &= (uint32_t)0xF8FF0000;
	RCC->CR &= (uint32_t)0xFEF6FFFF;
	RCC->CR &= (uint32_t)0xFFFAFFFF;
	RCC->CFGR &= (uint32_t)0xFF80FFFF;

	RCC->CR |= RCC_CR_HSEON;
	do
	{
		HSEStatus = RCC->CR & RCC_CR_HSERDY;
		StartUpCounter++;
	}
	while( (HSEStatus == RESET) && (StartUpCounter != HSEStartUp_TimeOut));

	if ((RCC->CR & RCC_CR_HSERDY) != 0)
	{
		RCC->CFGR &= 0xFFFFC00F;
		RCC->CFGR |= RCC_CFGR_HPRE_DIV1;
		RCC->CFGR |= RCC_CFGR_PPRE2_DIV1;
		RCC->CFGR |= RCC_CFGR_PPRE1_DIV2;

		RCC->CFGR &= 0xFFC0FFFF;
		RCC->CFGR |= (RCC_CFGR_PLLSRC_HSE | RCC_CFGR_PLLMULL9);

		RCC->CR |= RCC_CR_PLLON;
		while((RCC->CR & RCC_CR_PLLRDY) == 0);
		RCC->CFGR &= 0xFFFFFFFC;
		RCC->CFGR |= RCC_CFGR_SW_PLL;
		while ((RCC->CFGR & RCC_CFGR_SWS) != (uint32_t)0x08);
	}

	SystemCoreClockUpdate();
	SysTick_Config(SystemCoreClock/1000);
}
