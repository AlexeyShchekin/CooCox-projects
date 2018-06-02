#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stm32f10x_spi.h>

#include "sys_init.h"
#include "fontStructures.h"
#include "ILI9341_LowLevel_F10x.h"
#include "ili9341.h"

#define TFT_MIN_X	0
#define TFT_MIN_Y	0
#define TFT_MAX_X	319
#define TFT_MAX_Y	239

extern const FONT_INFO lucidaConsole12ptFontInfo;
const FONT_INFO *font = NULL;

static void TFT_setCol ( uint16_t StartCol, uint16_t EndCol);
static void TFT_setPage ( uint16_t StartPage, uint16_t EndPage );

void ili9341_init ( void )
{
    InitPins();

    TFT_CS_HIGH ();
    TFT_DC_HIGH ();

	TFT_RST_LOW ();
	Delay_ms ( 10 );
	TFT_RST_HIGH ();

    TFT_sendCMD(0x01); //software reset
    Delay_ms(5);
    TFT_sendCMD(0x28); // display off
    //---------------------------------------------------------
    TFT_sendCMD(0xcf);
    TFT_sendDATA(0x00);
    TFT_sendDATA(0x83);
    TFT_sendDATA(0x30);

    TFT_sendCMD(0xed);
    TFT_sendDATA(0x64);
    TFT_sendDATA(0x03);
    TFT_sendDATA(0x12);
    TFT_sendDATA(0x81);

    TFT_sendCMD(0xe8);
    TFT_sendDATA(0x85);
    TFT_sendDATA(0x01);
    TFT_sendDATA(0x79);

    TFT_sendCMD(0xcb);
    TFT_sendDATA(0x39);
    TFT_sendDATA(0x2c);
    TFT_sendDATA(0x00);
    TFT_sendDATA(0x34);
    TFT_sendDATA(0x02);

    TFT_sendCMD(0xf7);
    TFT_sendDATA(0x20);
    TFT_sendCMD(0xea);
    TFT_sendDATA(0x00);
    TFT_sendDATA(0x00);

    //------------power control------------------------------
    TFT_sendCMD(0xc0); //power control
    TFT_sendDATA(0x26);
    TFT_sendCMD(0xc1); //power control
    TFT_sendDATA(0x11);

    //--------------VCOM???????????????????? ---------
    TFT_sendCMD(0xc5); //vcom control
    TFT_sendDATA(0x35);//35
    TFT_sendDATA(0x3e);//3E
    TFT_sendCMD(0xc7); //vcom control
    TFT_sendDATA(0xbe); // 0x94

    //------------memory access control------------------------
    TFT_sendCMD(0x36); // memory access control
    TFT_sendDATA(0xE8); //0048 my,mx,mv,ml,BGR,mh,0.0 => 0x24
    TFT_sendCMD(0x3a); // pixel format set
    TFT_sendDATA(0x55);//16bit /pixel

    //----------------- frame rate------------------------------
    TFT_sendCMD(0xb1); // frame rate
    TFT_sendDATA(0x00);
    TFT_sendDATA(0x1B); //70

    //----------------Gamma---------------------------------
    TFT_sendCMD(0xf2); // 3Gamma Function Disable
    TFT_sendDATA(0x08);

    TFT_sendCMD(0x26);
    TFT_sendDATA(0x01); // gamma set 4 gamma curve 01/02/04/08

    TFT_sendCMD(0xE0); //positive gamma correction
    TFT_sendDATA(0x1f);
    TFT_sendDATA(0x1a);
    TFT_sendDATA(0x18);
    TFT_sendDATA(0x0a);
    TFT_sendDATA(0x0f);
    TFT_sendDATA(0x06);
    TFT_sendDATA(0x45);
    TFT_sendDATA(0x87);
    TFT_sendDATA(0x32);
    TFT_sendDATA(0x0a);
    TFT_sendDATA(0x07);
    TFT_sendDATA(0x02);
    TFT_sendDATA(0x07);
    TFT_sendDATA(0x05);
    TFT_sendDATA(0x00);

    TFT_sendCMD(0xE1); //negamma correction
    TFT_sendDATA(0x00);
    TFT_sendDATA(0x25);
    TFT_sendDATA(0x27);
    TFT_sendDATA(0x05);
    TFT_sendDATA(0x10);
    TFT_sendDATA(0x09);
    TFT_sendDATA(0x3a);
    TFT_sendDATA(0x78);
    TFT_sendDATA(0x4d);
    TFT_sendDATA(0x05);
    TFT_sendDATA(0x18);
    TFT_sendDATA(0x0d);
    TFT_sendDATA(0x38);
    TFT_sendDATA(0x3a);
    TFT_sendDATA(0x1f);

	//--------------ddram ---------------------
    TFT_sendCMD(0x2a); // column set
    TFT_sendDATA(0x00);
    TFT_sendDATA(0x00);
    TFT_sendDATA(0x00);
    TFT_sendDATA(0xEF);

    TFT_sendCMD(0x2b); // page address set
    TFT_sendDATA(0x00);
    TFT_sendDATA(0x00);
    TFT_sendDATA(0x01);
    TFT_sendDATA(0x3F);

    //TFT_sendCMD(0x34); // tearing effect off
    TFT_sendCMD(0x35); // tearing effect on
    //TFT_sendCMD(0xb4); // display inversion
    //TFT_sendDATA16(0x00,0x00);

    TFT_sendCMD(0xb7); //entry mode set
    TFT_sendDATA(0x07);

    //-----------------display---------------------
    TFT_sendCMD(0xb6); // display function control
    TFT_sendDATA(0x0a);
    TFT_sendDATA(0x82);
    TFT_sendDATA(0x27);
    TFT_sendDATA(0x00);

    TFT_sendCMD(0x11); //sleep out
    Delay_ms(100);

    TFT_sendCMD(0x29); // display on
    Delay_ms(100);

	TFT_DMA_fillRect ( TFT_MIN_X, TFT_MIN_Y, TFT_MAX_X, TFT_MAX_Y, BLACK );

	font = &lucidaConsole12ptFontInfo;

	//SPI_OFF();
}

static void TFT_setCol ( uint16_t StartCol, uint16_t EndCol )
{
	TFT_DC_LOW ( );			// DC = 1 - data
	TFT_CS_LOW ( );
	TFT_sendByte ( 0x2A );                                                      // Column Command address

	TFT_DC_HIGH ( );			// DC = 1 - data

	TFT_sendByte ( StartCol >> 8 );
	TFT_sendByte ( StartCol & 0x00ff );
	TFT_sendByte ( EndCol >> 8 );
	TFT_sendByte ( EndCol & 0x00ff );

	TFT_CS_HIGH ( );
}

static void TFT_setPage ( uint16_t StartPage, uint16_t EndPage )
{
	TFT_DC_LOW ( );			// DC = 1 - data
	TFT_CS_LOW ( );
	TFT_sendByte ( 0x2B );                                                      // Column Command address

	TFT_DC_HIGH ( );			// DC = 1 - data

	TFT_sendByte ( StartPage >> 8 );
	TFT_sendByte ( StartPage & 0x00ff );
	TFT_sendByte ( EndPage >> 8 );
	TFT_sendByte ( EndPage & 0x00ff );

	TFT_CS_HIGH ( );
}

void TFT_setXY ( uint16_t poX, uint16_t poY )
{
	TFT_setCol ( poX, poX  );
	TFT_setPage ( poY, poY );
	TFT_sendCMD ( 0x2c );
}

int TFT_rgb2color ( int R, int G, int B )
{
	return ((R & 0xF8) << 8) | ((G & 0xFC) << 3) | (B >> 3);
}

void TFT_setPixel ( uint16_t poX, uint16_t poY, uint16_t color )
{
	TFT_setXY ( poX, poY );
	TFT_sendWord ( color );
}

void TFT_DMA_drawRect ( uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color )
{
	TFT_DMA_drawHorizontalLine(x1,x2,y1,color);
	TFT_DMA_drawHorizontalLine(x1,x2,y2,color);
	TFT_DMA_drawVerticalLine(x1,y1,y2,color);
	TFT_DMA_drawVerticalLine(x2,y1,y2,color);
}

void TFT_fillRect ( uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color )
{
	uint32_t i;
	uint32_t pNum;
    uint8_t hi, lo;

    if ( x1 > x2 )
    { // swap
    	i = x1;
        x1 = x2;
        x2 = i;
    }

    if ( y1 > y2 )
    { // swap
    	i = y1;
        y1 = y2;
        y2 = i;
    }

    pNum = x2 - x1 + 1;
    pNum = pNum * ( y2 - y1 + 1 );

    TFT_setCol ( x1, x2 );
    TFT_setPage ( y1, y2 );
    TFT_sendCMD ( 0x2c );              // start to write to display ram

    TFT_DC_HIGH ( );			// DC = 1 - data
    TFT_CS_LOW ( );

    hi = color >> 8;
    lo = color & 0xff;

    for ( i = 0; i < pNum; i ++ )
    {
    	TFT_sendByte ( hi );
    	TFT_sendByte ( lo );
    }

    TFT_CS_HIGH ( );
}

void TFT_drawLine ( uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
{
	int deltaX, deltaY, signX, signY, error, error2;
	int dX, dY;

  if ( x1 == x2 )
  {
    TFT_DMA_drawVerticalLine(x1,y1,y2,color);
    return;
  }

  if ( y1 == y2 )
  {
    TFT_DMA_drawHorizontalLine(x1,x2,y1,color);
    return;
  }

  dX = x2 - x1;
  dY = y2 - y1;
  deltaX = abs(dX);
  deltaY = abs(dY);
  signX = (x1 < x2) ? 1 : -1;
  signY = (y1 < y2) ? 1 : -1;
  error = deltaX - deltaY;

  TFT_setPixel ( x2, y2, color );
  while ( x1 != x2 || y1 != y2 )
  {
	  TFT_setPixel ( x1, y1, color );
	  error2 = error * 2;

    if ( error2 > -deltaY )
    {
      error -= deltaY;
      x1 += signX;
    } // if

    if ( error2 < deltaX )
    {
      error += deltaX;
      y1 += signY;
    } // if
  } // while
} // TFT_drawLine

uint16_t TFT_DrawChar ( uint16_t x, uint16_t y, uint8_t c, uint16_t textColor,
		uint16_t backColor, uint8_t size)
{
	uint8_t i, j, ks, ms;
	uint8_t hi_t, hi_b, lo_t, lo_b;
	uint32_t mask, mask1, b, t, height;
	uint8_t widthBits;
	uint8_t widthBytes;
	uint8_t charIdx;
	const uint8_t *ptr;

	if ( !font )
		return 0;

	if ( c < font->FirstChar || c > font->LastChar )
		return 0;

	charIdx = c - font->FirstChar;

	widthBits = font->FontTable[charIdx].widthBits;
	if ( widthBits > 32 )
		return 0;

	if ( widthBits % 8 == 0 )
		widthBytes = widthBits / 8;
	else
		widthBytes = widthBits / 8 + 1;

	height = font->Height;

	ptr = font -> FontBitmaps + font -> FontTable[charIdx].start;

	hi_t = textColor >> 8;
	lo_t = textColor & 0xff;
	hi_b = backColor >> 8;
	lo_b = backColor & 0xff;
	TFT_setCol ( x, x + size*(widthBits + font->FontSpace)-1 );
	TFT_setPage ( y, y + size*height );
	TFT_sendCMD ( 0x2c );              // start to write to display ram

	TFT_DC_HIGH ( );			// DC = 1 - data
	TFT_CS_LOW ( );

	for ( i = 0; i < height; i ++ )
	{

		switch ( widthBytes )
		{
			case 1:
				mask1 = 0x00000080;
				b = *ptr;
				ptr ++;
				break;

			case 2:
				mask1 = 0x00008000;
				b = *ptr;
				ptr ++;
				b <<= 8;
				j = *ptr;
				ptr ++;
				b |= j;
				break;

			case 3:
				mask1 = 0x00800000;
				b = *ptr;
				ptr ++;
				b <<= 8;
				j = *ptr;
				ptr ++;
				b |= j;
				b <<= 8;
				j = *ptr;
				ptr ++;
				b |= j;
				break;

			case 4:
				mask1 = 0x80000000;
				b = *ptr;
				ptr ++;
				b <<= 8;
				j = *ptr;
				ptr ++;
				b |= j;
				b <<= 8;
				j = *ptr;
				ptr ++;
				b |= j;
				b <<= 8;
				j = *ptr;
				ptr ++;
				b |= j;
				break;
		} // swich

		for (ms=0; ms<size; ms++)
		{
			mask = mask1;
			for ( j = 0; j < widthBits; j ++ )
			{
				t = b & mask;

				if ( t != 0 )
				{
					for (ks=0; ks<size; ks++)
					{
						TFT_sendByte ( hi_t );
						TFT_sendByte ( lo_t );
					}
				}
				else
				{
					for (ks=0; ks<size; ks++)
					{
						TFT_sendByte ( hi_b );
						TFT_sendByte ( lo_b );
					}
				}

				mask >>= 1;
			}

			for ( j = 0; j < font->FontSpace; j ++ )
			{
				for (ks=0; ks<size; ks++)
				{
					TFT_sendByte ( hi_b );
					TFT_sendByte ( lo_b );
				}
			}
		}
	}

	TFT_CS_HIGH ( );

	return size*(widthBits + font->FontSpace);
}

void TFT_DrawString ( char* s, int len, uint16_t x, uint16_t y,
		uint16_t color, uint16_t backColor, uint8_t size)
{
	int i;//, len = strlen(s);

	for ( i = 0; i < len; i ++ )
		x += TFT_DrawChar ( x, y, s[i], color, backColor, size);
}

void TFT_DMA_fillRect ( uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color )
{
	uint32_t i;
	uint32_t pNum;
	uint8_t REQUIRED_REPEAT_DMA;

	if ( x1 > x2 )
	{ // swap
		i = x1;
		x1 = x2;
		x2 = i;
	}

	if ( y1 > y2 )
	{ // swap
		i = y1;
		y1 = y2;
		y2 = i;
	}

	pNum = x2 - x1 + 1;
	pNum = pNum * ( y2 - y1 + 1 );

	if (pNum > 65536)
	{
		pNum = 1+(uint32_t)(pNum/2);
		REQUIRED_REPEAT_DMA = 2;
	}
	else REQUIRED_REPEAT_DMA = 1;

	DMA_InitTypeDef DMA_INI;

	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
	DMA_DeInit(DMA1_Channel3);
	DMA_INI.DMA_PeripheralBaseAddr = (uint32_t)&(SPI1->DR);
	DMA_INI.DMA_MemoryBaseAddr = (uint32_t)&color;
	DMA_INI.DMA_DIR = DMA_DIR_PeripheralDST;
	DMA_INI.DMA_BufferSize = pNum;
	DMA_INI.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_INI.DMA_MemoryInc = DMA_MemoryInc_Disable;
	DMA_INI.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
	DMA_INI.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
	DMA_INI.DMA_Mode = DMA_Mode_Circular;
	DMA_INI.DMA_Priority = DMA_Priority_VeryHigh;
	DMA_INI.DMA_M2M = DMA_M2M_Disable;
	DMA_Init(DMA1_Channel3, &DMA_INI);

    TFT_setCol ( x1, x2 );
    TFT_setPage ( y1, y2 );
    TFT_sendCMD ( 0x2c );

    SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Tx, ENABLE);
    //SPI_I2S_ITConfig(SPI1,SPI_I2S_IT_TXE,ENABLE);
    SPI_DataSizeConfig(SPI1, SPI_DataSize_16b);

    TFT_DC_HIGH ( );			// DC = 1 - data
    TFT_CS_LOW ( );

   // DMA_ITConfig(DMA1_Channel3, DMA_IT_TC, ENABLE);
   // NVIC_EnableIRQ(DMA1_Channel3_IRQn);
   // NVIC_EnableIRQ(SPI1_IRQn);

	DMA_Cmd(DMA1_Channel3, ENABLE);

	for (i=0;i<REQUIRED_REPEAT_DMA;i++)
	{
		while(!DMA_GetFlagStatus(DMA1_FLAG_TC3));
		DMA1->IFCR = DMA1_FLAG_TC3;
	}

	DMA_Cmd(DMA1_Channel3, DISABLE);
	SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Tx, DISABLE);
	SPI_DataSizeConfig(SPI1, SPI_DataSize_8b);

	TFT_CS_HIGH();
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, DISABLE);

} // TFT_fillRect

void TFT_DMA_drawHorizontalLine(uint16_t x0, uint16_t x1, uint16_t y, uint16_t color)
{
	uint32_t pNum;

	pNum = x1 - x0 + 1;

	uint8_t CM[2*pNum];
	int i=0;
	for (i=0; i<pNum; i++)
	{
		CM[2*i] = (uint8_t)(color>>8);
		CM[2*i+1] = (uint8_t)(color&0x00FF);
	}

	DMA_InitTypeDef DMA_INI;

	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
	DMA_DeInit(DMA1_Channel3);
	DMA_INI.DMA_PeripheralBaseAddr = (uint32_t)&(TFT_SPI->DR);
	DMA_INI.DMA_MemoryBaseAddr = (uint32_t)&CM[0];
	DMA_INI.DMA_DIR = DMA_DIR_PeripheralDST;
	DMA_INI.DMA_BufferSize = 2*pNum;
	DMA_INI.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
	DMA_INI.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
	DMA_INI.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_INI.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_INI.DMA_Mode = DMA_Mode_Normal;
	DMA_INI.DMA_Priority = DMA_Priority_VeryHigh;
	DMA_INI.DMA_M2M = DMA_M2M_Disable;
	DMA_Init(DMA1_Channel3, &DMA_INI);

	TFT_setCol ( x0, x1 );
	TFT_setPage ( y, y );
	TFT_sendCMD ( 0x2c );              // start to write to display ram

	SPI_I2S_DMACmd(TFT_SPI, SPI_I2S_DMAReq_Tx, ENABLE);
	//SPI_I2S_ITConfig(SPI1,SPI_I2S_IT_TXE,ENABLE);
	//SPI_DataSizeConfig(TFT_SPI, SPI_DataSize_16b);

	TFT_DC_HIGH ( );			// DC = 1 - data
	TFT_CS_LOW ( );

	//DMA_ITConfig(DMA1_Channel3, DMA_IT_TC, ENABLE);
	//NVIC_EnableIRQ(DMA1_Channel3_IRQn);
	//NVIC_EnableIRQ(SPI1_IRQn);

	DMA_Cmd(DMA1_Channel3, ENABLE);

	while(!DMA_GetFlagStatus(DMA1_FLAG_TC3));
	DMA1->IFCR = DMA1_FLAG_TC3;

	DMA_Cmd(DMA1_Channel3, DISABLE);
	SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Tx, DISABLE);
	SPI_DataSizeConfig(SPI1, SPI_DataSize_8b);

	TFT_CS_HIGH();
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, DISABLE);
}

void TFT_DMA_drawVerticalLine(uint16_t x, uint16_t y0, uint16_t y1, uint16_t color)
{
	uint32_t pNum;

	pNum = y1 - y0 + 1 ;

	DMA_InitTypeDef DMA_INI;

	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
	DMA_DeInit(DMA1_Channel3);
	DMA_INI.DMA_PeripheralBaseAddr = (uint32_t)&(TFT_SPI->DR);
	DMA_INI.DMA_MemoryBaseAddr = (uint32_t)&color;
	DMA_INI.DMA_DIR = DMA_DIR_PeripheralDST;
	DMA_INI.DMA_BufferSize = pNum;
	DMA_INI.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_INI.DMA_MemoryInc = DMA_MemoryInc_Disable;
	DMA_INI.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
	DMA_INI.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
	DMA_INI.DMA_Mode = DMA_Mode_Circular;
	DMA_INI.DMA_Priority = DMA_Priority_VeryHigh;
	DMA_INI.DMA_M2M = DMA_M2M_Disable;
	DMA_Init(DMA1_Channel3, &DMA_INI);

	TFT_setCol ( x, x );
	TFT_setPage ( y0, y1 );
	TFT_sendCMD ( 0x2c );              // start to write to display ram

	SPI_I2S_DMACmd(TFT_SPI, SPI_I2S_DMAReq_Tx, ENABLE);
	//SPI_I2S_ITConfig(SPI1,SPI_I2S_IT_TXE,ENABLE);
	SPI_DataSizeConfig(TFT_SPI, SPI_DataSize_16b);

	TFT_DC_HIGH ( );			// DC = 1 - data
	TFT_CS_LOW ( );

	//DMA_ITConfig(DMA1_Channel3, DMA_IT_TC, ENABLE);
	//NVIC_EnableIRQ(DMA1_Channel3_IRQn);
	//NVIC_EnableIRQ(SPI1_IRQn);

	DMA_Cmd(DMA1_Channel3, ENABLE);

	while(!DMA_GetFlagStatus(DMA1_FLAG_TC3));

	DMA_Cmd(DMA1_Channel3, DISABLE);
	SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Tx, DISABLE);
	SPI_DataSizeConfig(SPI1, SPI_DataSize_8b);

	TFT_CS_HIGH();
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, DISABLE);
}

uint16_t TFT_DMA_DrawChar ( uint16_t x, uint16_t y, uint8_t c, uint16_t textColor,
		uint16_t backColor, uint8_t size)
{

	DMA_InitTypeDef DMA_INI;
	uint8_t i, j, ks, ms;
	uint32_t mask, mask1, b, height;
	uint8_t widthBits;
	uint8_t widthBytes;
	uint8_t charIdx;
	const uint8_t *ptr;

	if ( !font )
		return 0;

	if ( c < font->FirstChar || c > font->LastChar )
		return 0;

	charIdx = c - font->FirstChar;

	widthBits = font->FontTable[charIdx].widthBits;
	if ( widthBits > 32 )
		return 0;

	if ( widthBits % 8 == 0 )
		widthBytes = widthBits / 8;
	else
		widthBytes = widthBits / 8 + 1;

	height = font->Height;

	ptr = font -> FontBitmaps + font -> FontTable[charIdx].start;

	uint32_t pNum = size*(widthBits + font->FontSpace);
	uint16_t ColorMap[pNum];
	for (i=0;i<pNum;i++) ColorMap[i] = backColor;
	uint32_t PixCounter = 0;

	for ( i = 0; i < height; i ++ )
	{
		switch ( widthBytes )
		{
			case 1:
				mask1 = 0x00000080;
				b = *ptr;
				ptr ++;
				break;

			case 2:
				mask1 = 0x00008000;
				b = *ptr;
				ptr ++;
				b <<= 8;
				j = *ptr;
				ptr ++;
				b |= j;
				break;

			case 3:
				mask1 = 0x00800000;
				b = *ptr;
				ptr ++;
				b <<= 8;
				j = *ptr;
				ptr ++;
				b |= j;
				b <<= 8;
				j = *ptr;
				ptr ++;
				b |= j;
				break;

			case 4:
				mask1 = 0x80000000;
				b = *ptr;
				ptr ++;
				b <<= 8;
				j = *ptr;
				ptr ++;
				b |= j;
				b <<= 8;
				j = *ptr;
				ptr ++;
				b |= j;
				b <<= 8;
				j = *ptr;
				ptr ++;
				b |= j;
				break;
		}

		for (ms=0; ms<size; ms++)
		{
			PixCounter = 0;
			mask = mask1;
			for ( j = 0; j < widthBits; j ++ )
			{
				if ( (b & mask) != 0 )
				{
					for (ks=0; ks<size; ks++)
					{
						ColorMap[PixCounter] = textColor;
						PixCounter++;
					}
				}
				else
				{
						for (ks=0; ks<size; ks++)
						{
							ColorMap[PixCounter] = backColor;
							PixCounter++;
						}
				}

				mask >>= 1;
			}

			for ( j = 0; j < font->FontSpace; j ++ )
			{
					for (ks=0; ks<size; ks++)
					{
						ColorMap[PixCounter] = backColor;
						PixCounter++;
					}
			}

			RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
			DMA_DeInit(DMA1_Channel3);
			DMA_INI.DMA_PeripheralBaseAddr = (uint32_t)&(TFT_SPI->DR);
			DMA_INI.DMA_MemoryBaseAddr = (uint32_t)&(ColorMap[0]);
			DMA_INI.DMA_DIR = DMA_DIR_PeripheralDST;
			DMA_INI.DMA_BufferSize = pNum;
			DMA_INI.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
			DMA_INI.DMA_MemoryInc = DMA_MemoryInc_Enable;
			DMA_INI.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
			DMA_INI.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
			DMA_INI.DMA_Mode = DMA_Mode_Normal;
			DMA_INI.DMA_Priority = DMA_Priority_VeryHigh;
			DMA_INI.DMA_M2M = DMA_M2M_Disable;
			DMA_Init(DMA1_Channel3, &DMA_INI);

			TFT_setCol ( x, x + size*(widthBits + font->FontSpace)-1 );
			TFT_setPage ( y+size*i+ms, y+size*i+ms);
			TFT_sendCMD ( 0x2c );

			SPI_I2S_DMACmd(TFT_SPI, SPI_I2S_DMAReq_Tx, ENABLE);
			//SPI_I2S_ITConfig(SPI1,SPI_I2S_IT_TXE,ENABLE);
			SPI_DataSizeConfig(TFT_SPI, SPI_DataSize_16b);

			TFT_DC_HIGH ( );			// DC = 1 - data
			TFT_CS_LOW ( );

			//DMA_ITConfig(DMA1_Channel3, DMA_IT_TC, ENABLE);
			//NVIC_EnableIRQ(DMA1_Channel3_IRQn);
			//NVIC_EnableIRQ(SPI1_IRQn);

			DMA_Cmd(DMA1_Channel3, ENABLE);

			while(!DMA_GetFlagStatus(DMA1_FLAG_TC3));
			DMA1->IFCR = DMA1_FLAG_TC3;

			DMA_Cmd(DMA1_Channel3, DISABLE);
			SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Tx, DISABLE);
			SPI_DataSizeConfig(SPI1, SPI_DataSize_8b);

			TFT_CS_HIGH();
			RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, DISABLE);
		}
	}

	return size*(widthBits + font->FontSpace);
}

void TFT_DMA_DrawString ( char* s, int len, uint16_t x, uint16_t y,
		uint16_t color, uint16_t backColor, uint8_t size)
{
	int i;//, len = strlen(s);

	for ( i = 0; i < len; i ++ )
		x += TFT_DMA_DrawChar ( x, y, s[i], color, backColor, size);
}
