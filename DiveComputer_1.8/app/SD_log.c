#include <stm32f10x.h>
#include <stm32f10x_gpio.h>
#include <stm32f10x_rcc.h>
#include "SD_log.h"

volatile uint8_t SDHC;

uint8_t SD_SPI_send (uint8_t data)
{
  while (!(SPI1->SR & SPI_SR_TXE));
  SPI1->DR = data;
  while (!(SPI1->SR & SPI_SR_RXNE));
  return (SPI1->DR);
}

uint8_t SD_SPI_read(void)
{
  return SD_SPI_send(0xff);
}

void SD_SPI_SpeedSwitch(void)
{
	SPI_InitTypeDef SPI;
	SPI.SPI_Mode = SPI_Mode_Master;
	SPI.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_256;
	SPI.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
	SPI.SPI_CPOL = SPI_CPOL_Low;
	SPI.SPI_CPHA = SPI_CPHA_1Edge;
	SPI.SPI_CRCPolynomial = 7;
	SPI.SPI_DataSize = SPI_DataSize_8b;
	SPI.SPI_FirstBit = SPI_FirstBit_MSB;
	SPI.SPI_NSS = SPI_NSS_Soft;
	SPI_Init(SPI1,&SPI);
	SPI_NSSInternalSoftwareConfig(SPI1,SPI_NSSInternalSoft_Set);
	SPI_Cmd(SPI1,ENABLE);
}

void SD_SPI_init(void)
{
	GPIO_InitTypeDef gpio;

	GPIOB->BSRR = GPIO_BSRR_BS13 | GPIO_BSRR_BS14;

	gpio.GPIO_Pin = GPIO_Pin_4;
	gpio.GPIO_Speed = GPIO_Speed_50MHz;
	gpio.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOA, &gpio );
	GPIOA->BSRR = GPIO_BSRR_BS4;

	SD_SPI_SpeedSwitch();
}

uint8_t SD_sendCommand(uint8_t cmd, uint32_t arg)
{
  uint8_t response, wait=0, tmp;

  if(SDHC == 0)
  if(cmd == READ_SINGLE_BLOCK || cmd == WRITE_SINGLE_BLOCK )  {arg = arg << 9;}

  CS_SD_ENABLE;

  SD_SPI_send(cmd | 0x40);
  SD_SPI_send(arg>>24);
  SD_SPI_send(arg>>16);
  SD_SPI_send(arg>>8);
  SD_SPI_send(arg);

  if(cmd == SEND_IF_COND) SD_SPI_send(0x87);
  else                    SD_SPI_send(0x95);

  while((response = SD_SPI_read()) == 0xff)
   if(wait++ > 0xfe) break;

  if(response == 0x00 && cmd == 58)
  {
    tmp = SD_SPI_read();
    if(tmp & 0x40) SDHC = 1;
    else           SDHC = 0;
    SD_SPI_read();
    SD_SPI_read();
    SD_SPI_read();
  }

  SD_SPI_read();

  CS_SD_DISABLE;

  return response;
}

uint8_t SD_init(void)
{
  uint8_t   i;
  uint8_t   response;
  uint8_t   SD_version = 2;
  uint16_t  retry = 0 ;

  SD_SPI_init();
  for(i=0;i<10;i++) SD_SPI_send(0xff);

  CS_SD_ENABLE;
  while(SD_sendCommand(GO_IDLE_STATE, 0)!=0x01)
    if(retry++>0x20)  return 1;
  CS_SD_DISABLE;
  SD_SPI_send (0xff);
  SD_SPI_send (0xff);

  retry = 0;
  while(SD_sendCommand(SEND_IF_COND,0x000001AA)!=0x01)
  {
    if(retry++>0xfe)
    {
      SD_version = 1;
      break;
    }
  }

 retry = 0;
 do
 {
   response = SD_sendCommand(APP_CMD,0);
   response = SD_sendCommand(SD_SEND_OP_COND,0x40000000);
   retry++;
   if(retry>0xffe) return 1;
 }while(response != 0x00);

 retry = 0;
 SDHC = 0;
 if (SD_version == 2)
 {
   while(SD_sendCommand(READ_OCR,0)!=0x00)
	 if(retry++>0xfe)  break;
 }

 return 0;
}
