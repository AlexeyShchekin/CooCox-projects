/*************************************************** 
  This is a library for the Adafruit 1.8" SPI display.
  This library works with the Adafruit 1.8" TFT Breakout w/SD card  
  ----> http://www.adafruit.com/products/358  
  as well as Adafruit raw 1.8" TFT display  
  ----> http://www.adafruit.com/products/618
 
  Check out the links above for our tutorials and wiring diagrams 
  These displays use SPI to communicate, 4 or 5 pins are required to  
  interface (RST is optional) 
  Adafruit invests time and resources providing this open source code, 
  please support Adafruit and open-source hardware by purchasing 
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.  
  MIT license, all text above must be included in any redistribution
 ****************************************************/

/* 
 The code contained here is based upon the adafruit driver, but heavily modified
*/

#include <stm32f10x.h>
#include <stm32f10x_rcc.h>
#include <stm32f10x_gpio.h>
#include <stm32f10x_spi.h>
#include "spi.h"
#include "glcdfont.c"
#include "ST7735.h"
#include "sys_init.h"

#define LCD_PORT_BKL  GPIOB
#define LCD_PORT GPIOB
#define GPIO_PIN_BKL GPIO_Pin_1
#define GPIO_PIN_RST GPIO_Pin_1
#define GPIO_PIN_SCE GPIO_Pin_12
#define GPIO_PIN_DC GPIO_Pin_0

#define LCDSPEED SPI_FAST

#define LOW  0
#define HIGH 1

#define LCD_C LOW
#define LCD_D HIGH

#define ST7735_CASET 0x2A
#define ST7735_RASET 0x2B
#define ST7735_MADCTL 0x36
//#define ST7735_MADCTL 0xA8 // Flips screen "hamburger' style
//#define ST7735_MADCTL 0xB8 // Try this to flip screen
#define ST7735_RAMWR 0x2C
#define ST7735_RAMRD 0x2E
#define ST7735_COLMOD 0x3A

#define MADVAL(x) (((x) << 5) | 8)
static uint8_t madctlcurrent = MADVAL(MADCTLGRAPHICS);

struct ST7735_cmdBuf {
  uint8_t command;   // ST7735 command byte
  uint8_t delay;     // ms delay after
  uint8_t len;       // length of parameter data
  uint8_t data[16];  // parameter data
};

static const struct ST7735_cmdBuf initializers[] = {
  // SWRESET Software reset 
  { 0x01, 150, 0, 0},
  // SLPOUT Leave sleep mode
  { 0x11,  150, 0, 0},
  // FRMCTR1, FRMCTR2 Frame Rate configuration -- Normal mode, idle
  // frame rate = fosc / (1 x 2 + 40) * (LINE + 2C + 2D) 
  { 0xB1, 0, 3, { 0x01, 0x2C, 0x2D }},
  { 0xB2, 0, 3, { 0x01, 0x2C, 0x2D }},
  // FRMCTR3 Frame Rate configureation -- partial mode
  { 0xB3, 0, 6, { 0x01, 0x2C, 0x2D, 0x01, 0x2C, 0x2D }},
  // INVCTR Display inversion (no inversion)
  { 0xB4,  0, 1, { 0x07 }},
  // PWCTR1 Power control -4.6V, Auto mode
  { 0xC0,  0, 3, { 0xA2, 0x02, 0x84}},
  // PWCTR2 Power control VGH25 2.4C, VGSEL -10, VGH = 3 * AVDD
  { 0xC1,  0, 1, { 0xC5}},
  // PWCTR3 Power control, opamp current smal, boost frequency
  { 0xC2,  0, 2, { 0x0A, 0x00 }},
  // PWCTR4 Power control, BLK/2, opamp current small and medium low
  { 0xC3,  0, 2, { 0x8A, 0x2A}},
  // PWRCTR5, VMCTR1 Power control
  { 0xC4,  0, 2, { 0x8A, 0xEE}},
  { 0xC5,  0, 1, { 0x0E }},
  // INVOFF Don't invert display
  { 0x20,  0, 0, 0},
  // Memory access directions. row address/col address, bottom to top refesh (10.1.27)
  { ST7735_MADCTL,  0, 1, {MADVAL(MADCTLGRAPHICS)}},
  // Color mode 16 bit (10.1.30
  { ST7735_COLMOD,   0, 1, {0x05}},
  // Column address set 0..127 
  { ST7735_CASET,   0, 4, {0x00, 0x00, 0x00, 0x7F }},
  // Row address set 0..159
  { ST7735_RASET,   0, 4, {0x00, 0x00, 0x00, 0x9F }},
  // GMCTRP1 Gamma correction
  { 0xE0, 0, 16, {0x02, 0x1C, 0x07, 0x12, 0x37, 0x32, 0x29, 0x2D,
			    0x29, 0x25, 0x2B, 0x39, 0x00, 0x01, 0x03, 0x10 }},
  // GMCTRP2 Gamma Polarity corrction
  { 0xE1, 0, 16, {0x03, 0x1d, 0x07, 0x06, 0x2E, 0x2C, 0x29, 0x2D,
			    0x2E, 0x2E, 0x37, 0x3F, 0x00, 0x00, 0x02, 0x10 }},
  // DISPON Display on
  { 0x29, 100, 0, 0},
  // NORON Normal on
  { 0x13,  10, 0, 0},
  // End
  { 0, 0, 0, 0}
};

static void LcdWrite(char dc, const char *data, int nbytes)
{
  GPIO_WriteBit(LCD_PORT,GPIO_PIN_DC, dc);  // dc 1 = data, 0 = control
  GPIO_ResetBits(LCD_PORT,GPIO_PIN_SCE);
  spiReadWrite(SPILCD, 0, data, nbytes, LCDSPEED);
  GPIO_SetBits(LCD_PORT,GPIO_PIN_SCE); 
}

static void LcdWrite16(char dc, const uint16_t *data, int cnt)
{
  GPIO_WriteBit(LCD_PORT,GPIO_PIN_DC, dc);  // dc 1 = data, 0 = control
  GPIO_ResetBits(LCD_PORT,GPIO_PIN_SCE);
  spiReadWrite16(SPILCD, 0, data, cnt, LCDSPEED);
  GPIO_SetBits(LCD_PORT,GPIO_PIN_SCE); 
}

static void ST7735_writeCmd(uint8_t c)
{
  LcdWrite(LCD_C, &c, 1);
}

void ST7735_setAddrWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint8_t madctl)
{
  madctl = MADVAL(madctl);
  if (madctl != madctlcurrent){
      ST7735_writeCmd(ST7735_MADCTL);
      LcdWrite(LCD_D, &madctl, 1);
      madctlcurrent = madctl;
  }
  ST7735_writeCmd(ST7735_CASET);
  LcdWrite16(LCD_D, &x0, 1);
  LcdWrite16(LCD_D, &x1, 1);

  ST7735_writeCmd(ST7735_RASET);
  LcdWrite16(LCD_D, &y0, 1);
  LcdWrite16(LCD_D, &y1, 1);

  ST7735_writeCmd(ST7735_RAMWR);
}

void ST7735_pushColor(uint16_t *color, int cnt)
{
  LcdWrite16(LCD_D, color, cnt);
}

void ST7735_backLight(uint8_t on)
{
  //GPIO_WriteBit(LCD_PORT_BKL,GPIO_PIN_BKL, HIGH);
  if (on)
    GPIO_WriteBit(LCD_PORT_BKL,GPIO_PIN_BKL, LOW);
  else
    GPIO_WriteBit(LCD_PORT_BKL,GPIO_PIN_BKL, HIGH);
}

void ST7735_init() 
{
	init_clock();
	spiInit(SPI1);
  //GPIO_WriteBit(GPIOA, GPIO_Pin_1, 0);
  GPIO_InitTypeDef GPIO_InitStructureBKL;
  GPIO_InitTypeDef GPIO_InitStructureRST;
  GPIO_InitTypeDef GPIO_InitStructureSCE;
  GPIO_InitTypeDef GPIO_InitStructureDC;

  GPIO_StructInit(&GPIO_InitStructureBKL);
  GPIO_StructInit(&GPIO_InitStructureRST);
  GPIO_StructInit(&GPIO_InitStructureSCE);
  
  GPIO_StructInit(&GPIO_InitStructureDC);
//  GPIO_WriteBit(GPIOA, GPIO_Pin_1, 1);
  
  const struct ST7735_cmdBuf *cmd;

  // enable clocks
  //RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOC, ENABLE);
  //RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
  // enable pins
  // backlight
  GPIO_InitStructureBKL.GPIO_Pin = GPIO_PIN_BKL;
  GPIO_InitStructureBKL.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStructureBKL.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOA, &GPIO_InitStructureBKL);

  GPIO_InitStructureRST.GPIO_Pin = GPIO_PIN_RST;
  GPIO_InitStructureRST.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStructureRST.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOB, &GPIO_InitStructureRST);

  GPIO_InitStructureSCE.GPIO_Pin = GPIO_PIN_SCE;
  GPIO_InitStructureSCE.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStructureSCE.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOB, &GPIO_InitStructureSCE);
  
  GPIO_InitStructureDC.GPIO_Pin = GPIO_PIN_DC;
  GPIO_InitStructureDC.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStructureDC.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOB, &GPIO_InitStructureDC);


  // set cs, reset low
  //SysTick_Config(SystemCoreClock / 1000);

  GPIO_WriteBit(LCD_PORT,GPIO_PIN_SCE, HIGH);
  GPIO_WriteBit(LCD_PORT,GPIO_PIN_RST, HIGH);
  Delay_ms(10);
  GPIO_WriteBit(LCD_PORT,GPIO_PIN_RST, LOW);
  Delay_ms(10);
  GPIO_WriteBit(LCD_PORT,GPIO_PIN_RST, HIGH);
  Delay_ms(10);

  // Send initialization commands to ST7735
  for (cmd = initializers; cmd->command; cmd++)
    {
      LcdWrite(LCD_C, &(cmd->command), 1);
      if (cmd->len)
        LcdWrite(LCD_D, cmd->data, cmd->len);
      if (cmd->delay)
        Delay_ms(cmd->delay);
    }
}

int find_greatest_divisor(int n) {
	int i;
  for (i = 2000; i >= 1; i--) {
    if (n % i == 0) {
      return i;
    }
  }
  return 1;
}

void fill_rect_with_color(int x, int y, int w, int h, uint16_t color) {
  ST7735_setAddrWindow(x, y, x + w - 1, y + h - 1, MADCTLGRAPHICS);
  int len = w * h;
  int arr_len = find_greatest_divisor(len);
  uint16_t tbuf[arr_len];
  int i,j;
  for (i = 0; i < len / arr_len; i++) {
    for (j = 0; j < arr_len; j++) {
      tbuf[j] = color;
    }
    ST7735_pushColor(tbuf, arr_len);
  }
}

int get_bit_from_byte(int byte, int bit) {
  return byte & (1 << bit);
}

void display_character(int x, int y, char character, uint16_t textcolor, uint16_t bckcolor, int size) {
  int starting_index = character * 5;
  int len = 5 * 8 * size * size;
  uint16_t tbuf[len];
  ST7735_setAddrWindow(x, y, x + 5*size-1, y + 8*size-1, MADCTLGRAPHICS);
  int i,j;
  for (i = 0; i < 8*size; i++) {
    for (j = 0; j < 5*size; j++) {
      int byte = ASCII[starting_index + (int)(j/size)];
      if (get_bit_from_byte(byte, (int)(i/size))) {
        tbuf[5*size*i + j] = textcolor; // Letter color
      } else {
        tbuf[5*size*i + j] = bckcolor; // Background color 'BLUE' is red for some reason
      }
    }
  }
  ST7735_pushColor(tbuf, len);
}

void display_string(int x, int y, char *str, int len, uint16_t textcolor, uint16_t bckcolor, int size)
{
	int i;
  for (i = 0; i < len; i++) {
    display_character(x + 6*i*size, y, str[i], textcolor, bckcolor, size);
  }
}

void backlightInit(void){
// Initialize backlight on PA01
    GPIO_InitTypeDef GPIO_InitStructure;

    // Enable peripheral Clocks
    // Enable clocks for GPIO Port A
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    // Configure Pins
    // Pin PA01 must be configured as an output
    GPIO_StructInit(&GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
}

uint16_t ColorRGB(uint8_t R, uint8_t G, uint8_t B)
{
	uint16_t R_norm = (uint16_t)(31.0*((float)(R))/255.0);
	uint16_t G_norm = (uint16_t)(63.0*((float)(G))/255.0);
	uint16_t B_norm = (uint16_t)(31.0*((float)(B))/255.0);

	return (((B_norm & 0x001F)<<11) & 0xF800) | (((G_norm & 0x003F)<<5) & 0x07E0) | (R_norm & 0x001F);
}

void drawPixel(int16_t x, int16_t y, uint16_t color)
{
  if((x < 0) ||(x >= ST7735_width) || (y < 0) || (y >= ST7735_height)) return;

  ST7735_setAddrWindow(x,y,x+1,y+1,MADCTLGRAPHICS);

  ST7735_pushColor(&color,1);
}

void drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color)
{
  // Rudimentary clipping
	if((x >= ST7735_width) || (y >= ST7735_height)) return;
	if((y+h-1) >= ST7735_height) h = ST7735_height-y;
	ST7735_setAddrWindow(x, y, x, y+h-1, MADCTLGRAPHICS);

	GPIO_WriteBit(LCD_PORT,GPIO_PIN_DC, 1);  // dc 1 = data, 0 = control
	GPIO_ResetBits(LCD_PORT,GPIO_PIN_SCE);
    SPI_DataSizeConfig(SPILCD, SPI_DataSize_16b);
    SPILCD->CR1 = (SPILCD->CR1 & ~SPI_BaudRatePrescaler_256) | LCDSPEED;
    int i=0;
    for (i=0; i<h; i++)
	{
    	SPI_I2S_SendData(SPILCD, color);
    	while (SPI_I2S_GetFlagStatus(SPILCD, SPI_I2S_FLAG_RXNE) == RESET);
	}
    GPIO_SetBits(LCD_PORT,GPIO_PIN_SCE);
}

void drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color)
{
	// Rudimentary clipping
	if((x >= ST7735_width) || (y >= ST7735_height)) return;
	if((x+w-1) >= ST7735_width)  w = ST7735_width-x;
	ST7735_setAddrWindow(x, y, x+w-1, y, MADCTLGRAPHICS);

	GPIO_WriteBit(LCD_PORT,GPIO_PIN_DC, 1);  // dc 1 = data, 0 = control
	GPIO_ResetBits(LCD_PORT,GPIO_PIN_SCE);
	SPI_DataSizeConfig(SPILCD, SPI_DataSize_16b);
	SPILCD->CR1 = (SPILCD->CR1 & ~SPI_BaudRatePrescaler_256) | LCDSPEED;
	int i=0;
	for (i=0; i<w; i++)
	{
		SPI_I2S_SendData(SPILCD, color);
		while (SPI_I2S_GetFlagStatus(SPILCD, SPI_I2S_FLAG_RXNE) == RESET);
	}
	GPIO_SetBits(LCD_PORT,GPIO_PIN_SCE);
}

void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color)
{
	int16_t steep = abs(y1 - y0) > abs(x1 - x0);
	if (steep)
	{
		swap(x0, y0);
		swap(x1, y1);
	}

	if (x0 > x1)
	{
		swap(x0, x1);
		swap(y0, y1);
	}

	int16_t dx, dy;
	dx = x1 - x0;
	dy = abs(y1 - y0);

	int16_t err = dx / 2;
	int16_t ystep;

	if (y0 < y1)
	{
		ystep = 1;
	}
	else
	{
		ystep = -1;
	}

	for (; x0<=x1; x0++)
	{
		if (steep)
		{
			drawPixel(y0, x0, color);
		}
		else
		{
			drawPixel(x0, y0, color);
		}
		err -= dy;
		if (err < 0)
		{
			y0 += ystep;
			err += dx;
		}
	}
}
