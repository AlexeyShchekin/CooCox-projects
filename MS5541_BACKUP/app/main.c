#include <stm32f10x.h>
#include <stm32f10x_gpio.h>
#include <stm32f10x_rcc.h>
//#include <stm32f10x_rtc.h>
#include <stm32f10x_spi.h>

volatile uint32_t msTicks;

unsigned long result1 = 0;
unsigned long inbyte1 = 0;
unsigned long result2 = 0;
unsigned long inbyte2 = 0;
unsigned long result3 = 0;
unsigned long inbyte3 = 0;
unsigned long result4 = 0;
unsigned long inbyte4 = 0;

long c1 = 0;
long c2 = 0;
long c3 = 0;
long c4 = 0;
long c5 = 0;
long c6 = 0;

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
float TEMPREAL;

//2nd order compensation only for T > 0°C
long long dT2;
float TEMPCOMP;

void SysTick_Handler(void)
{
	msTicks++;
}
void Delay_ms(uint32_t dlyTicks)
{
	uint32_t curTicks;

	curTicks = msTicks;
	while ((msTicks - curTicks) < dlyTicks);
}

void init_clock(void)
{
	__IO uint32_t StartUpCounter = 0;
	FlagStatus HSEStatus = RESET;

	RCC->CR |= (uint32_t)0x00000001;
	RCC->CFGR &= (uint32_t)0xF8FF0000;
	RCC->CR &= (uint32_t)0xFEF6FFFF;
	RCC->CR &= (uint32_t)0xFFFAFFFF;
	RCC->CFGR &= (uint32_t)0xFF80FFFF;

	RCC->CR |= RCC_CR_HSEON;
	do
	{
		HSEStatus = RCC->CR & RCC_CR_HSERDY;
		StartUpCounter++;
	}
	while( (HSEStatus == RESET) && (StartUpCounter != HSEStartUp_TimeOut));

	if ((RCC->CR & RCC_CR_HSERDY) != 0)
	{
		RCC->CFGR &= 0xFFFFC00F;
		RCC->CFGR |= RCC_CFGR_HPRE_DIV1;
		RCC->CFGR |= RCC_CFGR_PPRE2_DIV1;
		RCC->CFGR |= RCC_CFGR_PPRE1_DIV2;

		RCC->CFGR &= 0xFFC0FFFF;
		RCC->CFGR |= (RCC_CFGR_PLLSRC_HSE | RCC_CFGR_PLLMULL6);

		RCC->CR |= RCC_CR_PLLON;
		while((RCC->CR & RCC_CR_PLLRDY) == 0);
		RCC->CFGR &= 0xFFFFFFFC;
		RCC->CFGR |= RCC_CFGR_SW_PLL;
		while ((RCC->CFGR & RCC_CFGR_SWS) != (uint32_t)0x08);
	}
}

void send_to_uart(uint8_t data)
{
	while(!(USART1->SR & 0x40));
	USART1->DR=data;
}

void Init_Timer(void)
{
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

void spiSend(uint8_t cmd)
{
	SPI_I2S_SendData(SPI1, cmd);
	while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY) == SET);
}

void ResetSensor(void)
{
	SPI1->CR1 &= 0xFFFC;
	spiSend(0x15);
	spiSend(0x55);
	spiSend(0x40);
}

void readCalibration(void)
{
	ResetSensor();

	spiSend(0x1D);
	spiSend(0x50);
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

	spiSend(0x1D);
	spiSend(0x60);
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

	spiSend(0x1D);
	spiSend(0x90);
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

	spiSend(0x1D);
	spiSend(0xA0);
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

	c1 = (result1 >> 3) & 0x00001FFF;
	c2 = ((result1 & 0x00000007) << 10) | ((result2 >> 6) & 0x000003FF);
	c3 = (result3 >> 6) & 0x000003FF;
	c4 = (result4 >> 7) & 0x000001FF;
	c5 = ((result2 & 0x0000003F) << 6) | (result3 & 0x0000003F);
	c6 = result4 & 0x0000007F;
}

void readData(void)
{
	ResetSensor();

	spiSend(0x0F);
	spiSend(0x20);
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

	spiSend(0x0F);
	spiSend(0x40);
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

	UT1 = (c5 << 3) + 10000;
	dT = D2 - UT1;
	TEMP = 200 + ((dT * (c6 + 100)) >> 11);
	OFF = c2 + (((c4 - 250) * dT) >> 12) + 10000;
	SENS = (c1 / 2) + (((c3 + 200) * dT) >> 13) + 3000;
	PCOMP = (SENS * (D1 - OFF) >> 12) + 1000;
	TEMPREAL = TEMP / 10;

	//2nd order compensation only for T > 0°C
	dT2 = dT - ((dT >> 7 * dT >> 7) >> 3);
	TEMPCOMP = (200 + (dT2 * (c6 + 100) >> 11)) / 10;
}

int main(void)
{
	init_clock();
	SystemCoreClockUpdate();
	SysTick_Config(SystemCoreClock/1000);

	GPIO_InitTypeDef GPIO_InitStructure;
	SPI_InitTypeDef SPI_InitStructure;
	USART_InitTypeDef uart_struct;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1 | RCC_APB2Periph_USART1 | RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);

	GPIO_InitStructure.GPIO_Pin = (GPIO_Pin_8 | GPIO_Pin_9);
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOB,&GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_6;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_7 | GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	Init_Timer();

	uart_struct.USART_BaudRate = 115200;
	uart_struct.USART_WordLength = USART_WordLength_8b;
	uart_struct.USART_StopBits = USART_StopBits_1;
	uart_struct.USART_Parity = USART_Parity_No;
	uart_struct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	uart_struct.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(USART1, &uart_struct);
	USART_Cmd(USART1, ENABLE);

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

    while(1)
    {
    	readCalibration();
    	readData();
    	/*send_to_uart((uint8_t)((result1&0x0000FF00)>>8));
		send_to_uart((uint8_t)(result1&0x000000FF));

		send_to_uart((uint8_t)((result2&0x0000FF00)>>8));
		send_to_uart((uint8_t)(result2&0x000000FF));

		send_to_uart((uint8_t)((result3&0x0000FF00)>>8));
		send_to_uart((uint8_t)(result3&0x000000FF));

		send_to_uart((uint8_t)((result4&0x0000FF00)>>8));
		send_to_uart((uint8_t)(result4&0x000000FF));

    	send_to_uart(SystemCoreClock/1000000UL);

		send_to_uart((uint8_t)((c1&0x0000FF00)>>8));
		send_to_uart((uint8_t)(c1&0x000000FF));

		send_to_uart((uint8_t)((c2&0x0000FF00)>>8));
		send_to_uart((uint8_t)(c2&0x000000FF));

		send_to_uart((uint8_t)((c3&0x0000FF00)>>8));
		send_to_uart((uint8_t)(c3&0x000000FF));

		send_to_uart((uint8_t)((c4&0x0000FF00)>>8));
		send_to_uart((uint8_t)(c4&0x000000FF));

		send_to_uart((uint8_t)((c5&0x0000FF00)>>8));
		send_to_uart((uint8_t)(c5&0x000000FF));

		send_to_uart((uint8_t)((c6&0x0000FF00)>>8));
		send_to_uart((uint8_t)(c6&0x000000FF));

		send_to_uart((uint8_t)((D1&0x0000FF00)>>8));
		send_to_uart((uint8_t)(D1&0x000000FF));

		send_to_uart((uint8_t)((D2&0x0000FF00)>>8));
		send_to_uart((uint8_t)(D2&0x000000FF));*/

		send_to_uart((uint8_t)((PCOMP&0x0000FF00)>>8));
		send_to_uart((uint8_t)(PCOMP&0x000000FF));

		send_to_uart((uint8_t)((TEMP&0x0000FF00)>>8));
		send_to_uart((uint8_t)(TEMP&0x000000FF));

    	Delay_ms(300);
    }
}
