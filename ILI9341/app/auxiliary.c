#include <stm32f10x.h>
#include "auxiliary.h"

void TempToStr(char Buff[], double Temp)
{
	char Bd[2];
	char Br[1];
	int Sd,Sr, sign;
	Sr = ((int)(Temp*10.0))%10;
	Sd = (int)(((int)(Temp*10.0))/10);
	if (Sd < 0)
	{
		sign = 1;
		Sd = -Sd;
	}
	else sign = 0;

	itoa(Sd,Bd,10);
	itoa(Sr,Br,10);
	if (sign==1)
	{
		Buff[0] = '-';
		Buff[1] = Bd[0];
		Buff[2] = Bd[1];
		Buff[3] = '.';
		Buff[4] = Br[0];
	}
	else
	{
		Buff[0] = Bd[0];
		Buff[1] = Bd[1];
		Buff[2] = '.';
		Buff[3] = Br[0];
		Buff[4] = ' ';
	}
}

void TimeToStr(char Buff[], int H, int M, int S)
{
	char SH[2];
	char SM[2];
	char SS[2];
	itoa(H,SH,10);
	itoa(M,SM,10);
	itoa(S,SS,10);
	if (H<10)
	{
		SH[1] = SH[0];
		SH[0] = '0';
	}
	if (M<10)
	{
		SM[1] = SM[0];
		SM[0] = '0';
	}
	if (S<10)
	{
		SS[1] = SS[0];
		SS[0] = '0';
	}
	Buff[0] = SH[0];
	Buff[1] = SH[1];
	Buff[2] = ':';
	Buff[3] = SM[0];
	Buff[4] = SM[1];
	Buff[5] = ':';
	Buff[6] = SS[0];
	Buff[7] = SS[1];
}

void send_to_uart(uint8_t data)
{
	while(!(USART1->SR & 0x40));
	USART1->DR=data;
}

void USART1_ON(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef uart_struct;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;		// INPUTS UART RX
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;	// OUTPUT UART TX
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	uart_struct.USART_BaudRate = 256000;
	uart_struct.USART_WordLength = USART_WordLength_8b;
	uart_struct.USART_StopBits = USART_StopBits_1;
	uart_struct.USART_Parity = USART_Parity_No;
	uart_struct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	uart_struct.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(USART1, &uart_struct);
	USART_Cmd(USART1, ENABLE);
}

void USART1_OFF(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	while (USART_GetFlagStatus(USART1,USART_FLAG_TC)==RESET) {};
	USART_Cmd(USART1, DISABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, DISABLE);
}

void NVIC_Configuration(void)
{
  NVIC_InitTypeDef NVIC_InitStructure;
  /* Configure one bit for preemption priority */
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
  /* Enable the RTC Alarm Interrupt */
  NVIC_InitStructure.NVIC_IRQChannel = RTCAlarm_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
}

void EXTI_Configuration(void)
{
	EXTI_InitTypeDef EXTI_InitStructure;
	/* Configure EXTI Line17(RTC Alarm) to generate an interrupt on rising edge */
	EXTI_ClearITPendingBit(EXTI_Line17);
	EXTI_InitStructure.EXTI_Line = EXTI_Line17;
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure);

  	EXTI_ClearITPendingBit(EXTI_Line0);
	EXTI_InitStructure.EXTI_Line = EXTI_Line0;
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure);

	EXTI_ClearITPendingBit(EXTI_Line1);
	EXTI_InitStructure.EXTI_Line = EXTI_Line1;
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure);

	EXTI_ClearITPendingBit(EXTI_Line2);
	EXTI_InitStructure.EXTI_Line = EXTI_Line2;
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure);

	EXTI_ClearITPendingBit(EXTI_Line3);
	EXTI_InitStructure.EXTI_Line = EXTI_Line3;
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure);
}

DateTime IncrementSecond(DateTime T)
{
	DateTime NewT = T;
	if (NewT.Second==59)
	{
		NewT.Second = 0;
		NewT = IncrementMinute(NewT);
	}
	return NewT;
}

DateTime IncrementMinute(DateTime T)
{
	DateTime NewT = T;
	if (NewT.Minute==59)
	{
		NewT.Minute = 0;
		NewT = IncrementHour(NewT);
	}
	return NewT;
}

DateTime IncrementHour(DateTime T)
{
	DateTime NewT = T;
	if (NewT.Hour==23)
	{
		NewT.Hour = 0;
		NewT = IncrementDay(NewT);
	}
	else NewT.Hour++;
	return NewT;
}

DateTime IncrementDay(DateTime T)
{
	DateTime NewT = T;

	if (NewT.WeekDay==7) NewT.WeekDay = 1;
	else NewT.WeekDay++;

	if (NewT.Day==31)
	{
		NewT.Day = 1;
		if (NewT.Month==12)
		{
			NewT.Month = 1;
			NewT.Year++;
			if ((NewT.Year%4)==0) NewT.Leap = 1;
			else NewT.Leap = 0;
		}
		else NewT.Month++;
	}
	else if ((NewT.Day==30)&&((NewT.Month==4)||(NewT.Month==6)||(NewT.Month==9)||(NewT.Month==11)))
	{
		NewT.Day = 1;
		NewT.Month++;
	}
	else if ((NewT.Day==28)&&(NewT.Month==2)&&(NewT.Leap==0))
	{
		NewT.Day = 1;
		NewT.Month++;
	}
	else if ((NewT.Day==29)&&(NewT.Month==2)&&(NewT.Leap==1))
	{
		NewT.Day = 1;
		NewT.Month++;
	}

	return NewT;
}

DateTime IncrementMonth(DateTime T)
{
	DateTime NewT = T;
	if (NewT.Month==12)
	{
		NewT.Month = 1;
		NewT.Year++;
		if ((NewT.Year%4)==0) NewT.Leap = 1;
		else NewT.Leap = 0;
	}
	else NewT.Month++;

	if ((NewT.Month==4)&&(NewT.Month==6)&&(NewT.Month==9)&&(NewT.Month==11)) NewT.WeekDay+=2;
	else if ((NewT.Month==2)&&(NewT.Leap==1)) NewT.WeekDay++;
	else if ((NewT.Month==2)&&(NewT.Leap==0)) {}
	else NewT.WeekDay+=3;

	return NewT;
}

DateTime IncrementYear(DateTime T)
{
	DateTime NewT = T;
	NewT.Year++;
	if ((NewT.Year%4)==0) NewT.Leap = 1;
	else NewT.Leap = 0;
	if (((NewT.Leap==1)&&(NewT.Month>2))||((T.Leap==1)&&(NewT.Month<=2)))
	{
		NewT.WeekDay += 2;
	}
	else NewT.WeekDay++;
	return NewT;
}
