#include <stm32f10x_rcc.h>
#include <stm32f10x_gpio.h>
#include <stm32f10x.h>

void delay(uint32_t i)
{
	volatile uint32_t j;
	for (j=0;j!=1000*i;j++);
}

void LongDelay(void)
{
	volatile uint32_t i;
	for (i=0; i != 0x70000; i++);
}

void send_to_uart(uint8_t data)
{
	while(!(USART1->SR & 0x40));
	USART1->DR=data;
}

int main(void)
{
	GPIO_InitTypeDef PORT;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_USART1 |RCC_APB2Periph_AFIO, ENABLE);

	PORT.GPIO_Pin = (GPIO_Pin_8 | GPIO_Pin_9);
	PORT.GPIO_Speed = GPIO_Speed_2MHz;
	PORT.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOB,&PORT);

	PORT.GPIO_Pin = GPIO_Pin_10;
	PORT.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &PORT);

	PORT.GPIO_Pin = GPIO_Pin_9;
	PORT.GPIO_Speed = GPIO_Speed_50MHz;
	PORT.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOA, &PORT);

	USART_InitTypeDef uart_struct;
	uart_struct.USART_BaudRate = 9600;
	uart_struct.USART_WordLength = USART_WordLength_8b;
	uart_struct.USART_StopBits = USART_StopBits_1;
	uart_struct.USART_Parity = USART_Parity_No;
	uart_struct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	uart_struct.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

	USART_Init(USART1, &uart_struct);
	USART_Cmd(USART1, ENABLE);

	RCC_APB2PeriphClockCmd(RCC_APB2ENR_ADC1EN, ENABLE);
	ADC1->CR2 |= ADC_CR2_CAL;
	while (!(ADC1->CR2 & ADC_CR2_CAL));

	ADC1->SMPR2 |= (ADC_SMPR2_SMP1_2 | ADC_SMPR2_SMP1_1 | ADC_SMPR2_SMP1_0);
	ADC1->CR2 |= ADC_CR2_JEXTSEL;
	ADC1->CR2 |= ADC_CR2_JEXTTRIG;
	ADC1->CR2 |= ADC_CR2_CONT;
	ADC1->CR1 |= ADC_CR1_JAUTO;

	ADC1->JSQR |= (1<<15);
	ADC1->CR2 |= ADC_CR2_ADON;
	ADC1->CR2 |= ADC_CR2_JSWSTART;
	while (!(ADC1->SR & ADC_SR_JEOC));

	uint32_t adc_res;

    while(1)
    {
    	adc_res = ADC1->JDR1;
    	send_to_uart(adc_res);
    	//send_to_uart('\n');
    	LongDelay();
    }
}
