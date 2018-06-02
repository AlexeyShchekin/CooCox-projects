#include <stm32f10x.h>
#include <stm32f10x_rcc.h>
#include <stm32f10x_gpio.h>
#include <stm32f10x_flash.h>
#include <stm32f10x_bkp.h>

#include "sys_init.h"

uint32_t BASE_ADDRESS = 0x08008000;

void send_to_uart(uint8_t data)
{
	while(!(USART1->SR & 0x40));
	USART1->DR=data;
}

void send16_to_uart(uint16_t data)
{
	send_to_uart((data>>8)&0xFF);
	send_to_uart(data&0xFF);
}

void send32_to_uart(uint32_t data)
{
	send_to_uart((data>>24)&0xFF);
	send_to_uart((data>>16)&0xFF);
	send_to_uart((data>>8)&0xFF);
	send_to_uart(data&0xFF);
}

void USART1_ON(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef uart_struct;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_USART1 | RCC_APB2Periph_AFIO, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;		// INPUTS UART RX
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;	// OUTPUT UART TX
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	uart_struct.USART_BaudRate = 9600;
	uart_struct.USART_WordLength = USART_WordLength_8b;
	uart_struct.USART_StopBits = USART_StopBits_1;
	uart_struct.USART_Parity = USART_Parity_No;
	uart_struct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	uart_struct.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(USART1, &uart_struct);
	USART_Cmd(USART1, ENABLE);
}

void USART1_OFF(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	while (USART_GetFlagStatus(USART1,USART_FLAG_TC)==RESET) {};
	USART_Cmd(USART1, DISABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, DISABLE);
}

uint8_t flash_ready(void)
{
	return !(FLASH->SR & FLASH_SR_BSY);
}

uint32_t flash_read(uint32_t address)
{
	return (*(__IO uint32_t*) address);
}

void flash_erase_page(uint32_t address)
{
	FLASH->CR|= FLASH_CR_PER;
	FLASH->AR = address;
	FLASH->CR|= FLASH_CR_STRT;
	while(!flash_ready()){}
	FLASH->CR&= ~FLASH_CR_PER;
}

void flash_write(uint32_t address, uint32_t data)
{
	FLASH->CR |= FLASH_CR_PG;
	while(!flash_ready());
	*(__IO uint16_t*)address = (uint16_t)data;
	while(!flash_ready());
	address+=2;
	data>>=16;
	*(__IO uint16_t*)address = (uint16_t)data;
	while(!flash_ready());
	FLASH->CR &= ~(FLASH_CR_PG);
}

uint32_t get_current_logaddr(void)
{
	return ((BKP->DR2&0x0000FFFF)<<16)|(BKP->DR3&0x0000FFFF);
}

void set_current_logaddr(uint32_t CURR_ADDR)
{
	BKP->DR2 = (CURR_ADDR>>16)&0xFFFF;
	BKP->DR3 = (CURR_ADDR)&0xFFFF;
}


void log_current_point(uint32_t CURR_ADDR, uint32_t time, double Depth)
{
	flash_write(CURR_ADDR, time);
	flash_write(CURR_ADDR+4, (uint32_t)(Depth*10));
	set_current_logaddr(CURR_ADDR+8);
}

int main(void)
{
	int i=0;
	uint32_t times[64];
	double Depths[64];
	for (i=0; i<64; i++)
	{
		if (i<32)
		{
			times[i] = i;
			Depths[i] = (double)(i*(32-i))/16;
		}
		else
		{
			times[i] = i-32;
			Depths[i] = (double)((63-i)*(i-32))/16;
		}
	}

	init_clock();

	USART1_ON();
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);
	PWR_BackupAccessCmd(ENABLE);

	if ((BKP->DR1&0x0002)==0x0000)
	{
		BKP->DR2 = (BASE_ADDRESS>>16)&0xFFFF;
		BKP->DR3 = (BASE_ADDRESS)&0xFFFF;
		BKP->DR1 |= 0x0002;
		FLASH_Unlock();
		flash_erase_page(BASE_ADDRESS);
		FLASH_Lock();
	}

	FLASH_Unlock();
	while (!flash_ready()) {}
	for (i=0; i<64;i++)
	{
		log_current_point(get_current_logaddr(),times[i],Depths[i]);

	}
	while (!flash_ready()) {}
	FLASH_Lock();

	BKP->DR4 |= 0x0002;

    while(1)
    {
    	if (USART1->SR & USART_SR_RXNE)
    	{
    		if (USART1->DR == 'r')
    		{
    			send16_to_uart(BKP->DR4);
    			uint32_t ADDR = get_current_logaddr();
				send32_to_uart(ADDR);
    			int i=0;
				uint32_t Pnts = (uint32_t)((ADDR-BASE_ADDRESS)/4);
				uint32_t DATA;
				for (i=0;i<Pnts;i++)
				{
					DATA = flash_read(BASE_ADDRESS+4*i);
					send32_to_uart(DATA);
				}
    		}
    	}
    	Delay_ms(100);
    }
}
