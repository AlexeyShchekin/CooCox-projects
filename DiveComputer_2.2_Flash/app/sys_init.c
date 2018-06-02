#include <stm32f10x.h>
#include <stm32f10x_gpio.h>
#include <stm32f10x_rcc.h>
#include "sys_init.h"

volatile uint32_t usTicks;

void SysTick_Handler(void)
{
	usTicks++;
}

uint32_t GetTicks(void)
{
	return usTicks;
}

void Delay_us(uint32_t dlyTicks)
{
	uint32_t curTicks;

	curTicks = usTicks;
	while ((usTicks - curTicks) < dlyTicks);
}

void Delay_ms(uint32_t dlyTicks)
{
	uint32_t dlyMTicks = 1000*dlyTicks;
	uint32_t curTicks;

	curTicks = usTicks;
	while ((usTicks - curTicks) < dlyMTicks);
}

void init_clock(void)
{
	__IO uint32_t StartUpCounter = 0, HSEStatus = 0;
	 RCC_DeInit();

		RCC->CR &= (uint32_t)((uint32_t)~RCC_CR_HSEON);
		RCC->CR |= (uint32_t)((uint32_t)RCC_CR_HSION);
		StartUpCounter = 0;
		do
		{
			HSEStatus = RCC->CR & RCC_CR_HSIRDY;
			StartUpCounter++;
		}
		while((HSEStatus == 0) && (StartUpCounter != HSEStartUp_TimeOut));
		if ((RCC->CR & RCC_CR_HSIRDY) != RESET)
		{
			FLASH->ACR |= FLASH_ACR_PRFTBE;
			FLASH->ACR &= (uint32_t)((uint32_t)~FLASH_ACR_LATENCY);
			FLASH->ACR |= (uint32_t)FLASH_ACR_LATENCY_2;
			RCC->CFGR |= (uint32_t)RCC_CFGR_HPRE_DIV1;
			RCC->CFGR |= (uint32_t)RCC_CFGR_PPRE2_DIV1;
			RCC->CFGR |= (uint32_t)RCC_CFGR_PPRE1_DIV2;
			RCC->CFGR |= (uint32_t)RCC_CFGR_ADCPRE_DIV8;
			RCC->CFGR &= (uint32_t)((uint32_t)~(RCC_CFGR_PLLSRC | RCC_CFGR_PLLXTPRE | RCC_CFGR_PLLMULL));
			RCC->CFGR |= (uint32_t)(RCC_CFGR_PLLSRC_HSI_Div2 | RCC_CFGR_PLLMULL16);

			RCC->CR |= RCC_CR_PLLON;

			while((RCC->CR & RCC_CR_PLLRDY) == 0) {};
			RCC->CFGR &= (uint32_t)((uint32_t)~(RCC_CFGR_SW));
			RCC->CFGR |= (uint32_t)RCC_CFGR_SW_PLL;

			while ((RCC->CFGR & (uint32_t)RCC_CFGR_SWS) != (uint32_t)0x08) {};
			RCC->CR|=RCC_CR_CSSON;
		}

	SystemCoreClockUpdate();
	SysTick_Config(SystemCoreClock/1000000L);
}
