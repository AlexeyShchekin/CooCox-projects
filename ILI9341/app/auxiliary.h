#ifndef AUXILIARY_H
#define AUXILIARY_H
#include <stdlib.h>

void TempToStr(char Buff[], double Temp);
void TimeToStr(char Buff[], int H, int M, int S);
void send_to_uart(uint8_t data);
void USART1_ON(void);
void USART1_OFF(void);
void NVIC_Configuration(void);
void EXTI_Configuration(void);

typedef struct{
	uint8_t Second;
	uint8_t Minute;
	uint8_t Hour;
	uint8_t Day;
	uint8_t WeekDay;
	uint8_t Month;
	uint8_t Year;
	uint8_t Leap;
} DateTime;

DateTime IncrementSecond(DateTime T);
DateTime IncrementMinute(DateTime T);
DateTime IncrementHour(DateTime T);
DateTime IncrementDay(DateTime T);
DateTime IncrementMonth(DateTime T);
DateTime IncrementYear(DateTime T);

#endif
