#ifndef ILI9341_H
#define ILI9341_H

#define RED			0xf800
#define GREEN		0x07e0
#define BLUE		0x001f
#define BLACK		0x0000
#define YELLOW		0xffe0
#define WHITE		0xffff
#define CYAN		0x07ff
#define BRIGHT_RED	0xf810
#define GRAY1		0x8410
#define GRAY2		0x4208

void ili9341_init ( void );

void TFT_setXY ( uint16_t poX, uint16_t poY );
int TFT_rgb2color ( int R, int G, int B );

void TFT_setPixel ( uint16_t poX, uint16_t poY, uint16_t color );

void TFT_DMA_drawHorizontalLine(uint16_t x0, uint16_t x1, uint16_t y, uint16_t color);
void TFT_DMA_drawVerticalLine(uint16_t x, uint16_t y0, uint16_t y1, uint16_t color);
void TFT_DMA_drawRect ( uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color );
void TFT_DMA_fillRect ( uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color );
void TFT_fillRect ( uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color );

void TFT_drawLine ( uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color);

uint16_t TFT_DrawChar ( uint16_t x, uint16_t y, uint8_t c, uint16_t textColor,
		uint16_t backColor, uint8_t size);
uint16_t TFT_DMA_DrawChar ( uint16_t x, uint16_t y, uint8_t c, uint16_t textColor,
		uint16_t backColor, uint8_t size);

void TFT_DrawString ( char* s, int len, uint16_t x, uint16_t y,
		uint16_t color, uint16_t backColor, uint8_t size);
void TFT_DMA_DrawString ( char* s, int len, uint16_t x, uint16_t y,
		uint16_t color, uint16_t backColor, uint8_t size);

#endif
