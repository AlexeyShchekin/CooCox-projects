#include <stm32f10x.h>
#include <stm32f10x_gpio.h>
#include <stm32f10x_rcc.h>
#include "sys_init.h"

GPIO_InitTypeDef GPIO_InitStructure;
USART_InitTypeDef USART_InitStructure;

#define GO_IDLE_STATE 0
#define SEND_IF_COND 8
#define READ_SINGLE_BLOCK 17
#define WRITE_SINGLE_BLOCK 24
#define SD_SEND_OP_COND 41
#define APP_CMD 55
#define READ_OCR 58

uint8_t  SDHC;

#define CS_ENABLE         GPIOA->BSRR = GPIO_BSRR_BR4;
#define CS_DISABLE    	  GPIOA->BSRR = GPIO_BSRR_BS4;

uint8_t spi_send (uint8_t data)
{ 
  while (!(SPI1->SR & SPI_SR_TXE));
  SPI1->DR = data;
  while (!(SPI1->SR & SPI_SR_RXNE));
  return (SPI1->DR);
}

uint8_t spi_read (void)
{ 
  return spi_send(0xff);
}

void spi_init(void)
{
  RCC->APB2ENR |=  RCC_APB2ENR_AFIOEN;
  RCC->APB2ENR |=  RCC_APB2ENR_IOPAEN;
  RCC->APB2ENR |=  RCC_APB2ENR_IOPBEN;

  GPIO_InitTypeDef gpio;

	//AFIO->MAPR |= AFIO_MAPR_SWJ_CFG_DISABLE;
	gpio.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_13 | GPIO_Pin_14;
	gpio.GPIO_Speed = GPIO_Speed_50MHz;
	gpio.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOB, &gpio );

	GPIOB->BSRR = GPIO_BSRR_BS9 | GPIO_BSRR_BS13 | GPIO_BSRR_BS14;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);

		gpio.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;	//SPI1 CLK, MOSI
		gpio.GPIO_Mode = GPIO_Mode_AF_PP;
		gpio.GPIO_Speed = GPIO_Speed_50MHz;
		GPIO_Init(GPIOA, &gpio);

	gpio.GPIO_Pin = GPIO_Pin_4;
	gpio.GPIO_Speed = GPIO_Speed_50MHz;
	gpio.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOA, &gpio );
	GPIOA->BSRR = GPIO_BSRR_BS4;

  /*GPIOA->CRL   |=  GPIO_CRL_MODE5;    //
  GPIOA->CRL   &= ~GPIO_CRL_CNF5;     //
  GPIOA->CRL   |=  GPIO_CRL_CNF5_1;   //
 
  GPIOA->CRL   &= ~GPIO_CRL_MODE6;    //
  GPIOA->CRL   &= ~GPIO_CRL_CNF6;     //
  GPIOA->CRL   |=  GPIO_CRL_CNF6_1;   //
  GPIOA->BSRR   =  GPIO_BSRR_BS6;     //
 
  GPIOA->CRL   |=  GPIO_CRL_MODE7;    //
  GPIOA->CRL   &= ~GPIO_CRL_CNF7;     //
  GPIOA->CRL   |=  GPIO_CRL_CNF7_1;   //
 
  RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
  SPI1->CR2     = 0x0000;
  SPI1->CR1     = SPI_CR1_MSTR;
  SPI1->CR1    |= SPI_CR1_BR;
  SPI1->CR1    |= SPI_CR1_SSI;
  SPI1->CR1    |= SPI_CR1_SSM;
  SPI1->CR1    |= SPI_CR1_SPE;*/

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

uint8_t SD_sendCommand(uint8_t cmd, uint32_t arg)
{
  uint8_t response, wait=0, tmp;     

  if(SDHC == 0)		
  if(cmd == READ_SINGLE_BLOCK || cmd == WRITE_SINGLE_BLOCK )  {arg = arg << 9;}
 
  CS_ENABLE;
 
  spi_send(cmd | 0x40);
  spi_send(arg>>24);
  spi_send(arg>>16);
  spi_send(arg>>8);
  spi_send(arg);
 
  if(cmd == SEND_IF_COND) spi_send(0x87);            
  else                    spi_send(0x95); 
 
  while((response = spi_read()) == 0xff) 
   if(wait++ > 0xfe) break;
 
  if(response == 0x00 && cmd == 58)     
  {
    tmp = spi_read();
    if(tmp & 0x40) SDHC = 1;
    else           SDHC = 0;
    spi_read(); 
    spi_read(); 
    spi_read(); 
  }
 
  spi_read();
 
  CS_DISABLE; 
 
  return response;
}

uint8_t SD_init(void)
{
  uint8_t   i;
  uint8_t   response;
  uint8_t   SD_version = 2;
  uint16_t  retry = 0 ;
 
  spi_init();
  for(i=0;i<10;i++) spi_send(0xff);
 
  CS_ENABLE;
  while(SD_sendCommand(GO_IDLE_STATE, 0)!=0x01)                                   
    if(retry++>0x20)  return 1;                    
  CS_DISABLE;
  spi_send (0xff);
  spi_send (0xff);
 
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

uint8_t SD_ReadSector(uint32_t BlockNumb,uint8_t *buff)
{ 
  uint16_t i=0;
 
  if(SD_sendCommand(READ_SINGLE_BLOCK, BlockNumb)) return 1;  
  CS_ENABLE;

  while(spi_read() != 0xfe)                
  if(i++ > 0xfffe) {CS_DISABLE; return 1;}       
 
  for(i=0; i<512; i++) *buff++ = spi_read();
 
  spi_read(); 
  spi_read(); 
  spi_read(); 
 
  CS_DISABLE;
 
  return 0;
}

uint8_t SD_WriteSector(uint32_t BlockNumb,uint8_t *buff)
{
  uint8_t     response;
  uint16_t    i,wait=0;
 
  if( SD_sendCommand(WRITE_SINGLE_BLOCK, BlockNumb)) return 1;
 
  CS_ENABLE;
  spi_send(0xfe);    
 
  for(i=0; i<512; i++) spi_send(*buff++);
 
  spi_send(0xff);
  spi_send(0xff);
 
  response = spi_read();
 
  if( (response & 0x1f) != 0x05)
  { CS_DISABLE; return 1; }
 
  while(!spi_read())
  if(wait++ > 0xfffe){CS_DISABLE; return 1;}
 
  CS_DISABLE;
  spi_send(0xff);   
  CS_ENABLE;         
 
  while(!spi_read())
  if(wait++ > 0xfffe){CS_DISABLE; return 1;}
  CS_DISABLE;
 
  return 0;
}

void init()
{
//------------------------------------------------------------
//	UART
//------------------------------------------------------------
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1 | RCC_APB2Periph_AFIO, ENABLE);
//Configure GPIO pin 
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;									//	Tx
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;								//	Rx
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
//	Configure UART
	USART_InitStructure.USART_BaudRate = 9600;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

	USART_Init(USART1, &USART_InitStructure);
	USART_Cmd(USART1, ENABLE);
}

void delay(unsigned long p)
{
	while(p>0){p--;}
}

void send_Uart(unsigned char c)
{
	while(USART_GetFlagStatus(USART1, USART_FLAG_TXE)== RESET){}
		USART_SendData(USART1, c);
}

unsigned char getch_Uart()
{
	while(USART_GetFlagStatus(USART1,USART_FLAG_RXNE) == RESET){}
	return USART_ReceiveData(USART1);
}

void send_Uart_str(char *s)
{
  while (*s != 0) 
    send_Uart(*s++);
}

uint8_t Buff[512];

void buff_clear()
{
	int i;
	for(i=0;i<512;i++)
	{
		Buff[i]=0;
	}
}

void send_int_Uart(unsigned long c)
{
	unsigned long d=10000000;
	do
	{
		c=c%d;
		d=d/10;
		send_Uart(c/d+'0');
	}
	while(d>1);
}

unsigned long read_int_uart()
{
	unsigned char temp=0,index=0;
	unsigned long value=0;
	while(index<8)
	{
		value=value*10+temp;
		temp=getch_Uart();
		index++;
		if((47<temp)&&(temp<58))
		{
			temp=temp-48;
		}
		else
		{
			index=255;
		}
	}
	return value;
}

void read_str_uart(unsigned char* s)
{
	unsigned char temp;
	unsigned int index=0;
	while(index<512)
	{
		temp=getch_Uart();
		if(temp!=13)
		{
			*s++=temp;
		}
		else
		{
			index=512;
		}
		index++;
	}
}

int main(void)
{
	unsigned long address,i;
	unsigned char c;
	
	init_clock();
	init();
	send_Uart_str("init pins ok\n");
	if(SD_init()==0)
	{
		send_Uart_str("init sd ok\n");
	}
	else
	{
		send_Uart_str("init sd fail\n");
	}

	send_Uart_str("Alex Sh");
	while(1)
	{
		send_Uart_str("\n----------------------------\n");
		send_Uart_str("\n Read or write sd card r/w = ");
		c=getch_Uart();
		while(getch_Uart()!=13){}
		send_Uart_str("\n Press enter address = ");
		buff_clear();
		address=read_int_uart();
		send_Uart_str("\n Address = ");
		send_int_Uart(address);
		send_Uart('\n');
		if(c=='w')
		{
			send_Uart_str("Press enter data block (max 512B), to exit press <enter>\n");
			read_str_uart(Buff);
			i=0;
			while((i<512)&&(Buff[i]!=0))i++;
			send_Uart_str("Length text data = ");
			send_int_Uart(i);
			send_Uart('\n');
			if(SD_WriteSector(address, Buff)==0)
			{
				send_Uart_str("write sd ok\n");
			}
			else
			{
				send_Uart_str("write sd fail\n");
			}
		}
		else
		{
			if(SD_ReadSector(address, Buff)==0)
			{
				send_Uart_str("read sd ok\n");
			}
			else
			{
				send_Uart_str("read sd fail\n");
			}
			for(i=0;i<512;i++)
			{
				send_Uart(Buff[i]);
			}
		}

		delay(1000000);
	}
}
