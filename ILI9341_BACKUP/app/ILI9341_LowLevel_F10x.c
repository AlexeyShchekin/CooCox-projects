// ILI9341_LowLevel.c

#include <stdint.h>

#include <stm32f10x.h>

#include <stm32f10x_gpio.h>
#include <stm32f10x_rcc.h>
#include <stm32f10x_spi.h>

#include "ILI9341_LowLevel_F10x.h"

/*
 * SPI2 - F103
SCK  - PB13
MISO - PB14
MOSI - PB15

RES  - PB5
D/C  - PB6
CS   - PB12
*/

#define TFT_PORT				GPIOB
#define TFT_SPI					SPI2
#define TFT_SPI_RCC				RCC_APB1Periph_SPI2
#define TFT_SPI_PORT_RCC		RCC_APB2Periph_GPIOB

#define PIN_RST		GPIO_Pin_10
#define PIN_DC		GPIO_Pin_11
#define PIN_CS		GPIO_Pin_12

#define PIN_SCK		GPIO_Pin_13
#define PIN_MISO	GPIO_Pin_14
#define PIN_MOSI	GPIO_Pin_15


void InitPins ( void )
{
	GPIO_InitTypeDef gpio;
	SPI_InitTypeDef spi;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1 | RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);

	gpio.GPIO_Pin = GPIO_Pin_6;		// SPI1 MISO
	gpio.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &gpio);

	gpio.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_7;	//SPI1 CLK, MOSI
	gpio.GPIO_Mode = GPIO_Mode_AF_PP;
	gpio.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &gpio);

	spi.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
	spi.SPI_DataSize = SPI_DataSize_8b;
	spi.SPI_CPOL = SPI_CPOL_Low;
	spi.SPI_CPHA = SPI_CPHA_1Edge;
	spi.SPI_NSS = SPI_NSS_Soft;
	spi.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_256;
	spi.SPI_FirstBit = SPI_FirstBit_MSB;
	spi.SPI_Mode = SPI_Mode_Master;
	SPI_Init(SPI1, &spi);
	SPI_Cmd(SPI1, ENABLE);
	SPI_NSSInternalSoftwareConfig(SPI1, SPI_NSSInternalSoft_Set);

	RCC_APB2PeriphClockCmd ( TFT_SPI_PORT_RCC | RCC_APB2Periph_AFIO, ENABLE );

    gpio.GPIO_Pin = PIN_DC | PIN_RST | PIN_CS | GPIO_Pin_8 | GPIO_Pin_9;
	gpio.GPIO_Speed = GPIO_Speed_50MHz;
    gpio.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init ( TFT_PORT, &gpio );

    TFT_CS_HIGH ( );		// nCS - PC8 = 1
    TFT_DC_HIGH ( );		// DC  - PC7 = 1
    TFT_RST_HIGH ( );		// RST - PC5 = 1

	//////////////////////////////////////////////////////
	// SPI
	RCC_APB1PeriphClockCmd ( TFT_SPI_RCC, ENABLE );

	gpio.GPIO_Pin = PIN_SCK | PIN_MOSI | PIN_MISO;
	gpio.GPIO_Speed = GPIO_Speed_50MHz;
	gpio.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init ( GPIOB, &gpio );

	SPI_Cmd ( TFT_SPI, DISABLE );
	// SPI config (max 16 MHz ןמ ÄØ ILI9341)
	spi.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
	spi.SPI_Mode = SPI_Mode_Master;
	spi.SPI_DataSize = SPI_DataSize_8b;
	spi.SPI_CPOL = SPI_CPOL_Low;
	spi.SPI_CPHA = SPI_CPHA_1Edge;
	spi.SPI_NSS = SPI_NSS_Soft;
//	spi.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_16;	// 72/16 = 4.5 MHz
	spi.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2;
	spi.SPI_FirstBit = SPI_FirstBit_MSB;
	spi.SPI_CRCPolynomial = 7;
	SPI_Init ( TFT_SPI, &spi );

	SPI_Cmd ( TFT_SPI, ENABLE );
	SPI_CalculateCRC ( TFT_SPI, DISABLE );
	///////////////////////////////

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);

	TIM4->PSC = SystemCoreClock/(2*32768) - 1;
	TIM4->ARR = 1;
	TIM4->DIER |= TIM_DIER_UIE;
	TIM4->CR1 |= TIM_CR1_CEN;

	NVIC_EnableIRQ(TIM4_IRQn);
} // InitPins

void TIM4_IRQHandler()
{
	GPIOB->ODR^=GPIO_Pin_9;
	TIM_ClearITPendingBit(TIM4, TIM_IT_Update);
}


uint8_t TFT_sendByte ( uint8_t data )
{
    u8 rxbyte;

    while ( !(TFT_SPI->SR & SPI_SR_TXE) );
    SPI2->DR = data;

    while ( !(TFT_SPI->SR & SPI_SR_RXNE) );
    rxbyte = TFT_SPI->DR;

    return rxbyte;

/*
	int b, d;

    for(b=0; b<8; b++)
    {
    	d = (data >> (7 - b)) & 0x0001;
        if (d) {
			GPIO_WriteBit(TFT_PORT, PIN_MOSI, Bit_SET);
        }
        else {
			GPIO_WriteBit(TFT_PORT, PIN_MOSI, Bit_RESET);
        }

		GPIO_WriteBit(TFT_PORT, PIN_SCK, Bit_RESET);
        GPIO_WriteBit(TFT_PORT, PIN_SCK, Bit_SET);
    }

    return 0;
*/
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
	GPIO_WriteBit (TFT_PORT, PIN_CS, Bit_RESET );
}

void TFT_CS_HIGH ( void )
{
	GPIO_WriteBit( TFT_PORT, PIN_CS, Bit_SET );
}

void TFT_DC_LOW ( void )
{
	GPIO_WriteBit ( TFT_PORT, PIN_DC, Bit_RESET );
}

void TFT_DC_HIGH ( void )
{
	GPIO_WriteBit ( TFT_PORT, PIN_DC, Bit_SET );
}

void TFT_RST_LOW ( void )
{
	GPIO_WriteBit ( TFT_PORT, PIN_RST, Bit_RESET );
}

void TFT_RST_HIGH ( void )
{
	GPIO_WriteBit ( TFT_PORT, PIN_RST, Bit_SET );
}
