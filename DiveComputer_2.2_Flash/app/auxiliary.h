#ifndef AUXILIARY_H
#define AUXILIARY_H
#include <stdlib.h>

//void ChargeON(void);
//void ChargeOFF(void);
uint16_t readADC1(void);
void IntToStr(char Buff[], int V);
void AltToStr(char Buff[], double Alt);
void TempToStr(char Buff[], double Temp);
void TimeToStr(char Buff[], int H, int M, int S);
//void DbToStr(O2Printout,CO2*100);
void send_to_uart(uint8_t data);
void send16_to_uart(uint16_t data);
void send32_to_uart(uint32_t data);
void USART1_ON(void);
void USART1_OFF(void);
void NVIC_Configuration(void);
void EXTI_Configuration(void);

uint32_t get_current_logaddr(void);
void set_current_logaddr(uint32_t CURR_ADDR);

typedef struct{
	uint8_t Day;
	uint8_t WeekDay;
	uint8_t Month;
	uint16_t Year;
	uint8_t Leap;
} DateTime;

DateTime IncrementDay(DateTime T);
void StoreDate(DateTime T);
DateTime GetDate(void);

#endif
