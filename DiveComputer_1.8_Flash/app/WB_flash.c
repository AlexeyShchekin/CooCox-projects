#include <stm32f10x.h>
#include <stm32f10x_gpio.h>
#include <stm32f10x_rcc.h>
#include <stm32f10x_spi.h>
#include "sys_init.h"
#include "WB_flash.h"

void InitWBSPI_Pins(void)
{
	GPIO_InitTypeDef gpio;
	gpio.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_7;
	gpio.GPIO_Mode = GPIO_Mode_Out_PP;
	gpio.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &gpio);
	gpio.GPIO_Pin = GPIO_Pin_6;
	gpio.GPIO_Speed = GPIO_Speed_50MHz;
	gpio.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &gpio );
}

void HDEL(void)
{
	Delay_us(1);
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
	if ((mask&0x80)==0x80) {WBFLASH_MOSI_HIGH;}
	else {WBFLASH_MOSI_LOW;}
}

void MOSI_OUT32(uint32_t mask)
{
	if ((mask&0x80000000)==0x80000000) {WBFLASH_MOSI_HIGH;}
	else {WBFLASH_MOSI_LOW;}
}

void SendFlashCmd(uint8_t cmd)
{
	int i;
	WBFLASH_CS_LOW;
	HDEL();

	for (i=0;i<8;i++)
	{
		MOSI_OUT(cmd);
		HDEL();
		WBFLASH_CLK_HIGH;
		HDEL();
		WBFLASH_CLK_LOW;
		cmd = cmd<<1;
	}

	HDEL();
	WBFLASH_CS_HIGH;
}

/*uint16_t ReadStatusRegister(uint8_t cmd)
{
	int i;
	uint16_t value = 0x0000;
	WBFLASH_CS_LOW;
	HDEL();

	for (i=0;i<8;i++)
	{
		MOSI_OUT(cmd);
		HDEL();
		WBFLASH_CLK_HIGH;
		HDEL();
		WBFLASH_CLK_LOW;
		cmd = cmd<<1;
	}
	for (i=0;i<16;i++)
	{
		HDEL();
		WBFLASH_CLK_HIGH;
		value = value<<1;
		if ((GPIOA->IDR&GPIO_Pin_6)==GPIO_Pin_6) value |= 0x0001;
		HDEL();
		WBFLASH_CLK_LOW;
	}

	HDEL();
	WBFLASH_CS_HIGH;
	return value;
}*/

uint8_t ReadData(uint32_t addr)
{
	int i;
	uint8_t value = 0x00;
	addr = addr<<8;
	uint8_t cmd = 0x03;

	WBFLASH_CS_LOW;
	HDEL();

	for (i=0;i<8;i++)
	{
		MOSI_OUT(cmd);
		HDEL();
		WBFLASH_CLK_HIGH;
		HDEL();
		WBFLASH_CLK_LOW;
		cmd = cmd<<1;
	}
	for (i=0;i<24;i++)
	{
		MOSI_OUT32(addr);
		HDEL();
		WBFLASH_CLK_HIGH;
		HDEL();
		WBFLASH_CLK_LOW;
		addr = addr<<1;
	}
	for (i=0;i<8;i++)
	{
		HDEL();
		WBFLASH_CLK_HIGH;
		value = value<<1;
		if ((GPIOA->IDR&GPIO_Pin_6)==GPIO_Pin_6) value |= 0x01;
		HDEL();
		WBFLASH_CLK_LOW;
	}

	HDEL();
	WBFLASH_CS_HIGH;
	return value;
}

/*void ReadArray(uint32_t addr, uint8_t *Data, uint16_t length)
{
	int i,k;
	uint8_t value = 0x00;
	addr = addr<<8;
	uint8_t cmd = 0x03;

	WBFLASH_CS_LOW;
	HDEL();

	for (i=0;i<8;i++)
	{
		MOSI_OUT(cmd);
		HDEL();
		WBFLASH_CLK_HIGH;
		HDEL();
		WBFLASH_CLK_LOW;
		cmd = cmd<<1;
	}
	for (i=0;i<24;i++)
	{
		MOSI_OUT32(addr);
		HDEL();
		WBFLASH_CLK_HIGH;
		HDEL();
		WBFLASH_CLK_LOW;
		addr = addr<<1;
	}
	for (k=0;k<length;k++)
	{
		value = 0x00;
		for (i=0;i<8;i++)
		{
			HDEL();
			WBFLASH_CLK_HIGH;
			value = value<<1;
			if ((GPIOA->IDR&GPIO_Pin_6)==GPIO_Pin_6) value |= 0x01;
			HDEL();
			WBFLASH_CLK_LOW;
		}
		*(Data++)=value;
	}

	HDEL();
	WBFLASH_CS_HIGH;
}
*/
/*void SectorErase(uint32_t addr)
{
	int i;
	addr = addr<<8;
	uint8_t cmd = 0x20;

	WBFLASH_CS_LOW;
	HDEL();

	for (i=0;i<8;i++)
	{
		MOSI_OUT(cmd);
		HDEL();
		WBFLASH_CLK_HIGH;
		HDEL();
		WBFLASH_CLK_LOW;
		cmd = cmd<<1;
	}
	for (i=0;i<24;i++)
	{
		MOSI_OUT32(addr);
		HDEL();
		WBFLASH_CLK_HIGH;
		HDEL();
		WBFLASH_CLK_LOW;
		addr = addr<<1;
	}

	HDEL();
	WBFLASH_CS_HIGH;
}

void ProgramData(uint32_t addr,uint8_t value)
{
	int i;
	addr = addr<<8;
	uint8_t cmd = 0x02;

	WBFLASH_CS_LOW;
	HDEL();

	for (i=0;i<8;i++)
	{
		MOSI_OUT(cmd);
		HDEL();
		WBFLASH_CLK_HIGH;
		HDEL();
		WBFLASH_CLK_LOW;
		cmd = cmd<<1;
	}
	for (i=0;i<24;i++)
	{
		MOSI_OUT32(addr);
		HDEL();
		WBFLASH_CLK_HIGH;
		HDEL();
		WBFLASH_CLK_LOW;
		addr = addr<<1;
	}
	for (i=0;i<8;i++)
	{
		MOSI_OUT(value);
		HDEL();
		WBFLASH_CLK_HIGH;
		HDEL();
		WBFLASH_CLK_LOW;
		value = value<<1;
	}

	HDEL();
	WBFLASH_CS_HIGH;
}*/

void ProgramArray8(uint32_t addr, uint8_t value[8])
{
	int i,k;
	addr = addr<<8;
	uint8_t VL = 0x00;
	uint8_t cmd = 0x02;

	WBFLASH_CS_LOW;
	HDEL();

	for (i=0;i<8;i++)
	{
		MOSI_OUT(cmd);
		HDEL();
		WBFLASH_CLK_HIGH;
		HDEL();
		WBFLASH_CLK_LOW;
		cmd = cmd<<1;
	}
	for (i=0;i<24;i++)
	{
		MOSI_OUT32(addr);
		HDEL();
		WBFLASH_CLK_HIGH;
		HDEL();
		WBFLASH_CLK_LOW;
		addr = addr<<1;
	}
	for (k=0;k<8;k++)
	{
		VL = value[k];
		for (i=0;i<8;i++)
		{
			MOSI_OUT(VL);
			HDEL();
			WBFLASH_CLK_HIGH;
			HDEL();
			WBFLASH_CLK_LOW;
			VL = VL<<1;
		}
	}

	HDEL();
	WBFLASH_CS_HIGH;
}
