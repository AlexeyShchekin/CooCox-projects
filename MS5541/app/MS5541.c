#include <stm32f10x.h>
#include <stm32f10x_gpio.h>
#include <stm32f10x_rcc.h>
//#include <stm32f10x_rtc.h>
#include <stm32f10x_spi.h>
#include "sys_init.h"
#include "MS5541.h"

void InitSensor(void)
{		// MSCLK B9, CLK A5, MISO A6, MOSI A7
	GPIO_InitTypeDef GPIO_InitStructure;
	SPI_InitTypeDef SPI_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1 | RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);

	GPIO_InitStructure.GPIO_Pin = (GPIO_Pin_8 | GPIO_Pin_9);	// SIGNALING AND MSCLK
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOB,&GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;		// SPI1 MISO
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_7;	//SPI1 CLK, MOSI
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
	SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
	SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
	SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
	SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
	SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_256;
	SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
	SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
	SPI_Init(SPI1, &SPI_InitStructure);
	SPI_Cmd(SPI1, ENABLE);
	SPI_NSSInternalSoftwareConfig(SPI1, SPI_NSSInternalSoft_Set);

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);

	TIM4->PSC = SystemCoreClock/(2*32768) - 1;
	TIM4->ARR = 1;
	TIM4->DIER |= TIM_DIER_UIE;
	TIM4->CR1 |= TIM_CR1_CEN;

	NVIC_EnableIRQ(TIM4_IRQn);
}

void TIM4_IRQHandler()
{
	GPIOB->ODR^=GPIO_Pin_9;
	TIM_ClearITPendingBit(TIM4, TIM_IT_Update);
}

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
