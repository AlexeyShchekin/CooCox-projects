#include <stm32f10x.h>
#include <stm32f10x_gpio.h>
#include <stm32f10x_rcc.h>
#include "sys_init.h"

int main()
{
	init_clock();
	GPIO_InitTypeDef GPIO_InitStructure;

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

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2,ENABLE);

	AFIO->MAPR |= AFIO_MAPR_SWJ_CFG_DISABLE;
	AFIO->MAPR |= AFIO_MAPR_TIM2_REMAP_FULLREMAP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	TIM2->PSC = SystemCoreClock/1000 - 1;
	TIM2->ARR = 10;
	TIM2->CCER |= TIM_CCER_CC1E;
	TIM2->CCMR1|=(TIM_CCMR1_OC1M_0| TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_2);
	TIM2->CR1 |= TIM_CR1_CEN;

	TIM2->CCR1 = 9;

    while(1)
    {
    	/*TIM2->CCR1++;
    	if (TIM2->CCR1>=1000) TIM2->CCR1 = 0;
    	Delay_ms(1);*/
    }
}
