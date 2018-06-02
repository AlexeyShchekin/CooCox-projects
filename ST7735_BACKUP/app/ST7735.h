#ifndef ST7335_H
#define ST7335_H

/*  Colors are 565 RGB (5 bits Red, 6 bits green, 5 bits blue) */

#define BLACK           0x0000
#define BLUE            0x001F
#define GREEN           0x07E0
#define CYAN            0x07FF
#define RED             0xF800
#define MAGENTA         0xF81F       
#define YELLOW          0xFFE0  
#define WHITE           0xFFFF

#define SPILCD SPI2

#define swap(a, b) { int16_t t = a; a = b; b = t; }

/* MADCTL [MY MX MV]
 *    MY  row address order   1 (bottom to top), 0 (top to bottom)
 *    MX  col address order   1 (right to left), 0 (left to right)
 *    MV  col/row exchange    1 (exchange),      0 normal
 */

//#define MADCTLGRAPHICS 0x6
#define MADCTLGRAPHICS 0x5
#define MADCTLBMP      0x2

#define ST7735_width  160//128
#define ST7735_height 128//160

void ST7735_setAddrWindow(uint16_t x0, uint16_t y0, 
			  uint16_t x1, uint16_t y1, uint8_t madctl);
void ST7735_pushColor(uint16_t *color, int cnt);
void ST7735_init();
void ST7735_backLight(uint8_t on);
void fill_rect_with_color(int x, int y, int w, int h, uint16_t color);
void display_string(int x, int y, char *str, int len, uint16_t textcolor, uint16_t bckcolor, int size);
void backlightInit(void);

uint16_t ColorRGB(uint8_t R, uint8_t G, uint8_t B);
void drawPixel(int16_t x, int16_t y, uint16_t color);
void drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color);
void drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color);
void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);

#endif
