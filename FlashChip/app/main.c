#include <stm32f10x.h>
#include <stm32f10x_gpio.h>
#include <stm32f10x_rcc.h>
#include <stm32f10x_spi.h>
#include "sys_init.h"

#define CS_LOW		GPIOB->BSRR |= GPIO_BSRR_BR13;
#define CS_HIGH		GPIOB->BSRR |= GPIO_BSRR_BS13;
#define CLK_LOW		GPIOA->BSRR |= GPIO_BSRR_BR5;
#define CLK_HIGH	GPIOA->BSRR |= GPIO_BSRR_BS5;
#define MOSI_LOW	GPIOA->BSRR |= GPIO_BSRR_BR7;
#define MOSI_HIGH	GPIOA->BSRR |= GPIO_BSRR_BS7;

uint16_t readADC1(void)
{
	uint16_t result;
	GPIO_InitTypeDef GPIOinit;
	GPIOinit.GPIO_Mode = GPIO_Mode_AIN;
	GPIOinit.GPIO_Pin = GPIO_Pin_3;
	GPIO_Init(GPIOA, &GPIOinit);

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);

	ADC_InitTypeDef ADC_InitStructure;
	ADC_StructInit(&ADC_InitStructure);
	ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
	ADC_InitStructure.ADC_ScanConvMode = DISABLE;
	ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
	ADC_InitStructure.ADC_NbrOfChannel = 1;
	ADC_Init(ADC1, &ADC_InitStructure);
	ADC_Cmd(ADC1, ENABLE);

	ADC_RegularChannelConfig(ADC1, ADC_Channel_3, 1, ADC_SampleTime_55Cycles5);

	ADC_ResetCalibration(ADC1);
	while (ADC_GetResetCalibrationStatus(ADC1));
	ADC_StartCalibration(ADC1);
	while (ADC_GetCalibrationStatus(ADC1));

	ADC_SoftwareStartConvCmd(ADC1, ENABLE);
	while(ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET);
	result = ADC_GetConversionValue(ADC1);

	ADC_Cmd(ADC1, DISABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, DISABLE);

	GPIOinit.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIOinit.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIOinit);

   return result;
}

void HDEL(void)
{
	Delay_us(1);
}

void send_to_uart(uint8_t data)
{
	while(!(USART1->SR & 0x40));
	USART1->DR = data;
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
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;		// INPUTS UART RX
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;	// OUTPUT UART TX
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	uart_struct.USART_BaudRate = 256000;
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
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, DISABLE);
}

/*void SPI_ON()
{
	GPIO_InitTypeDef gpio;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);

	gpio.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;
	gpio.GPIO_Mode = GPIO_Mode_AF_PP;
	gpio.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &gpio);

	gpio.GPIO_Pin = GPIO_Pin_4;
	gpio.GPIO_Speed = GPIO_Speed_50MHz;
	gpio.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOA, &gpio );
	CS_HIGH;

	SPI_Cmd (SPI1, DISABLE );
	FLASHSPI.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
	FLASHSPI.SPI_DataSize = SPI_DataSize_8b;
	FLASHSPI.SPI_CPOL = SPI_CPOL_Low;
	FLASHSPI.SPI_CPHA = SPI_CPHA_1Edge;
	FLASHSPI.SPI_NSS = SPI_NSS_Soft;
	FLASHSPI.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2;
	FLASHSPI.SPI_FirstBit = SPI_FirstBit_MSB;
	FLASHSPI.SPI_CRCPolynomial = 7;
	FLASHSPI.SPI_Mode = SPI_Mode_Master;
	SPI_Init(SPI1, &FLASHSPI);
	SPI_Cmd(SPI1, ENABLE);
	SPI_NSSInternalSoftwareConfig(SPI1, SPI_NSSInternalSoft_Set);
	SPI_CalculateCRC (SPI1, DISABLE );
}

void SPI_Flash_Send(uint8_t cmd)
{
	SPI_I2S_SendData(SPI1, cmd);
	while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY) == SET);
}*/

void MOSI_OUT(uint8_t mask)
{
	if ((mask&0x80)==0x80) {MOSI_HIGH;}
	else {MOSI_LOW;}
}

void MOSI_OUT32(uint32_t mask)
{
	if ((mask&0x80000000)==0x80000000) {MOSI_HIGH;}
	else {MOSI_LOW;}
}

void WriteEnable(void)//0x06
{
	int i;
	uint8_t cmd = 0x06;
	CS_LOW;
	HDEL();

	for (i=0;i<8;i++)
	{
		MOSI_OUT(cmd);
		HDEL();
		CLK_HIGH;
		HDEL();
		CLK_LOW;
		cmd = cmd<<1;
	}

	HDEL();
	CS_HIGH;
}

uint16_t ReadStatusRegister(uint8_t cmd)
{
	int i;
	uint16_t value = 0x0000;
	CS_LOW;
	HDEL();

	for (i=0;i<8;i++)
	{
		MOSI_OUT(cmd);
		HDEL();
		CLK_HIGH;
		HDEL();
		CLK_LOW;
		cmd = cmd<<1;
	}
	for (i=0;i<16;i++)
	{
		HDEL();
		CLK_HIGH;
		value = value<<1;
		if ((GPIOA->IDR&GPIO_Pin_6)==GPIO_Pin_6) value |= 0x0001;
		HDEL();
		CLK_LOW;
	}

	HDEL();
	CS_HIGH;
	return value;
}

uint8_t ReadData(uint32_t addr)
{
	int i;
	uint8_t value = 0x00;
	addr = addr<<8;
	uint8_t cmd = 0x03;

	CS_LOW;
	HDEL();

	for (i=0;i<8;i++)
	{
		MOSI_OUT(cmd);
		HDEL();
		CLK_HIGH;
		HDEL();
		CLK_LOW;
		cmd = cmd<<1;
	}
	for (i=0;i<24;i++)
	{
		MOSI_OUT32(addr);
		HDEL();
		CLK_HIGH;
		HDEL();
		CLK_LOW;
		addr = addr<<1;
	}
	for (i=0;i<8;i++)
	{
		HDEL();
		CLK_HIGH;
		value = value<<1;
		if ((GPIOA->IDR&GPIO_Pin_6)==GPIO_Pin_6) value |= 0x01;
		HDEL();
		CLK_LOW;
	}

	HDEL();
	CS_HIGH;
	return value;
}

void SectorErase(uint32_t addr)
{
	int i;
	addr = addr<<8;
	uint8_t cmd = 0x20;

	CS_LOW;
	HDEL();

	for (i=0;i<8;i++)
	{
		MOSI_OUT(cmd);
		HDEL();
		CLK_HIGH;
		HDEL();
		CLK_LOW;
		cmd = cmd<<1;
	}
	for (i=0;i<24;i++)
	{
		MOSI_OUT32(addr);
		HDEL();
		CLK_HIGH;
		HDEL();
		CLK_LOW;
		addr = addr<<1;
	}

	HDEL();
	CS_HIGH;
}

void ProgramData(uint32_t addr,uint8_t value)
{
	int i;
	addr = addr<<8;
	uint8_t cmd = 0x02;

	CS_LOW;
	HDEL();

	for (i=0;i<8;i++)
	{
		MOSI_OUT(cmd);
		HDEL();
		CLK_HIGH;
		HDEL();
		CLK_LOW;
		cmd = cmd<<1;
	}
	for (i=0;i<24;i++)
	{
		MOSI_OUT32(addr);
		HDEL();
		CLK_HIGH;
		HDEL();
		CLK_LOW;
		addr = addr<<1;
	}
	for (i=0;i<8;i++)
	{
		MOSI_OUT(value);
		HDEL();
		CLK_HIGH;
		HDEL();
		CLK_LOW;
		value = value<<1;
	}

	HDEL();
	CS_HIGH;
}

int main(void)
{
	init_clock();
	GPIO_InitTypeDef gpio;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);
	gpio.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_4 | GPIO_Pin_7;
	gpio.GPIO_Mode = GPIO_Mode_Out_PP;
	gpio.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &gpio);
	gpio.GPIO_Pin = GPIO_Pin_6;
	gpio.GPIO_Speed = GPIO_Speed_50MHz;
	gpio.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &gpio );
	gpio.GPIO_Pin = GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_9;
	gpio.GPIO_Mode = GPIO_Mode_Out_PP;
	gpio.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &gpio);

	GPIOB->BSRR |= GPIO_BSRR_BS12 | GPIO_BSRR_BS9;

	CS_HIGH;
	CLK_LOW;

	USART1_ON();

	WriteEnable();
	Delay_ms(10);
	SectorErase(0x00000000);
	Delay_ms(1000);
	WriteEnable();
	Delay_ms(10);
	ProgramData(0x00000000,0x57);
	Delay_ms(100);
	while(1)
    {
    	send16_to_uart(ReadStatusRegister(0x05));
    	send16_to_uart(ReadStatusRegister(0x35));
    	send_to_uart(ReadData(0x00000000));
    	GPIOB->BSRR |= GPIO_BSRR_BS11;
    	send16_to_uart(readADC1());
    	GPIOB->BSRR |= GPIO_BSRR_BR11;
		send16_to_uart(readADC1());
    	Delay_ms(1000);
    }
}
