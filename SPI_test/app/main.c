#include <stm32f10x.h>
#include <stm32f10x_gpio.h>
#include <stm32f10x_rcc.h>
//#include <stm32f10x_rtc.h>
#include <stm32f10x_spi.h>

volatile uint32_t msTicks;
long C[6];
uint32_t W[4];
long Temperature, Pressure, D1, D2;

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

void ResetSensor()
{
	SPI_I2S_SendData(SPI1, 0x15);
	while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY) == SET);
	SPI_I2S_SendData(SPI1, 0x55);
	while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY) == SET);
	SPI_I2S_SendData(SPI1, 0x40);
	while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY) == SET);
	Delay_ms(35);
}

uint32_t GetSensorData(uint8_t CMD1, uint8_t CMD2, uint8_t IsDelay)
{
	uint8_t dataM = 0;
	uint8_t dataL = 0;
	SPI1->CR1 &= 0xFFFC;
	SPI_I2S_SendData(SPI1, CMD1);
	while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY) == SET);
	SPI_I2S_SendData(SPI1, CMD2);
	while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY) == SET);
	SPI1->CR1 |= 0x0001;

	if (IsDelay == 0x01)
	{
		Delay_ms(200);
	}
	else
	{
		Delay_ms(35);
	}

	SPI_I2S_SendData(SPI1, 0x00);
	while (!(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == SET));
	dataL = SPI_I2S_ReceiveData(SPI1);
	SPI_I2S_SendData(SPI1, 0x00);
	while (!(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == SET));
	dataM = SPI_I2S_ReceiveData(SPI1);

	return (((((uint32_t)dataM)&0x000000FF)<<8)&0x0000FF00) + (((uint32_t)dataL)&0x000000FF);
}

void GetCalibration(void)
{
	W[0] = GetSensorData(0x1D,0x50,0x00);
	W[1] = GetSensorData(0x1D,0x60,0x00);
	W[2] = GetSensorData(0x1D,0x90,0x00);
	W[3] = GetSensorData(0x1D,0xA0,0x00);

	C[0] = (long)(((W[0]&0x0000FFF8)>>3)&0x00001FFF);
	C[1] = (long)((((W[0]&0x00000007)<<10)&0x00001C00)|(((W[1]&0x0000FFC0)>>6)&0x000003FF));
	C[2] = (long)(((W[2]&0x0000FFC0)>>6)&0x000003FF);
	C[3] = (long)(((W[3]&0x0000FF80)>>7)&0x000001FF);
	C[4] = (long)((((W[1]&0x0000003F)<<6)&0x00000FC0)|(W[2]&0x0000003F));
	C[5] = (long)(W[3]&0x0000007F);
}

void GetPhysData(void)
{
	long long dT,dT2,SENS,OFF;
	D1 = (long)(GetSensorData(0x0F,0x40,0x01));
	D2 = (long)(GetSensorData(0x0F,0x20,0x01));
	dT = D2 - (C[4]<<3)-10000;
	/*if (dT<0)
	{
		dT2 = dT - (int32_t)(((dT/128)*(dT/128))/2);
	}
	else
	{
		dT2 = dT - (int32_t)(((dT/128)*(dT/128))/8);
	}*/
	Temperature = 200 + ((dT*(C[5]+100))>>11);

	OFF = C[1] + (((C[3] - 250)*dT)>>12) + 10000;
	SENS = (C[0]>>1) + (((((int32_t)C[2]) + 200)*dT)>>13) + 3000;
	Pressure = 1000 + ((SENS*(D1 - OFF))>>12);
}

int main(void)
{
	init_clock();
	SystemCoreClockUpdate();
	SysTick_Config(SystemCoreClock/1000);

	GPIO_InitTypeDef GPIO_InitStructure;
	SPI_InitTypeDef SPI_InitStructure;
	USART_InitTypeDef uart_struct;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1 | RCC_APB2Periph_USART1 | RCC_APB2Periph_GPIOA| RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);

	GPIO_InitStructure.GPIO_Pin = (GPIO_Pin_8 | GPIO_Pin_9);
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOB,&GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7 | GPIO_Pin_9;
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

	int32_t i;
	ResetSensor();
	for (i=0;i<5;i++)
	{
		GetCalibration();
		Delay_ms(200);
	}

    while(1)
    {
    	GetPhysData();
    	send_to_uart(SystemCoreClock/1000000UL);
    	/*for (i=0; i<6; i++)
    	{
			send_to_uart((uint8_t)((C[i]&0x0000FF00)>>8));
			send_to_uart((uint8_t)(C[i]&0x000000FF));
    	}*/
    	//send_to_uart((uint8_t)((Temperature&0xFF000000)>>24));
		//send_to_uart((uint8_t)((Temperature&0x00FF0000)>>16));
		send_to_uart((uint8_t)((Temperature&0x0000FF00)>>8));
		send_to_uart((uint8_t)(Temperature&0x000000FF));
		//send_to_uart((uint8_t)((Pressure&0xFF000000)>>24));
		//send_to_uart((uint8_t)((Pressure&0x00FF0000)>>16));
		send_to_uart((uint8_t)((Pressure&0x0000FF00)>>8));
		send_to_uart((uint8_t)(Pressure&0x000000FF));

    	Delay_ms(300);
    }
}
