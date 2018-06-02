#include <stm32f10x.h>
#include <stm32f10x_gpio.h>
#include <stm32f10x_rcc.h>
//#include <stm32f10x_spi.h>
//#include <stm32f10x_tim.h>

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
		RCC->CFGR |= (RCC_CFGR_PLLSRC_HSE | RCC_CFGR_PLLMULL7);

		RCC->CR |= RCC_CR_PLLON;
		while((RCC->CR & RCC_CR_PLLRDY) == 0);
		RCC->CFGR &= 0xFFFFFFFC;
		RCC->CFGR |= RCC_CFGR_SW_PLL;
		while ((RCC->CFGR & RCC_CFGR_SWS) != (uint32_t)0x08);
	}
}

void send_to_uart(uint8_t data)
{
	while(!(USART1->SR & 0x40));
	USART1->DR=data;
}

void Init_Timer(void)
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);

	TIM4->PSC = SystemCoreClock / 32768 - 1;
	TIM4->ARR = 1024;
	TIM4->DIER |= TIM_DIER_UIE;
	TIM4->CR1 |= TIM_CR1_CEN;

	NVIC_EnableIRQ(TIM4_IRQn);
}

void TIM4_IRQHandler()
{
//	GPIOB->ODR^=GPIO_Pin_9;
	TIM_ClearITPendingBit(TIM4, TIM_IT_Update);
}

int main(void)
{
	init_clock();
	SystemCoreClockUpdate();
	SysTick_Config(SystemCoreClock/1000);

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1 | RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);

	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef uart_struct;
	GPIO_InitStructure.GPIO_Pin = (GPIO_Pin_13 | GPIO_Pin_14);
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOB,&GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	Init_Timer();

	uart_struct.USART_BaudRate = 9600;
	uart_struct.USART_WordLength = USART_WordLength_8b;
	uart_struct.USART_StopBits = USART_StopBits_1;
	uart_struct.USART_Parity = USART_Parity_No;
	uart_struct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	uart_struct.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(USART1, &uart_struct);
	USART_Cmd(USART1, ENABLE);

	GPIOB->ODR^=GPIO_Pin_9;
    while(1)
    {
    	GPIOB->ODR^=GPIO_Pin_13;
    	GPIOB->ODR^=GPIO_Pin_14;
    	send_to_uart(SystemCoreClock/1000000UL);
    	Delay_ms(1000);
    }
}
