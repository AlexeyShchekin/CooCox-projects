// ILI9341_LowLevel.c

#include <stdint.h>

#include <stm32f10x.h>

#include <stm32f10x_gpio.h>
#include <stm32f10x_rcc.h>
#include <stm32f10x_spi.h>

#include "ILI9341_LowLevel_F10x.h"

void SPI_ON()
{
	GPIO_InitTypeDef gpio;
	SPI_InitTypeDef spi;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);

	gpio.GPIO_Pin = PIN_SCK | PIN_MISO | PIN_MOSI;	//SPI1 CLK, MOSI
	gpio.GPIO_Mode = GPIO_Mode_AF_PP;
	gpio.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &gpio);

	gpio.GPIO_Pin = PIN_CS;
	gpio.GPIO_Speed = GPIO_Speed_50MHz;
	gpio.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOA, &gpio );

	gpio.GPIO_Pin = PIN_DC | PIN_RST;
	gpio.GPIO_Speed = GPIO_Speed_50MHz;
	gpio.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOB, &gpio );

	SPI_Cmd (SPI1, DISABLE );
	spi.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
	spi.SPI_DataSize = SPI_DataSize_8b;
	spi.SPI_CPOL = SPI_CPOL_Low;
	spi.SPI_CPHA = SPI_CPHA_1Edge;
	spi.SPI_NSS = SPI_NSS_Soft;
	spi.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2;
	spi.SPI_FirstBit = SPI_FirstBit_MSB;
	spi.SPI_CRCPolynomial = 7;
	spi.SPI_Mode = SPI_Mode_Master;
	SPI_Init(SPI1, &spi);
	SPI_Cmd(SPI1, ENABLE);
	SPI_NSSInternalSoftwareConfig(SPI1, SPI_NSSInternalSoft_Set);
	SPI_CalculateCRC (SPI1, DISABLE );

	TFT_CS_HIGH ( );		// nCS - PC8 = 1
	TFT_DC_HIGH ( );		// DC  - PC7 = 1
	TFT_RST_HIGH ( );		// RST - PC5 = 1
}

void SPI_OFF()
{
	GPIO_InitTypeDef gpio;
	SPI_Cmd (SPI1, DISABLE );
	gpio.GPIO_Pin = PIN_DC | PIN_RST;
	gpio.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_Init(GPIOB, &gpio );
	gpio.GPIO_Pin = PIN_CS | PIN_SCK | PIN_MISO | PIN_MOSI;
	gpio.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_Init(GPIOA, &gpio );
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, DISABLE);
}

void Sensor_ON(void)
{
	GPIO_InitTypeDef gpio;

	gpio.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
	gpio.GPIO_Speed = GPIO_Speed_50MHz;
	gpio.GPIO_Mode = GPIO_Mode_Out_OD;
	GPIO_Init(GPIOB, &gpio );

	gpio.GPIO_Pin = GPIO_Pin_5;
	gpio.GPIO_Speed = GPIO_Speed_50MHz;
	gpio.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOB, &gpio );

	GPIOB->BSRR = GPIO_BSRR_BS5 | GPIO_BSRR_BS6 | GPIO_BSRR_BS7;
}

void Sensor_OFF(void)
{
	GPIO_InitTypeDef gpio;
	gpio.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;
	gpio.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_Init(GPIOB, &gpio );
}

void InitPins ( void )
{
	GPIO_InitTypeDef gpio;

	SPI_ON();

	gpio.GPIO_Pin = GPIO_Pin_15 | GPIO_Pin_12 | GPIO_Pin_13;
	gpio.GPIO_Speed = GPIO_Speed_50MHz;
	gpio.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOB, &gpio );

	GPIOB->BSRR = GPIO_BSRR_BS12 | GPIO_BSRR_BS13 | GPIO_BSRR_BS15;
}

uint8_t TFT_sendByte ( uint8_t data )
{
    u8 rxbyte;

    while ( !(TFT_SPI->SR & SPI_SR_TXE) );
    SPI1->DR = data;

    while ( !(TFT_SPI->SR & SPI_SR_RXNE) );
    rxbyte = TFT_SPI->DR;

    return rxbyte;
}

void TFT_sendWord ( uint16_t data )
{
    TFT_DC_HIGH ( );			// DC = 1 - data
    TFT_CS_LOW ( );

    TFT_sendByte ( data >> 8 );
    TFT_sendByte ( data & 0x00ff );

    TFT_CS_HIGH ( );
}

void TFT_sendCMD ( uint8_t index )
{
    TFT_DC_LOW ( );			// DC = 0 - command
    TFT_CS_LOW ( );

    TFT_sendByte ( index );

    TFT_CS_HIGH ( );
}

uint8_t TFT_sendDATA ( uint8_t data )
{
	uint8_t t;

    TFT_DC_HIGH ( );			// DC = 1 - data
    TFT_CS_LOW ( );

    t = TFT_sendByte ( data );

    TFT_CS_HIGH ( );

    return t;
}

uint8_t TFT_Read_Register ( uint8_t Addr, uint8_t xParameter )
{
	uint8_t data = 0;

    TFT_sendCMD ( 0xD9 );                                         // ext command
    TFT_sendByte ( 0x10 + xParameter );                           // 0x11 is the first Parameter

    TFT_DC_LOW ( );			// DC = 0
    TFT_CS_LOW ( );
    TFT_sendByte  ( Addr );
    data = TFT_sendByte ( 0xFF );
    TFT_DC_HIGH ( );

    TFT_CS_HIGH ();

    return data;
}

void TFT_CS_LOW ( void )
{
	GPIO_WriteBit (GPIOA, PIN_CS, Bit_RESET );
}

void TFT_CS_HIGH ( void )
{
	GPIO_WriteBit( GPIOA, PIN_CS, Bit_SET );
}

void TFT_DC_LOW ( void )
{
	GPIO_WriteBit ( GPIOB, PIN_DC, Bit_RESET );
}

void TFT_DC_HIGH ( void )
{
	GPIO_WriteBit ( GPIOB, PIN_DC, Bit_SET );
}

void TFT_RST_LOW ( void )
{
	GPIO_WriteBit ( GPIOB, PIN_RST, Bit_RESET );
}

void TFT_RST_HIGH ( void )
{
	GPIO_WriteBit ( GPIOB, PIN_RST, Bit_SET );
}
