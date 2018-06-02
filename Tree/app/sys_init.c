#include <stm32f10x.h>
#include <stm32f10x_gpio.h>
#include <stm32f10x_rcc.h>
#include "sys_init.h"

volatile uint32_t usTicks;

void SysTick_Handler(void)
{
	usTicks++;
}

void WAIT(uint32_t dlyTicks)
{
	uint32_t curTicks;

	curTicks = usTicks;
	while ((usTicks - curTicks) < dlyTicks);
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
			FLASH->ACR |= (uint32_t)FLASH_ACR_LATENCY_0;
			RCC->CFGR |= (uint32_t)RCC_CFGR_HPRE_DIV128;
			RCC->CFGR |= (uint32_t)RCC_CFGR_PPRE2_DIV16;
			RCC->CFGR |= (uint32_t)RCC_CFGR_PPRE1_DIV16;
			RCC->CFGR &= (uint32_t)((uint32_t)~(RCC_CFGR_PLLSRC | RCC_CFGR_PLLXTPRE | RCC_CFGR_PLLMULL));
			RCC->CFGR |= (uint32_t)(RCC_CFGR_PLLSRC_HSI_Div2 | RCC_CFGR_PLLMULL2);

			RCC->CR |= RCC_CR_PLLON;

			while((RCC->CR & RCC_CR_PLLRDY) == 0) {};
			RCC->CFGR &= (uint32_t)((uint32_t)~(RCC_CFGR_SW));
			RCC->CFGR |= (uint32_t)RCC_CFGR_SW_PLL;

			while ((RCC->CFGR & (uint32_t)RCC_CFGR_SWS) != (uint32_t)0x08) {};
			RCC->CR|=RCC_CR_CSSON;
		}

	SystemCoreClockUpdate();
	SysTick_Config(SystemCoreClock/1000L);
}

void Start(void)
{
	init_clock();
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);

	GPIO_InitTypeDef GPIO_InitStructure;

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOB,&GPIO_InitStructure);

	GPIOB->BSRR |= GPIO_BSRR_BR0 | GPIO_BSRR_BR1;

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOA,&GPIO_InitStructure);

	GPIOA->BSRR |= GPIO_BSRR_BR0 | GPIO_BSRR_BR1 | GPIO_BSRR_BR2 | GPIO_BSRR_BR3 | GPIO_BSRR_BR4 | GPIO_BSRR_BR5 | GPIO_BSRR_BR6 | GPIO_BSRR_BR7;
}
