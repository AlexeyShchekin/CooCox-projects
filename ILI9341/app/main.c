#include <stm32f10x.h>
#include <stm32f10x_rcc.h>
#include <stm32f10x_gpio.h>
#include <stm32f10x_spi.h>
#include <stm32f10x_rtc.h>
#include <stm32f10x_bkp.h>
#include "sys_init.h"
#include "ili9341.h"
#include "ILI9341_LowLevel_F10x.h"
#include "auxiliary.h"

#define START_STATE			0x0000
#define SETHOUR_STATE		0x0100
#define SETMINUTE_STATE		0x0200
#define SETSECOND_STATE		0x0300
#define SETDAY_STATE		0x0400
#define SETWEEKDAY_STATE	0x0500
#define SETMON_STATE		0x0600
#define SETYEAR_STATE		0x0700
#define TIMEUPDATE_STATE	0x0800

#define RUN_STATE			0x1000
#define ALTCALIBR_STATE		0x1100

#define NODIVE_STATE		0x1000
#define DIVE_STATE			0x1001
#define DECO_STATE			0x1002
#define SURF_STATE			0x1003

#define MENU_STATE			0x2000
#define SETOX1_STATE		0x2010
#define SETOX2_STATE		0x2020
#define SETHE1_STATE		0x2030
#define SETHE2_STATE		0x2040
#define SETTIME_STATE		0x2050
#define SETCONN_STATE		0x2060
#define SETEMUL_STATE		0x2070

#define SLEEP_STATE			0x3000
#define CONNECTION_STATE	0x4000

volatile int HOUR = 0;
volatile int MINUTE = 0;
volatile int SECOND = 0;
volatile uint16_t STATE = START_STATE;
volatile uint16_t PREV_STATE = START_STATE;

//volatile uint8_t PWR_FLAG = 0;
volatile uint8_t TFT_FLAG = 0x00;
volatile uint32_t RTC_Time = 19*3600 + 23*60 + 0;
volatile uint8_t Counter = 0;
volatile uint8_t CounterLED = 0;

char PStr[9] = "     mmHg";
char TStr[6] = "     C";
double Temp;
int sz = 3;

void redraw(void)
{
	char TmStr[8] = "        ";
	char FStr[10] = "          ";
	char DRStr[2] = "  ";
	//SPI_ON();
	Sensor_ON();
	long C[6];
	long long Data[2];
	readCalibration(C);
	readData(C,Data);
	Sensor_OFF();
	TFT_DMA_fillRect(0, 0, 319, 239, TFT_rgb2color(0,0,0));

	Temp = ((double)Data[0])/10;
	TempToStr(TStr, Temp);
	itoa((int)(Data[1]/1.3332),PStr,10);

	TFT_DMA_DrawString(TStr, 6, 20, 20, TFT_rgb2color(255,255,0), TFT_rgb2color(0,0,0),2);
	TFT_DMA_DrawString(PStr, 9, 20, 60, TFT_rgb2color(255,255,0), TFT_rgb2color(0,0,0),2);

	uint32_t T,H,M,S;
	T = (uint32_t)(RTC_GetCounter()>>8);
	H = (uint32_t)(T/3600);
	M = (uint32_t)(T/60) - 60*H;
	S = T - 3600*H - 60*M;
	//itoa((uint32_t)(T>>8),TmStr,10);
	TimeToStr(TmStr,H,M,S);
	TFT_DMA_DrawString(TmStr, 8, 20, 100, TFT_rgb2color(255,255,0), TFT_rgb2color(0,0,0),2);

	itoa((uint32_t)((uint32_t)(SystemCoreClock/1000000)),FStr,10);
	TFT_DMA_DrawString(FStr, 10, 20, 140, TFT_rgb2color(255,255,0), TFT_rgb2color(0,0,0),2);

	itoa((uint32_t)((uint32_t)(BKP->DR1)),DRStr,10);
	TFT_DMA_DrawString(DRStr, 2, 20, 180, TFT_rgb2color(255,255,0), TFT_rgb2color(0,0,0),2);

	TFT_DMA_fillRect(200, 100, 300, 200, TFT_rgb2color(0,255,255));
	//SPI_OFF();
}

void drawSetTime(void)
{
	char Th[3] = "   ";
	char Tm[3] = "   ";
	char Ts[3] = "   ";
	TFT_DMA_DrawString("Set time:", 9, 20, 20, TFT_rgb2color(255,255,255), TFT_rgb2color(0,0,0),2);
	TFT_DMA_DrawString("Hours:", 6, 20, 60, TFT_rgb2color(255,255,255), TFT_rgb2color(0,0,0),2);
	TFT_DMA_DrawString("Minutes:", 8, 20, 100, TFT_rgb2color(255,255,255), TFT_rgb2color(0,0,0),2);
	TFT_DMA_DrawString("Seconds:", 8, 20, 140, TFT_rgb2color(255,255,255), TFT_rgb2color(0,0,0),2);
	itoa(HOUR,Th,10);
	itoa(MINUTE,Tm,10);
	itoa(SECOND,Ts,10);
	if (STATE==SETHOUR_STATE) TFT_DMA_DrawString(Th, 2, 160, 60, TFT_rgb2color(0,0,0), TFT_rgb2color(255,255,0),2);
	else TFT_DMA_DrawString(Th, 2, 160, 60, TFT_rgb2color(255,255,0), TFT_rgb2color(0,0,0),2);
	if (STATE==SETMINUTE_STATE) TFT_DMA_DrawString(Tm, 2, 160, 100, TFT_rgb2color(0,0,0), TFT_rgb2color(255,255,0),2);
	else TFT_DMA_DrawString(Tm, 2, 160, 100, TFT_rgb2color(255,255,0), TFT_rgb2color(0,0,0),2);
	if (STATE==SETSECOND_STATE) TFT_DMA_DrawString(Ts, 2, 160, 140, TFT_rgb2color(0,0,0), TFT_rgb2color(255,255,0),2);
	else TFT_DMA_DrawString(Ts, 2, 160, 140, TFT_rgb2color(255,255,0), TFT_rgb2color(0,0,0),2);
}

void RTCAlarm_IRQHandler(void)
{
  if (RTC_GetITStatus(RTC_IT_ALR) != RESET)
  {
    /* Clear EXTI line17 pending bit */
    EXTI_ClearITPendingBit(EXTI_Line17);
    /* Check if the Wake-Up flag is set */
    if (PWR_GetFlagStatus(PWR_FLAG_WU) != RESET)
    {
      /* Clear Wake Up flag */
      PWR_ClearFlag(PWR_FLAG_WU);
    }
    /* Wait until last write operation on RTC registers has finished */
    RTC_WaitForLastTask();
    /* Clear RTC Alarm interrupt pending bit */
    RTC_ClearITPendingBit(RTC_IT_ALR);
    /* Wait until last write operation on RTC registers has finished */
    RTC_WaitForLastTask();
    //RTC_SetAlarm(RTC_GetCounter());
    //RTC_WaitForLastTask();

    if (TFT_FLAG==0x00)
    {
    	if (CounterLED==0) GPIOA->BSRR = GPIO_BSRR_BS14;
    	else GPIOA->BSRR = GPIO_BSRR_BR14;
    	if (CounterLED==3) CounterLED = 0;
    	else CounterLED++;
    }
	if (Counter>=255) Counter = 0;
	else Counter++;
  }
}

void RTC_Configuration(void)
{
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE); /* Enable PWR and BKP clocks */

  PWR_BackupAccessCmd(ENABLE); /* Allow access to BKP Domain */

  if ((PWR_GetFlagStatus(PWR_FLAG_SB) != RESET) || ((BKP->DR1&0x01)==0x01)) /* Check if the StandBy flag is set */
  {
	  STATE = RUN_STATE;
	  PREV_STATE = START_STATE;
    PWR_ClearFlag(PWR_FLAG_SB); /* Clear StandBy flag */
    RTC_WaitForSynchro(); /* Wait for RTC APB registers synchronisation */
    /* No need to configure the RTC as the RTC configuration(clock source, enable,
       prescaler,...) is kept after wake-up from STANDBY */
  }
  else if ((BKP->DR1&0x01)==0x00)
  {
    /* StandBy flag is not set */
    BKP_DeInit(); /* Reset Backup Domain */
    RCC_LSEConfig(RCC_LSE_ON); /* Enable LSE */
    while (RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET) {};
    RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);
    RCC_RTCCLKCmd(ENABLE); /* Enable RTC Clock */
    RTC_WaitForSynchro(); /* Wait for RTC registers synchronization */
    RTC_WaitForLastTask(); /* Wait until last write operation on RTC registers has finished */
    RTC_SetPrescaler(127); /* RTC period = RTCCLK/RTC_PR = (32.768 KHz)/(32767+1) */
    RTC_WaitForLastTask(); /* Wait until last write operation on RTC registers has finished */
    STATE = SETHOUR_STATE;
    PREV_STATE = START_STATE;
    BKP->DR1 = 0x01;
  }
  else
  {
	  STATE = RUN_STATE;
	  PREV_STATE = START_STATE;
	  RTC_WaitForSynchro();
  }
  RTC_ITConfig(RTC_IT_ALR, ENABLE);
  RTC_WaitForLastTask();
}

int main(void)
{
	init_clock();
	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_All;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	GPIO_Init(GPIOC, &GPIO_InitStructure);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC, DISABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2ENR_ADC1EN | RCC_APB2ENR_ADC2EN , DISABLE);

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC | RCC_APB2Periph_AFIO, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2ENR_AFIOEN, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3;		// INPUTS UART RX
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	RCC_ClearFlag();

	NVIC_Configuration();
	  /* Configure EXTI Line to generate an interrupt on falling edge */
	EXTI_Configuration();

	AFIO->EXTICR[0] = 0x00000000;

	NVIC_EnableIRQ (EXTI3_IRQn);
	NVIC_EnableIRQ (EXTI2_IRQn);
	NVIC_EnableIRQ (EXTI1_IRQn);
	NVIC_EnableIRQ (EXTI0_IRQn);

	RTC_Configuration();

	ili9341_init();

//	USART1_ON();

	//RTC_ClearFlag(RTC_FLAG_SEC); /* Wait till RTC Second event occurs */

	//while(RTC_GetFlagStatus(RTC_FLAG_SEC) == RESET);
	/* Alarm in 5 second, The expectation is we'll restart every SIX seconds */

	uint32_t T;

    while(1)
    {
    	if ((STATE==SETHOUR_STATE)||(STATE==SETMINUTE_STATE)||(STATE==SETSECOND_STATE))
    	{
    		drawSetTime();
    		Delay_ms(500);
    	}
    	if (STATE==TIMEUPDATE_STATE)
    	{
    		Delay_ms(500);
    		RTC_SetCounter(((3600*HOUR+60*MINUTE+SECOND)<<8));
    		RTC_WaitForLastTask();
    		PREV_STATE = STATE;
    		STATE = RUN_STATE;
    		//RTC_ITConfig(RTC_IT_ALR, ENABLE);
		    //RTC_WaitForLastTask();
    	}
    	else //if (STATE==RUN_STATE)
    	{
    		if (TFT_FLAG==0x01)
			{
				init_clock();
				GPIOA->BSRR = GPIO_BSRR_BS14;
				GPIOC->BSRR = GPIO_BSRR_BS0;
				SPI_ON();
				Delay_ms(500);
				ili9341_init();
				TFT_FLAG=0x00;
			}
			if (TFT_FLAG==0x11)
			{
				Delay_ms(500);
				SPI_OFF();
				GPIOA->BSRR = GPIO_BSRR_BR14;
				GPIOC->BSRR = GPIO_BSRR_BR0;
				TFT_FLAG = 0x10;
			}
			if ((STATE&0x3000)!=0x3000)//PWR_FLAG==0)
			{
				if (Counter>=255)
				{
					init_clock();
					GPIOA->BSRR = GPIO_BSRR_BS14;
					T = RTC_GetCounter();
					if (T>=0x01517F00)
					{
						T = T%0x01517F00;
						RTC_SetCounter(T);
						RTC_WaitForLastTask();
					}
					USART1_ON();
					send_to_uart((uint8_t)((T&0xFF000000)>>24));
					send_to_uart((uint8_t)((T&0x00FF0000)>>16));
					send_to_uart((uint8_t)((T&0x0000FF00)>>8));
					send_to_uart((uint8_t)(T&0x000000FF));
					USART1_OFF();
					if (TFT_FLAG==0x00) redraw();
				}
				//RTC_Time = RTC_GetCounter();
				RTC_SetAlarm(RTC_GetCounter());
				/* Wait until last write operation on RTC registers has finished */
				RTC_WaitForLastTask();
				PWR_EnterSTOPMode(PWR_Regulator_LowPower, PWR_STOPEntry_WFI);
				//__WFI();
			}
			else if ((STATE&0x3000)==0x3000)//(PWR_FLAG == 1)
			{
				GPIOA->BSRR = GPIO_BSRR_BR14;
				GPIOC->BSRR = GPIO_BSRR_BR0;
				GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
				GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
				GPIO_Init(GPIOC, &GPIO_InitStructure);
				GPIO_InitStructure.GPIO_Pin = GPIO_Pin_14;
				GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
				GPIO_Init(GPIOA, &GPIO_InitStructure);
				Delay_ms(400);
				PWR_WakeUpPinCmd(ENABLE);
				PWR_EnterSTANDBYMode();
			}
    	}

    }
}

void EXTI0_IRQHandler(void)
{
	EXTI_ClearITPendingBit(EXTI_Line0);
	if ((STATE&0x3000)!=0x3000)//PWR_FLAG==0)
	{
		PREV_STATE = STATE;
		STATE |= SLEEP_STATE;
		//PWR_FLAG = 1;
	}
	else
	{
		//PWR_FLAG = 0;
		PREV_STATE = STATE;
		STATE &= ~SLEEP_STATE;
		STATE |= RUN_STATE;
		init_clock();
	}
	//EXTI->PR|=0x01;
	EXTI_ClearFlag(EXTI_Line0);
}

void EXTI1_IRQHandler(void)
{
	//GPIO_InitTypeDef GPIO_InitStructure;
	EXTI_ClearITPendingBit(EXTI_Line1);
	if (STATE==SETHOUR_STATE)
	{
		PREV_STATE = STATE;
		STATE = SETMINUTE_STATE;
	}
	else if (STATE==SETMINUTE_STATE)
	{
		PREV_STATE = STATE;
		STATE = SETSECOND_STATE;
	}
	else if (STATE==SETSECOND_STATE)
	{
		PREV_STATE = STATE;
		STATE = TIMEUPDATE_STATE;
	}
	else if (STATE==RUN_STATE)
	{
		if (TFT_FLAG == 0x00)
		{
			TFT_FLAG = 0x11;
		}
		else if ((TFT_FLAG&0x10)==0x10)
		{
			TFT_FLAG = 0x01;
			//ili9341_init();
		}
		//EXTI->PR|=0x02;
	}
	EXTI_ClearFlag(EXTI_Line1);
	//EXTI->PR|=0x02;
}

void EXTI2_IRQHandler(void)
{
	EXTI_ClearITPendingBit(EXTI_Line2);
	if (STATE==SETHOUR_STATE)
	{
		if (HOUR==23) HOUR = 0;
		else HOUR = HOUR + 1;
	}
	if (STATE==SETMINUTE_STATE)
	{
		if (MINUTE==59) MINUTE = 0;
		else MINUTE = MINUTE + 1;
	}
	if (STATE==SETSECOND_STATE)
	{
		if (SECOND==59) SECOND = 0;
		else SECOND = SECOND + 1;
	}
	//EXTI->PR|=0x04;
	EXTI_ClearFlag(EXTI_Line2);
}

void EXTI3_IRQHandler(void)
{
	EXTI_ClearITPendingBit(EXTI_Line3);
	if (STATE==SETHOUR_STATE)
	{
		if (HOUR==0) HOUR = 23;
		else HOUR = HOUR - 1;
	}
	if (STATE==SETMINUTE_STATE)
	{
		if (MINUTE==0) MINUTE = 59;
		else MINUTE = MINUTE - 1;
	}
	if (STATE==SETSECOND_STATE)
	{
		if (SECOND==0) SECOND = 59;
		else SECOND = SECOND - 1;
	}
	//EXTI->PR|=0x08;
	EXTI_ClearFlag(EXTI_Line3);
}
