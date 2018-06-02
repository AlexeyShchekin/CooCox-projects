#include <stdint.h>
#include <stdlib.h>
#include <string.h> // For memcpy
#include <stm32f10x_gpio.h>
#include <stm32f10x_rcc.h>
#include <stm32f10x_spi.h>

#include <sys_init.h>
#include <st7735.h>
#include <font5x7.h>
#include <font7x11.h>
#include <font10x16.h>

uint16_t scr_width;
uint16_t scr_height;


void ST7735_write(uint8_t data) {
#ifdef SOFT_SPI
	uint8_t i;

	for(i = 0; i < 8; i++) {
		if (data & 0x80) SDA_H(); else SDA_L();
		data = data << 1;
		SCK_L();
		SCK_H();
	}
#else
	while (SPI_I2S_GetFlagStatus(SPI_PORT,SPI_I2S_FLAG_TXE) == RESET);
	SPI_I2S_SendData(SPI_PORT,data);
#endif
}

void ST7735_cmd(uint8_t cmd) {
	A0_L();
	ST7735_write(cmd);
#ifndef SOFT_SPI
	while (SPI_I2S_GetFlagStatus(SPI_PORT,SPI_I2S_FLAG_BSY) == SET);
#endif
}

void ST7735_data(uint8_t data) {
	A0_H();
	ST7735_write(data);
#ifndef SOFT_SPI
	while (SPI_I2S_GetFlagStatus(SPI_PORT,SPI_I2S_FLAG_BSY) == SET);
#endif
}

uint16_t RGB565(uint8_t R,uint8_t G,uint8_t B) {
	return ((B >> 3) << 11) | ((G >> 2) << 5) | (R >> 3);
}

void ST7735_Init(void) {
#ifdef SOFT_SPI
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO,ENABLE);
	GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE); // Disable JTAG for use PB3
#else
	#if _SPI_PORT == 1
		// SPI1
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1 | RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO,ENABLE);
	#elif _SPI_PORT == 2
		// SPI2
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2,ENABLE);
	#elif _SPI_PORT == 3
		// SPI3
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO,ENABLE);
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI3,ENABLE);
		GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE); // Disable JTAG for use PB3
	#endif

	// Configure and enable SPI
	SPI_InitTypeDef SPI;
	SPI.SPI_Mode = SPI_Mode_Master;
	SPI.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2;
	SPI.SPI_Direction = SPI_Direction_1Line_Tx;
	SPI.SPI_CPOL = SPI_CPOL_Low;
	SPI.SPI_CPHA = SPI_CPHA_1Edge;
	SPI.SPI_CRCPolynomial = 7;
	SPI.SPI_DataSize = SPI_DataSize_8b;
	SPI.SPI_FirstBit = SPI_FirstBit_MSB;
	SPI.SPI_NSS = SPI_NSS_Soft;
	SPI_Init(SPI_PORT,&SPI);
	// NSS must be set to '1' due to NSS_Soft settings (otherwise it will be Multimaster mode).
	SPI_NSSInternalSoftwareConfig(SPI_PORT,SPI_NSSInternalSoft_Set);
	SPI_Cmd(SPI_PORT,ENABLE);
#endif

	GPIO_InitTypeDef PORT;
	PORT.GPIO_Mode = GPIO_Mode_Out_PP;
	PORT.GPIO_Speed = GPIO_Speed_50MHz;
	PORT.GPIO_Pin = ST7735_CS_PIN;
	GPIO_Init(ST7735_CS_PORT,&PORT);
	PORT.GPIO_Pin = ST7735_RST_PIN;
	GPIO_Init(ST7735_RST_PORT,&PORT);
	PORT.GPIO_Pin = ST7735_A0_PIN;
	GPIO_Init(ST7735_A0_PORT,&PORT);
#ifdef SOFT_SPI
	PORT.GPIO_Pin = ST7735_SDA_PIN;
	GPIO_Init(ST7735_SDA_PORT,&PORT);
	PORT.GPIO_Pin = ST7735_SCK_PIN;
	GPIO_Init(ST7735_SCK_PORT,&PORT);
#else
	// Configure SPI pins
	PORT.GPIO_Pin = SPI_SCK_PIN | SPI_MOSI_PIN;
	PORT.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(SPI_GPIO_PORT,&PORT);
	PORT.GPIO_Mode = GPIO_Mode_Out_PP;
#endif

	// Reset display
	CS_H();
	RST_H();
	Delay_ms(5);
	RST_L();
	Delay_ms(5);
	RST_H();
	CS_H();
	Delay_ms(5);
	CS_L();

	ST7735_cmd(0x11); // Sleep out & booster on
	Delay_ms(120); // Datasheet says what display wakes about 120ms (may be much faster actually)

	ST7735_cmd(0xb1);   // In normal mode (full colors):
	A0_H();
	ST7735_write(0x05); //   RTNA set 1-line period: RTNA2, RTNA0
	ST7735_write(0x3c); //   Front porch: FPA5,FPA4,FPA3,FPA2
	ST7735_write(0x3c); //   Back porch: BPA5,BPA4,BPA3,BPA2

	ST7735_cmd(0xb2);   // In idle mode (8-colors):
	A0_H();
	ST7735_write(0x05); //   RTNB set 1-line period: RTNAB, RTNB0
	ST7735_write(0x3c); //   Front porch: FPB5,FPB4,FPB3,FPB2
	ST7735_write(0x3c); //   Back porch: BPB5,BPB4,BPB3,BPB2

	ST7735_cmd(0xb3);   // In partial mode + full colors:
	A0_H();
	ST7735_write(0x05); //   RTNC set 1-line period: RTNC2, RTNC0
	ST7735_write(0x3c); //   Front porch: FPC5,FPC4,FPC3,FPC2
	ST7735_write(0x3c); //   Back porch: BPC5,BPC4,BPC3,BPC2
	ST7735_write(0x05); //   RTND set 1-line period: RTND2, RTND0
	ST7735_write(0x3c); //   Front porch: FPD5,FPD4,FPD3,FPD2
	ST7735_write(0x3c); //   Back porch: BPD5,BPD4,BPD3,BPD2

	ST7735_cmd(0xB4);   // Display dot inversion control:
	ST7735_data(0x03);  //   NLB,NLC

	ST7735_cmd(0x3a);   // Interface pixel format
//	ST7735_data(0x03);  // 12-bit/pixel RGB 4-4-4 (4k colors)
	ST7735_data(0x05);  // 16-bit/pixel RGB 5-6-5 (65k colors)
//	ST7735_data(0x06);  // 18-bit/pixel RGB 6-6-6 (256k colors)

//	ST7735_cmd(0x36);   // Memory data access control:
						//   MY MX MV ML RGB MH - -
//	ST7735_data(0x00);  //   Normal: Top to Bottom; Left to Right; RGB
//	ST7735_data(0x80);  //   Y-Mirror: Bottom to top; Left to Right; RGB
//	ST7735_data(0x40);  //   X-Mirror: Top to Bottom; Right to Left; RGB
//	ST7735_data(0xc0);  //   X-Mirror,Y-Mirror: Bottom to top; Right to left; RGB
//	ST7735_data(0x20);  //   X-Y Exchange: X and Y changed positions
//	ST7735_data(0xC8);
//	ST7735_data(0xA0);  //   X-Y Exchange,Y-Mirror
//	ST7735_data(0x60);  //   X-Y Exchange,X-Mirror
//	ST7735_data(0xE0);  //   X-Y Exchange,X-Mirror,Y-Mirror

	ST7735_cmd(0x20);   // Display inversion off
//	ST7735_cmd(0x21);   // Display inversion on

	ST7735_cmd(0x13);   // Partial mode off

	ST7735_cmd(0x26);   // Gamma curve set:
	ST7735_data(0x01);  //   Gamma curve 1 (G2.2) or (G1.0)
//	ST7735_data(0x02);  //   Gamma curve 2 (G1.8) or (G2.5)
//	ST7735_data(0x04);  //   Gamma curve 3 (G2.5) or (G2.2)
//	ST7735_data(0x08);  //   Gamma curve 4 (G1.0) or (G1.8)

	ST7735_cmd(0x29);   // Display on

	CS_H();

	ST7735_Orientation(scr_normal);
}

void ST7735_Orientation(ScrOrientation_TypeDef orientation) {
	CS_L();
	ST7735_cmd(0x36); // Memory data access control:
	switch(orientation) {
	case scr_CW:
		scr_width  = scr_h;
		scr_height = scr_w;
		ST7735_data(0xA0); // X-Y Exchange,Y-Mirror
		break;
	case scr_CCW:
		scr_width  = scr_h;
		scr_height = scr_w;
		ST7735_data(0x60); // X-Y Exchange,X-Mirror
		break;
	case scr_180:
		scr_width  = scr_w;
		scr_height = scr_h;
		ST7735_data(0xc0); // X-Mirror,Y-Mirror: Bottom to top; Right to left; RGB
		break;
	default:
		scr_width  = scr_w;
		scr_height = scr_h;
		ST7735_data(0x00); // Normal: Top to Bottom; Left to Right; RGB
		break;
	}
	CS_H();
}

void ST7735_AddrSet(uint16_t XS, uint16_t YS, uint16_t XE, uint16_t YE) {
	ST7735_cmd(0x2a); // Column address set
	A0_H();
	ST7735_write(XS >> 8);
	ST7735_write(XS);
	ST7735_write(XE >> 8);
	ST7735_write(XE);

	ST7735_cmd(0x2b); // Row address set
	A0_H();
	ST7735_write(YS >> 8);
	ST7735_write(YS);
	ST7735_write(YE >> 8);
	ST7735_write(YE);

	ST7735_cmd(0x2c); // Memory write
}

void ST7735_Clear(uint16_t color) {
	uint16_t i;
	uint8_t  CH,CL;

	CH = color >> 8;
	CL = (uint8_t)color;

	CS_L();
	ST7735_AddrSet(0,0,scr_width - 1,scr_height - 1);
	A0_H();
	for (i = 0; i < scr_width * scr_height; i++) {
		ST7735_write(CH);
		ST7735_write(CL);
	}
	CS_H();
}

void ST7735_Pixel(uint16_t X, uint16_t Y, uint16_t color) {
    CS_L();
    ST7735_AddrSet(X,Y,X,Y);
    A0_H();
    ST7735_write(color >> 8);
    ST7735_write((uint8_t)color);
    CS_H();
}

void ST7735_HLine(uint16_t X1, uint16_t X2, uint16_t Y, uint16_t color) {
    uint16_t i;
    uint8_t CH = color >> 8;
    uint8_t CL = (uint8_t)color;

    CS_L();
    ST7735_AddrSet(X1,Y,X2,Y);
	A0_H();
	for (i = 0; i <= (X2 - X1); i++) {
		ST7735_write(CH);
		ST7735_write(CL);
	}
	CS_H();
}

void ST7735_VLine(uint16_t X, uint16_t Y1, uint16_t Y2, uint16_t color) {
    uint16_t i;
    uint8_t CH = color >> 8;
    uint8_t CL = (uint8_t)color;

    CS_L();
    ST7735_AddrSet(X,Y1,X,Y2);
	A0_H();
	for (i = 0; i <= (Y2 - Y1); i++) {
		ST7735_write(CH);
		ST7735_write(CL);
	}
	CS_H();
}

void ST7735_Line(int16_t X1, int16_t Y1, int16_t X2, int16_t Y2, uint16_t color) {
	int16_t dX = X2-X1;
	int16_t dY = Y2-Y1;
	int16_t dXsym = (dX > 0) ? 1 : -1;
	int16_t dYsym = (dY > 0) ? 1 : -1;

	if (dX == 0) {
		if (Y2>Y1) ST7735_VLine(X1,Y1,Y2,color); else ST7735_VLine(X1,Y2,Y1,color);
		return;
	}
	if (dY == 0) {
		if (X2>X1) ST7735_HLine(X1,X2,Y1,color); else ST7735_HLine(X2,X1,Y1,color);
		return;
	}

	dX *= dXsym;
	dY *= dYsym;
	int16_t dX2 = dX << 1;
	int16_t dY2 = dY << 1;
	int16_t di;

	if (dX >= dY) {
		di = dY2 - dX;
		while (X1 != X2) {
			ST7735_Pixel(X1,Y1,color);
			X1 += dXsym;
			if (di < 0) {
				di += dY2;
			} else {
				di += dY2 - dX2;
				Y1 += dYsym;
			}
		}
	} else {
		di = dX2 - dY;
		while (Y1 != Y2) {
			ST7735_Pixel(X1,Y1,color);
			Y1 += dYsym;
			if (di < 0) {
				di += dX2;
			} else {
				di += dX2 - dY2;
				X1 += dXsym;
			}
		}
	}
	ST7735_Pixel(X1,Y1,color);
}

void ST7735_Rect(uint16_t X1, uint16_t Y1, uint16_t X2, uint16_t Y2, uint16_t color) {
	ST7735_HLine(X1,X2,Y1,color);
	ST7735_HLine(X1,X2,Y2,color);
	ST7735_VLine(X1,Y1,Y2,color);
	ST7735_VLine(X2,Y1,Y2,color);
}

void ST7735_FillRect_DMA(uint16_t X1, uint16_t Y1, uint16_t X2, uint16_t Y2, uint16_t color) {
	uint16_t FS = (X2 - X1 + 1) * (Y2 - Y1 + 1);

    DMA_InitTypeDef DMA_INI;
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
	DMA_DeInit(DMA1_Channel3);
	DMA_INI.DMA_PeripheralBaseAddr = (uint32_t)&(SPI1->DR);
	DMA_INI.DMA_MemoryBaseAddr = (uint32_t)&color;
	DMA_INI.DMA_DIR = DMA_DIR_PeripheralDST;
	DMA_INI.DMA_BufferSize = FS;
	DMA_INI.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_INI.DMA_MemoryInc = DMA_MemoryInc_Disable;
	DMA_INI.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
	DMA_INI.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
	DMA_INI.DMA_Mode = DMA_Mode_Circular;
	DMA_INI.DMA_Priority = DMA_Priority_VeryHigh;
	DMA_INI.DMA_M2M = DMA_M2M_Disable;
	DMA_Init(DMA1_Channel3, &DMA_INI);

	CS_L();
	ST7735_AddrSet(X1,Y1,X2,Y2);
	A0_H();

	SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Tx, ENABLE);
	SPI_DataSizeConfig(SPI1, SPI_DataSize_16b);

	DMA_Cmd(DMA1_Channel3, ENABLE);
	while(!DMA_GetFlagStatus(DMA1_FLAG_TC3));
	DMA_Cmd(DMA1_Channel3, DISABLE);
	SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Tx, DISABLE);
	SPI_DataSizeConfig(SPI1, SPI_DataSize_8b);

	CS_H();
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, DISABLE);
}

void ST7735_PutChar5x7_DMA(uint16_t X, uint16_t Y, uint8_t chr, uint16_t color, uint16_t backcolor, uint8_t size) {
	uint16_t i,j;
	uint8_t buffer[5];
	int pNum = 35*size*size;
	uint16_t COL_BUF[pNum];

	memcpy(buffer,&Font5x7[(chr - 32) * 5],5);

	int k = 0;
	int n,m;
	for (j = 0; j < 7; j++) {
		for (n=0; n<size; n++)
		{
			for (i = 0; i < 5; i++) {
				if ((buffer[i] >> j) & 0x01) {
					for (m=0; m<size; m++)
					{
						COL_BUF[k] = color;
						k++;
					}
				} else {
					for (m=0; m<size; m++)
					{
						COL_BUF[k] = backcolor;
						k++;
					}
				}
			}
		}
	}

	DMA_InitTypeDef DMA_INI;
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
	DMA_DeInit(DMA1_Channel3);
	DMA_INI.DMA_PeripheralBaseAddr = (uint32_t)&(SPI1->DR);
	DMA_INI.DMA_MemoryBaseAddr = (uint32_t)&COL_BUF[0];
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

	CS_L();
	ST7735_AddrSet(X,Y,X + 5*size - 1,Y + 7*size - 1);
	A0_H();

	SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Tx, ENABLE);
	SPI_DataSizeConfig(SPI1, SPI_DataSize_16b);

	DMA_Cmd(DMA1_Channel3, ENABLE);
	while(!DMA_GetFlagStatus(DMA1_FLAG_TC3));
	DMA_Cmd(DMA1_Channel3, DISABLE);
	SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Tx, DISABLE);
	SPI_DataSizeConfig(SPI1, SPI_DataSize_8b);

	CS_H();
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, DISABLE);
}

void ST7735_PutStr5x7_DMA(uint8_t X, uint8_t Y, char *str, uint16_t color, uint16_t backcolor, uint8_t size) {
    while (*str) {
        ST7735_PutChar5x7_DMA(X,Y,*str++,color,backcolor,size);
        if (X < scr_width - 6*size) { X += 6*size; } else if (Y < scr_height - 8*size) { X = 0; Y += 8*size; } else { X = 0; Y = 0; }
    };
}

void ST7735_PutChar7x11_DMA(uint16_t X, uint16_t Y, uint8_t chr, uint16_t color, uint16_t backcolor, uint8_t size) {
	uint16_t i,j,n,m;
	uint16_t buffer[7];
	int pNum = 77*size*size;
	uint16_t COL_BUF[pNum];

	memcpy(buffer,&Font7x11[(chr-32) * 15 + 1], 14);

	int k = 0;

	for (j = 0; j < 11; j++)
	{
		for (n=0; n<size; n++)
		{
			for (i = 0; i < 7; i++) {
				if ((buffer[i] >> j) & 0x01)
				{
					for (m=0; m<size; m++)
					{
						COL_BUF[k] = color;
						k++;
					}
				} else
				{
					for (m=0; m<size; m++)
					{
						COL_BUF[k] = backcolor;
						k++;
					}
				}
			}
		}
    }

	DMA_InitTypeDef DMA_INI;
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
	DMA_DeInit(DMA1_Channel3);
	DMA_INI.DMA_PeripheralBaseAddr = (uint32_t)&(SPI1->DR);
	DMA_INI.DMA_MemoryBaseAddr = (uint32_t)&COL_BUF[0];
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

	CS_L();
	ST7735_AddrSet(X,Y,X + 7*size - 1,Y + 11*size - 1);
	A0_H();

	SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Tx, ENABLE);
	SPI_DataSizeConfig(SPI1, SPI_DataSize_16b);

	DMA_Cmd(DMA1_Channel3, ENABLE);
	while(!DMA_GetFlagStatus(DMA1_FLAG_TC3));
	DMA_Cmd(DMA1_Channel3, DISABLE);
	SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Tx, DISABLE);
	SPI_DataSizeConfig(SPI1, SPI_DataSize_8b);

	CS_H();
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, DISABLE);
}

void ST7735_PutStr7x11_DMA(uint8_t X, uint8_t Y, char *str, uint16_t color, uint16_t backcolor,uint8_t size) {
    while (*str) {
        ST7735_PutChar7x11_DMA(X,Y,*str++,color,backcolor,size);
        if (X < scr_width - 8*size) { X += 8*size; } else if (Y < scr_height - 12*size) { X = 0; Y += 12*size; } else { X = 0; Y = 0; }
    };
}

void ST7735_PutChar10x16_DMA(uint16_t X, uint16_t Y, uint8_t chr, uint16_t color, uint16_t backcolor, uint8_t size) {
	uint16_t i,j,n,m;
	uint16_t buffer[10];
	int pNum = 160*size*size;
	uint16_t COL_BUF[pNum];

	memcpy(buffer,&Font10x16[(chr-32) * 21 + 1],20);

	int k = 0;

	for (j = 0; j < 16; j++)
	{
		for (n=0; n<size; n++)
		{
			for (i = 0; i < 10; i++) {
				if ((buffer[i] >> j) & 0x01)
				{
					for (m=0; m<size; m++)
					{
						COL_BUF[k] = color;
						k++;
					}
					//ST7735_write(CH);
					//ST7735_write(CL);
				} else
				{
					for (m=0; m<size; m++)
					{
						COL_BUF[k] = backcolor;
						k++;
					}
					//ST7735_write(BCH);
					//ST7735_write(BCL);
				}
			}
		}
    }

	DMA_InitTypeDef DMA_INI;
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
	DMA_DeInit(DMA1_Channel3);
	DMA_INI.DMA_PeripheralBaseAddr = (uint32_t)&(SPI1->DR);
	DMA_INI.DMA_MemoryBaseAddr = (uint32_t)&COL_BUF[0];
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

	CS_L();
	ST7735_AddrSet(X,Y,X + 10*size - 1,Y + 16*size - 1);
	A0_H();

	SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Tx, ENABLE);
	SPI_DataSizeConfig(SPI1, SPI_DataSize_16b);

	DMA_Cmd(DMA1_Channel3, ENABLE);
	while(!DMA_GetFlagStatus(DMA1_FLAG_TC3));
	DMA_Cmd(DMA1_Channel3, DISABLE);
	SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Tx, DISABLE);
	SPI_DataSizeConfig(SPI1, SPI_DataSize_8b);

	CS_H();
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, DISABLE);
}

void ST7735_PutStr10x16_DMA(uint8_t X, uint8_t Y, char *str, uint16_t color, uint16_t backcolor, uint8_t size) {
    while (*str) {
        ST7735_PutChar10x16_DMA(X,Y,*str++,color,backcolor, size);
        if (X < scr_width - 10*size) { X += 10*size; } else if (Y < scr_height - 16*size) { X = 0; Y += 16*size; } else { X = 0; Y = 0; }
    };
}
