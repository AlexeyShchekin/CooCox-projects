// ili9341.c

// 4-line SPI mode - D/nC, MISO/MOSI/SCK
// p. 64 .pdf
// Запись и выдача по нарастанию SCK -

// http://chipspace.ru/stm32-spi-stdperiph_lib/

// http://www.gaw.ru/html.cgi/txt/interface/spi
// http://radiohlam.ru/teory/SPI.htm

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stm32f10x_spi.h>

#include "sys_init.h"
#include "fontStructures.h"
#include "ILI9341_LowLevel_F10x.h"
#include "ili9341.h"

//TFT resolution 240*320
#define TFT_MIN_X	0
#define TFT_MIN_Y	0
#define TFT_MAX_X	319
#define TFT_MAX_Y	239

// Таблицы шрифта
extern const FONT_INFO lucidaConsole12ptFontInfo;
const FONT_INFO *font = NULL;

static void TFT_setCol ( uint16_t StartCol, uint16_t EndCol);
static void TFT_setPage ( uint16_t StartPage, uint16_t EndPage );

// Инициализация
void ili9341_init ( void )
{
    // инициализация SPI и пинов
    InitPins();

	////////////////////////////////////////////////////////
	// ILI init
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

    TFT_sendCMD(0x34); // tearing effect off
    //TFT_sendCMD(0x35); // tearing effect on
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

//	TFT_clearScreen ( );
	TFT_fillRect ( TFT_MIN_X, TFT_MIN_Y, TFT_MAX_X, TFT_MAX_Y, BLACK );

	// Шрифт
	font = &lucidaConsole12ptFontInfo;
} // ili9341_init

static void TFT_setCol ( uint16_t StartCol, uint16_t EndCol )
{
//	uint8_t lo, hi;

	TFT_sendCMD ( 0x2A );                                                      // Column Command address
	TFT_sendWord ( StartCol );
	TFT_sendWord ( EndCol );
}

static void TFT_setPage ( uint16_t StartPage, uint16_t EndPage )
{
	TFT_sendCMD ( 0x2B );                                                      // Column Command address

	TFT_sendWord ( StartPage );
	TFT_sendWord ( EndPage );
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

void TFT_drawHorizontalLine(uint16_t x0, uint16_t x1, uint16_t y, uint16_t color)
{
	uint32_t i;
	uint32_t pNum;
	uint8_t hi, lo;

	pNum = x1 - x0 + 1 ;

	TFT_setCol ( x0, x1 );
	TFT_setPage ( y, y );
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

void TFT_drawVerticalLine(uint16_t x, uint16_t y0, uint16_t y1, uint16_t color)
{
	uint32_t i;
	uint32_t pNum;
	uint8_t hi, lo;

	pNum = y1 - y0 + 1 ;

	TFT_setCol ( x, x );
	TFT_setPage ( y0, y1 );
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

void TFT_drawRect ( uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color )
{
	TFT_drawHorizontalLine(x1,x2,y1,color);
	TFT_drawHorizontalLine(x1,x2,y2,color);
	TFT_drawVerticalLine(x1,y1,y2,color);
	TFT_drawVerticalLine(x2,y1,y2,color);
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
} // TFT_fillRect

// Рисование линии
void TFT_drawLine ( uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
{
	int deltaX, deltaY, signX, signY, error, error2;
	int dX, dY;

  if ( x1 == x2 )
  { // Вертикальная линия
    TFT_drawVerticalLine(x1,y1,y2,color);
    return;
  } // if

  if ( y1 == y2 )
  { // Гoризонтальная линия
    TFT_drawHorizontalLine(x1,x2,y1,color);
    return;
  } // if

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

//////////////////////////////////////
// Строки и символы

// Рисование одного символа (альбомная ориентация)
uint16_t TFT_DrawChar ( uint16_t x, uint16_t y, uint8_t c, uint16_t textColor,
		uint16_t backColor, uint8_t size)
{
	uint8_t i, j, ks, ms;
	uint8_t hi_t, hi_b, lo_t, lo_b;
	uint32_t mask, mask1, b, t, height;
	uint8_t widthBits;		// ширина символа в битах
	uint8_t widthBytes;		// ширина символа в байтах
	uint8_t charIdx;
	const uint8_t *ptr;

	// Если шрифт для вывода не определен выход
	if ( !font )
		return 0;

	// Если символ не лежит в пределах символов для шрифта то выход
	if ( c < font->FirstChar || c > font->LastChar )
		return 0;

	// Индекс описателей символа в таблицах
	charIdx = c - font->FirstChar;

	// Ширина символа в битах
	widthBits = font->FontTable[charIdx].widthBits;
	if ( widthBits > 32 )	// символы более 32 точек в ширину не поддерживаются
		return 0;

	// Ширина символа в байтах
	if ( widthBits % 8 == 0 )
		widthBytes = widthBits / 8;
	else
		widthBytes = widthBits / 8 + 1;

	// Высота символа
	height = font->Height;

	// Указатель на начало описания символа
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

	// цикл по высоте символа
	for ( i = 0; i < height; i ++ )
	{

		// Формирование начальной маски и слова-описателя строки пикселей
		switch ( widthBytes )
		{
			case 1:
				// начальная маска
				mask1 = 0x00000080;
				// Формирование описателя строки символа
				b = *ptr;       // старший байт
				ptr ++;
				break;

			case 2:
				// начальная маска
				mask1 = 0x00008000;
				// Формирование описателя строки символа
				b = *ptr;       // старший байт
				ptr ++;
				b <<= 8;
				j = *ptr;    // младший байт
				ptr ++;
				b |= j;
				break;

			case 3:
				// начальная маска
				mask1 = 0x00800000;
				// Формирование описателя строки символа
				b = *ptr;       // старший байт
				ptr ++;
				b <<= 8;
				j = *ptr;		// средний байт
				ptr ++;
				b |= j;
				b <<= 8;
				j = *ptr;		// младший байт
				ptr ++;
				b |= j;
				break;

			case 4:
				// начальная маска
				mask1 = 0x80000000;
				// Формирование описателя строки символа
				b = *ptr;       // старший байт
				ptr ++;
				b <<= 8;
				j = *ptr;		// средний байт 1
				ptr ++;
				b |= j;
				b <<= 8;
				j = *ptr;		// средний байт 2
				ptr ++;
				b |= j;
				b <<= 8;
				j = *ptr;		// младший байт
				ptr ++;
				b |= j;
				break;
		} // swich

		for (ms=0; ms<size; ms++)
		{
			mask = mask1;
			// Вывод строки пикселей
			for ( j = 0; j < widthBits; j ++ )
			{
				t = b & mask;

				if ( t != 0 )
				{ // пиксель есть
					for (ks=0; ks<size; ks++)
					{
						TFT_sendByte ( hi_t );
						TFT_sendByte ( lo_t );
					}
				} // if
				else
				{ // пикселя нет
				//	if ( isTransparent != 0 )       // непрозрачный - рисуем фон
				//	{
						for (ks=0; ks<size; ks++)
						{
							TFT_sendByte ( hi_b );
							TFT_sendByte ( lo_b );
						}
				//	} // if
				} // else

				mask >>= 1;
			} // for - ширина

			// межсимвольный промежуток
			for ( j = 0; j < font->FontSpace; j ++ )
			{
				// if ( isTransparent != 0 )       // непрозрачный - рисуем фон
				// {
					for (ks=0; ks<size; ks++)
					{
						TFT_sendByte ( hi_b );
						TFT_sendByte ( lo_b );
					}
				// } // if
			} // for межсимвольный промежуток
		}
	} // for - высота

	TFT_CS_HIGH ( );

	return size*(widthBits + font->FontSpace);
} // TFT_DrawChar

// Вывод строки
void TFT_DrawString ( char* s, int len, uint16_t x, uint16_t y,
		uint16_t color, uint16_t backColor, uint8_t size)
{
	int i;//, len = strlen(s);

	for ( i = 0; i < len; i ++ )
		x += TFT_DrawChar ( x, y, s[i], color, backColor, size);
} // TFT_DrawChar_DrawString

//-----------------------------------Pressure Sensor------------------------------------------------

void spiSensorSend(uint8_t cmd)
{
	SPI_I2S_SendData(SPI1, cmd);
	while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY) == SET);
}

void ResetSensor(void)
{
	SPI1->CR1 &= 0xFFFC;
	spiSensorSend(0x15);
	spiSensorSend(0x55);
	spiSensorSend(0x40);
}

void readCalibration(long C[])
{
	unsigned long result1 = 0;
	unsigned long inbyte1 = 0;
	unsigned long result2 = 0;
	unsigned long inbyte2 = 0;
	unsigned long result3 = 0;
	unsigned long inbyte3 = 0;
	unsigned long result4 = 0;
	unsigned long inbyte4 = 0;

	ResetSensor();

	spiSensorSend(0x1D);
	spiSensorSend(0x50);
	SPI1->CR1 |= 0x0001;
	SPI_I2S_SendData(SPI1, 0x00);
	while (!(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == SET));
	SPI_I2S_ReceiveData(SPI1);
	SPI_I2S_SendData(SPI1, 0x00);
	while (!(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == SET));
	result1 = SPI_I2S_ReceiveData(SPI1);
	SPI_I2S_SendData(SPI1, 0x00);
	while (!(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == SET));
	inbyte1 = SPI_I2S_ReceiveData(SPI1);
	result1 = (result1 << 8) | inbyte1;

	ResetSensor();

	spiSensorSend(0x1D);
	spiSensorSend(0x60);
	SPI1->CR1 |= 0x0001;
	SPI_I2S_SendData(SPI1, 0x00);
	while (!(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == SET));
	SPI_I2S_ReceiveData(SPI1);
	SPI_I2S_SendData(SPI1, 0x00);
	while (!(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == SET));
	result2 = SPI_I2S_ReceiveData(SPI1);
	SPI_I2S_SendData(SPI1, 0x00);
	while (!(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == SET));
	inbyte2 = SPI_I2S_ReceiveData(SPI1);
	result2 = (result2 << 8) | inbyte2;

	ResetSensor();

	spiSensorSend(0x1D);
	spiSensorSend(0x90);
	SPI1->CR1 |= 0x0001;
	SPI_I2S_SendData(SPI1, 0x00);
	while (!(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == SET));
	SPI_I2S_ReceiveData(SPI1);
	SPI_I2S_SendData(SPI1, 0x00);
	while (!(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == SET));
	result3 = SPI_I2S_ReceiveData(SPI1);
	SPI_I2S_SendData(SPI1, 0x00);
	while (!(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == SET));
	inbyte3 = SPI_I2S_ReceiveData(SPI1);
	result3 = (result3 << 8) | inbyte3;

	ResetSensor();

	spiSensorSend(0x1D);
	spiSensorSend(0xA0);
	SPI1->CR1 |= 0x0001;
	SPI_I2S_SendData(SPI1, 0x00);
	while (!(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == SET));
	SPI_I2S_ReceiveData(SPI1);
	SPI_I2S_SendData(SPI1, 0x00);
	while (!(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == SET));
	result4 = SPI_I2S_ReceiveData(SPI1);
	SPI_I2S_SendData(SPI1, 0x00);
	while (!(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == SET));
	inbyte4 = SPI_I2S_ReceiveData(SPI1);
	result4 = (result4 << 8) | inbyte4;

	C[0] = (result1 >> 3) & 0x00001FFF;
	C[1] = ((result1 & 0x00000007) << 10) | ((result2 >> 6) & 0x000003FF);
	C[2] = (result3 >> 6) & 0x000003FF;
	C[3] = (result4 >> 7) & 0x000001FF;
	C[4] = ((result2 & 0x0000003F) << 6) | (result3 & 0x0000003F);
	C[5] = result4 & 0x0000007F;
}

void readData(long C[], long long DATA[])
{
	long tempMSB = 0; //first byte of value
	long tempLSB = 0; //last byte of value
	long long D2 = 0;

	long presMSB = 0; //first byte of value
	long presLSB = 0; //last byte of value
	long long D1 = 0;

	long long UT1;
	long long dT;
	long long TEMP;
	long long OFF;
	long long SENS;
	long long PCOMP;
	//float TEMPREAL;

	//2nd order compensation only for T > 0°C
	//long long dT2;
	//float TEMPCOMP;

	ResetSensor();

	spiSensorSend(0x0F);
	spiSensorSend(0x20);
	Delay_ms(35);
	while ((GPIOA->IDR & 0x40) == 0x40);
	SPI1->CR1 |= 0x0001;
	SPI_I2S_SendData(SPI1, 0x00);
	while (!(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == SET));
	SPI_I2S_ReceiveData(SPI1);

	SPI_I2S_SendData(SPI1, 0x00);
	while (!(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == SET));
	tempMSB = SPI_I2S_ReceiveData(SPI1);
	SPI_I2S_SendData(SPI1, 0x00);
	while (!(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == SET));
	tempLSB = SPI_I2S_ReceiveData(SPI1);

	tempMSB = tempMSB << 8;
	D2 = tempMSB | tempLSB;

	ResetSensor();

	spiSensorSend(0x0F);
	spiSensorSend(0x40);
	Delay_ms(35);
	while ((GPIOA->IDR & 0x40) == 0x40);
	SPI1->CR1 |= 0x0001;
	SPI_I2S_SendData(SPI1, 0x00);
	while (!(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == SET));
	SPI_I2S_ReceiveData(SPI1);

	SPI_I2S_SendData(SPI1, 0x00);
	while (!(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == SET));
	presMSB = SPI_I2S_ReceiveData(SPI1);
	SPI_I2S_SendData(SPI1, 0x00);
	while (!(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == SET));
	presLSB = SPI_I2S_ReceiveData(SPI1);

	presMSB = presMSB << 8;
	D1 = presMSB | presLSB;

	UT1 = (C[4] << 3) + 10000;
	dT = D2 - UT1;
	TEMP = 200 + ((dT * (C[5] + 100)) >> 11);
	OFF = C[1] + (((C[3] - 250) * dT) >> 12) + 10000;
	SENS = (C[0] / 2) + (((C[2] + 200) * dT) >> 13) + 3000;
	PCOMP = (SENS * (D1 - OFF) >> 12) + 1000;
	//TEMPREAL = TEMP / 10;

	//2nd order compensation only for T > 0°C
	//dT2 = dT - ((dT >> 7 * dT >> 7) >> 3);
	//TEMPCOMP = (200 + (dT2 * (C[5] + 100) >> 11)) / 10;

	DATA[0] = TEMP;
	DATA[1] = PCOMP;
}
