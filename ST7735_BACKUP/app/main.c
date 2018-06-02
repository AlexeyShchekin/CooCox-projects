#include <stm32f10x.h>
#include <stm32f10x_rcc.h>
#include <stm32f10x_gpio.h>
#include <stm32f10x_spi.h>
#include "ST7735.h"
#include "sys_init.h"

int main(void) {
  ST7735_init();
  ST7735_backLight(0);
  fill_rect_with_color(0, 0, 160, 128, ColorRGB(255,0,0));
  display_string(20, 60, "Ready!!", 7, ColorRGB(255,255,255), ColorRGB(255,0,0),1);
  int size = 3;
  while(1)
  {
	  fill_rect_with_color(0,0,160, 128 ,ColorRGB(255,0,0));
	  drawFastVLine(100,100,40,ColorRGB(255,255,255));
	  drawFastHLine(100,100,30,ColorRGB(255,255,255));
	  display_string(20, 60, "Ready!!", 7, ColorRGB(255,255,255), ColorRGB(255,0,0),size);
	  drawLine(100,100,50,50,ColorRGB(0,0,0));
	  Delay_ms(1000);
	  fill_rect_with_color(0,0,160, 128,ColorRGB(0,255,0));
	  drawFastVLine(100,100,40,ColorRGB(255,255,255));
	  drawFastHLine(100,100,30,ColorRGB(255,255,255));
	  display_string(20, 60, "Ready!!", 7, ColorRGB(255,255,255), ColorRGB(0,255,0),size);
	  drawLine(100,100,50,50,ColorRGB(0,0,0));
	  Delay_ms(1000);
	  fill_rect_with_color(0,0,160, 128,ColorRGB(0,0,255));
	  drawFastHLine(100,100,30,ColorRGB(255,255,255));
	  display_string(20, 60, "Ready!!", 7, ColorRGB(255,255,255), ColorRGB(0,0,255),size);
	  drawLine(100,100,50,50,ColorRGB(0,0,0));
	  Delay_ms(1000);
	  fill_rect_with_color(0,0,160, 128,ColorRGB(255,255,0));
	  display_string(20, 60, "Ready!!", 7, ColorRGB(255,255,255), ColorRGB(255,255,0),size);
	  drawLine(100,100,50,50,ColorRGB(0,0,0));
	  Delay_ms(1000);
	  fill_rect_with_color(0,0,160, 128,ColorRGB(0,255,255));
	  display_string(20, 60, "Ready!!", 7, ColorRGB(255,255,255), ColorRGB(0,255,255),size);
	  drawLine(100,100,50,50,ColorRGB(0,0,0));
	  Delay_ms(1000);
	  fill_rect_with_color(0,0,160, 128,ColorRGB(255, 0, 255));
	  display_string(20, 60, "Ready!!", 7, ColorRGB(255,255,255), ColorRGB(255,0,255),size);
	  drawLine(100,100,50,50,ColorRGB(0,0,0));
	  Delay_ms(1000);
  }
}
#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t* file, uint32_t line){
            /* Infinite loop */
                        /* Use GDB to find out why we're here */
                                        while(1);
}
#endif

