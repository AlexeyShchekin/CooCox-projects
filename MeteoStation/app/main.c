//#include <string.h>
#include <stm32f10x.h>
#include <stm32f10x_gpio.h>
#include <stm32f10x_rcc.h>
//#include <stm32f10x_bkp.h>
//#include <stm32f10x_rtc.h>
#include "sys_init.h"
#include "auxiliary.h"

#define DQ_HIGH GPIOB->BSRR |= GPIO_BSRR_BS4;
#define DQ_LOW GPIOB->BSRR |= GPIO_BSRR_BR4;

void send_presence()
{
	DQ_HIGH;
	Delay_us(10);	//10
	DQ_LOW;
	Delay_us(480);	//480
	DQ_HIGH;
	//while (GPIOB->IDR&DQ_Pin){}
	Delay_us(600);	//600
}

void one_wire_write_bit(uint8_t bit)
{
	DQ_LOW;
	Delay_us(bit ? 7 : 70);	//7:70
	DQ_HIGH;
	Delay_us(bit ? 83 : 10);	//83:10
}

uint8_t one_wire_read_bit()
{
	uint8_t bit = 0;
	DQ_LOW;
	Delay_us(1);		//1
	DQ_HIGH;
	Delay_us(9);		//9
	bit = (GPIOB->IDR & GPIO_Pin_4 ? 1 : 0);
	Delay_us(70);	//70
	return bit;
}

void one_wire_write_byte(uint8_t data)
{
	for(uint8_t i = 0; i<8; i++)
		one_wire_write_bit(data>>i & 1);
}

float GetTemperature(void)
{
	send_presence();
	one_wire_write_byte(0xCC);
	one_wire_write_byte(0x44);

	Delay_us(800000);	//800000

	send_presence();
	one_wire_write_byte(0xCC);
	one_wire_write_byte(0xBE);
	uint16_t data = 0;
	for(uint8_t i = 0; i<16; i++) data += (uint16_t)one_wire_read_bit()<<i;

	send_presence();

	return (float)data/16.0;
}

volatile char cmd = ' ';
volatile uint8_t currentByte = 0;
char received[50], rec_len=0;

int main(void)
{
	init_clock();

	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_All;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	GPIO_Init(GPIOC, &GPIO_InitStructure);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC, DISABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2ENR_ADC1EN | RCC_APB2ENR_ADC2EN , DISABLE);

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2ENR_AFIOEN, ENABLE);

	AFIO->MAPR|=AFIO_MAPR_SWJ_CFG_JTAGDISABLE;

	USART1_ON();
	USART3_ON();

	InitSensors();
	float temp = GetTemperature();

    while(1)
    {
    	//char c = uart_getc(USART3);
    	if (cmd=='0')
    	{
    		temp = GetTemperature();
    		char ptr[5];
    		TempToStr(temp, ptr);
    		uart_putc(ptr[0],USART3);//3
    		uart_putc(ptr[1],USART3);
    		uart_putc(ptr[2],USART3);
    		uart_putc(ptr[3],USART3);
    		uart_putc(ptr[4],USART3);
    		cmd = ' ';
    	}
    	else if (cmd=='1')
    	{
    		uart_putc(currentByte,USART3);
    		str_to_uart("ATD+79261559218\r");
    		Delay_ms(1000);
    		cmd = ' ';
    	}
    	else if (cmd=='2')
		{
			str_to_uart("AT+CMGF=1\r");
			Delay_ms(100);
			str_to_uart("AT+CMGS=\"+79261559218\"\r");
			Delay_ms(100);
			str_to_uart("Hi from Robot!");
			send_to_uart(0x1A);
			Delay_ms(1000);
			cmd = ' ';
		}
    	else if (cmd=='3')
    	{
    		send_str_to_uart("AT+CIPSTATUS\r",13);
    		while (USART_GetFlagStatus(USART1, USART_FLAG_RXNE) == SET)
    		{
    			char c = uart_getc(USART1);
    			uart_putc(c,USART3);
    		}
    		char host[17] = "data.sparkfun.com";
    		char publicKey[20] = "N8aGzlARA1IEO1YRn3Wx";
    		char privateKey[20] = "5269zqrDr4Ubd8zWGYZa";
    		cmd = ' ';
    	}
    }
}

void USART3_IRQHandler(void)
{
	if (USART_GetITStatus(USART3, USART_IT_RXNE))
	{
		USART_ClearITPendingBit(USART3, USART_IT_RXNE);
		currentByte = USART3->DR;
		cmd = '0';
	}
}
