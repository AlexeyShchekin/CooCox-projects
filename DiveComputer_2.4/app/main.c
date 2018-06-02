#include <math.h>
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
#include "MS5803_I2C.h"

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

#define NODIVE_STATE		0
#define DIVE_STATE			1
#define DECO_STATE			2
#define SURF_STATE			3

#define MENU_STATE			0x2000
#define MENU_ALTITUDE		0
#define MENU_SETOXLIM		1
#define MENU_SETOX			2
#define MENU_SETHE			3
#define MENU_SETCONN		6
#define MENU_SETEMUL		4
#define MENU_SETTIME		5

#define SLEEP_STATE			0x3000
#define CONNECTION_STATE	0x4000

uint32_t BASE_ADDRESS = 0x08020000;
uint32_t END_ADDRESS = 0x080C0000;

volatile int HOUR = 0;
volatile int MINUTE = 0;
volatile int SECOND = 0;
volatile DateTime DT = {31,7,1,2016,1};

volatile uint16_t STATE = START_STATE;
volatile uint16_t PREV_STATE = START_STATE;
volatile int MenuItem = 0;
volatile uint8_t MenuSwitchDelay = 0;

volatile uint8_t TFT_FLAG = 0x00;
volatile uint8_t Counter = 0;
volatile uint8_t CounterLED = 0;

volatile double k[16] = {5.0,8.0,12.5,18.5,27.0,38.3,54.3,77.0,109.0,146.0,187.0,239.0,305.0,390.0,498.0,635.0};
volatile double M[16] = {29.6,25.4,22.5,20.3,18.5,16.9,15.9,15.2,14.7,14.3,14.0,13.7,13.4,13.1,12.9,12.7};
volatile double dM[16] = {1.7928,1.5352,1.3847,1.2780,1.2306,1.1857,1.1504,1.1223,1.0999,1.0844,1.0731,1.0635,1.0552,1.0478,1.0414,1.0359};

volatile double k_He[16] = {1.88,3.02,4.72,6.99,10.21,14.48,20.53,29.11,41.20,55.19,70.69,90.34,115.29,147.42,188.24,240.03};
volatile double M_He[16] = {37.2,31.2,27.2,24.3,22.4,20.8,19.4,18.2,17.4,16.8,16.4,16.2,16.1,16.1,16.0,15.9};
volatile double dM_He[16] = {2.0964,1.7400,1.5321,1.3845,1.3189,1.2568,1.2079,1.1692,1.1419,1.1232,1.1115,1.1022,1.0963,1.0904,1.0850,1.0791};

volatile double NDL = 1000.0;

volatile uint8_t Emulation = 0;
volatile uint8_t Flash_WR = 0;

volatile uint8_t DiveMODE = NODIVE_STATE;

double Altitude = 0.0;
double P_atm = 10.0;
double Delta_H = 0.0;
double CN2 = 0.79;
double CO2 = 0.21;
double CHe = 0.0;
double PO2_lim = 1.4;
volatile double PN[16];
volatile double PHe[16];
double CNS = 0.0;
double DecoStop = 0.0;

uint32_t StartTime = 0;//*//
uint32_t StartSurfTime = 0;
uint32_t CurrentTime = 0;
uint32_t CurrentSurfTime = 0;
uint32_t PrevTime = 0;//*//
double SensCalibr = 1000.0;
double CurrentData[2];

double Velocity = 0.0;
double PrevDepth = 0.0;

void changePO2Lim(int incr)
{
	if (incr==1)
	{
		if (PO2_lim < 2.0) PO2_lim = PO2_lim + 0.05;
		else PO2_lim = 1.0;
	}
	else
	{
		if (PO2_lim > 1.05) PO2_lim = PO2_lim - 0.05;
		else PO2_lim = 1.0;
	}
}

void changeOxygen(int incr)
{
	if (incr==1)
	{
		if (CO2 < 0.5) CO2 = CO2 + 0.01;
		else CO2 = 0.5;
	}
	else
	{
		if (CO2 > 0.05) CO2 = CO2 - 0.01;
		else CO2 = 0.05;
	}
	CN2 = 1 - CHe - CO2;
}

void changeHelium(int incr)
{
	if (incr==1)
	{
		if (CHe < 0.8) CHe = CHe + 0.01;
		else CHe = 0.0;
	}
	else
	{
		if (CHe>0.01) CHe = CHe - 0.01;
		else CHe = 0.0;
	}
	CN2 = 1 - CHe - CO2;
}

void DrawMenu(int8_t MenuState)
{
	uint32_t color = TFT_rgb2color(255,255,255);
	uint32_t backcolor = TFT_rgb2color(0,0,0);
	char AltStr[6];
	char POLimStr[5];
	char PO2Str[5];
	char PHeStr[5];
	TFT_DMA_fillRect(0,0,319,239,backcolor);

	if (Altitude < 0) AltToStr(AltStr,-Altitude);
	else AltToStr(AltStr,Altitude);
	TFT_DMA_DrawString("Altitude: ",10,30,10,color,backcolor,1);
	TFT_DMA_DrawString(AltStr,6,200,10,color,backcolor,1);

	TempToStr(POLimStr,PO2_lim);
	TFT_DMA_DrawString("Set PO2 limit: ",15,30,45,color,backcolor,1);
	TFT_DMA_DrawString(POLimStr,5,200,45,color,backcolor,1);

	TempToStr(PO2Str,100*CO2);
	TFT_DMA_DrawString("Set PO2: ",9,30,80,color,backcolor,1);
	TFT_DMA_DrawString(PO2Str,5,200,80,color,backcolor,1);

	TempToStr(PHeStr,100*CHe);
	TFT_DMA_DrawString("Set He: ",8,30,115,color,backcolor,1);
	TFT_DMA_DrawString(PHeStr,5,200,115,color,backcolor,1);

	if (Emulation) TFT_DMA_DrawString("Emulation: YES",14,30,150,color,backcolor,1);
	else TFT_DMA_DrawString("Emulation: NO",13,30,150,color,backcolor,1);

	TFT_DMA_DrawString("Reset time?",11,30,185,color,backcolor,1);

	TFT_DMA_DrawString("Set connection?",15,30,220,color,backcolor,1);

	switch (MenuItem)
	{
		case MENU_ALTITUDE:
			TFT_DMA_fillRect(0,10,20,30,TFT_rgb2color(255,255,0));
			break;
		case MENU_SETOXLIM:
			TFT_DMA_fillRect(0,45,20,65,TFT_rgb2color(255,255,0));
			break;
		case MENU_SETOX:
			TFT_DMA_fillRect(0,80,20,100,TFT_rgb2color(255,255,0));
			break;
		case MENU_SETHE:
			TFT_DMA_fillRect(0,115,20,135,TFT_rgb2color(255,255,0));
			break;
		case MENU_SETEMUL:
			TFT_DMA_fillRect(0,150,20,170,TFT_rgb2color(255,255,0));
			break;
		case MENU_SETTIME:
			TFT_DMA_fillRect(0,185,20,205,TFT_rgb2color(255,255,0));
			break;
		case MENU_SETCONN:
			if (STATE==MENU_STATE) TFT_DMA_fillRect(0,220,20,240,TFT_rgb2color(255,255,0));
			if (STATE==CONNECTION_STATE) TFT_DMA_fillRect(0,220,20,240,TFT_rgb2color(0,255,0));
			break;
		default:
			break;
	}
}

void GetSensorMeasurements(void)
{
	double Data[2];
	MS5803_Init();
	MS5803_reset();
	MS5803_begin();
	MS5803_getFullData(CELSIUS,ADC_4096,Data);
	MS5803_Deinit();
	CurrentData[0] = Data[0];
	CurrentData[1] = (Data[1]-SensCalibr)/101.325;
}

void RedrawValues(void)
{
	uint16_t color = TFT_rgb2color(255,255,255);
	uint16_t backcolor = TFT_rgb2color(0,0,0);
	char TStr[5] = "     ";
	char HStr[5] = "     ";
	char TmStr[8] = "        ";
	char CNSStr[6] = "     %";
	char StateStr[2] = "  ";

	TempToStr(TStr, CurrentData[0]);
	TempToStr(HStr, CurrentData[1]);

	TFT_DMA_DrawString(TStr, 5, 35, 75, color, backcolor,1);
	TFT_DMA_DrawString(HStr, 5, 30, 145, color, backcolor,2);

	DT = GetDate();
	itoa(DT.Day,StateStr,10);
	TFT_DMA_DrawString(StateStr, 2, 110, 115, color, backcolor,1);
	switch(DT.Month)
	{
		case 1:
			TFT_DMA_DrawString("JAN", 3, 135, 115, color, backcolor,1);
			break;
		case 2:
			TFT_DMA_DrawString("FEB", 3, 135, 115, color, backcolor,1);
			break;
		case 3:
			TFT_DMA_DrawString("MAR", 3, 135, 115, color, backcolor,1);
			break;
		case 4:
			TFT_DMA_DrawString("APR", 3, 135, 115, color, backcolor,1);
			break;
		case 5:
			TFT_DMA_DrawString("MAY", 3, 135, 115, color, backcolor,1);
			break;
		case 6:
			TFT_DMA_DrawString("JUN", 3, 135, 115, color, backcolor,1);
			break;
		case 7:
			TFT_DMA_DrawString("JUL", 3, 135, 115, color, backcolor,1);
			break;
		case 8:
			TFT_DMA_DrawString("AUG", 3, 135, 115, color, backcolor,1);
			break;
		case 9:
			TFT_DMA_DrawString("SEP", 3, 135, 115, color, backcolor,1);
			break;
		case 10:
			TFT_DMA_DrawString("OCT", 3, 135, 115, color, backcolor,1);
			break;
		case 11:
			TFT_DMA_DrawString("NOV", 3, 135, 115, color, backcolor,1);
			break;
		case 12:
			TFT_DMA_DrawString("DEC", 3, 135, 115, color, backcolor,1);
			break;
		default:
			break;
	}
	switch(DT.WeekDay)
	{
		case 1:
			TFT_DMA_DrawString("MON", 3, 120, 140, color, backcolor,1);
			break;
		case 2:
			TFT_DMA_DrawString("TUE", 3, 120, 140, color, backcolor,1);
			break;
		case 3:
			TFT_DMA_DrawString("WED", 3, 120, 140, color, backcolor,1);
			break;
		case 4:
			TFT_DMA_DrawString("THU", 3, 120, 140, color, backcolor,1);
			break;
		case 5:
			TFT_DMA_DrawString("FRI", 3, 120, 140, color, backcolor,1);
			break;
		case 6:
			TFT_DMA_DrawString("SAT", 3, 120, 140, color, backcolor,1);
			break;
		case 7:
			TFT_DMA_DrawString("SUN", 3, 120, 140, color, backcolor,1);
			break;
		default:
			break;
	}
	itoa(DT.Year,HStr,10);
	TFT_DMA_DrawString(HStr, 4, 115, 165, color, backcolor,1);

	uint32_t T,H,M,S;
	if (DiveMODE==DECO_STATE)
	{
		TempToStr(HStr, DecoStop);
		TFT_DMA_DrawString(HStr, 5, 210, 145, color, backcolor,2);
	}
	else
	{
		if (DiveMODE==NODIVE_STATE || DiveMODE==SURF_STATE) T = (uint32_t)(RTC_GetCounter()>>8);
		else T = (uint32_t)(NDL*60.0);
		H = (uint32_t)(T/3600);
		M = (uint32_t)(T/60) - 60*H;
		S = T - 3600*H - 60*M;
		TimeToStr(TmStr,H,M,S);
		TFT_DMA_DrawString(TmStr, 8, 180, 145, color, backcolor,2);
	}
	if (DiveMODE==DIVE_STATE||DiveMODE==DECO_STATE)
	{
		H = (uint32_t)(CurrentTime/3600);
		M = (uint32_t)(CurrentTime/60) - 60*H;
		S = CurrentTime - 3600*H - 60*M;
		TimeToStr(TmStr,H,M,S);
		TFT_DMA_DrawString(TmStr, 8, 230, 75, color, backcolor,1);
		TempToStr(HStr, Velocity);
		TFT_DMA_DrawString(HStr, 5, 140, 75, color, backcolor,1);
	}
	if (DiveMODE==SURF_STATE)
	{
		H = (uint32_t)(CurrentSurfTime/3600);
		M = (uint32_t)(CurrentSurfTime/60) - 60*H;
		S = CurrentSurfTime - 3600*H - 60*M;
		TimeToStr(TmStr,H,M,S);
		TFT_DMA_DrawString(TmStr, 8, 230, 75, color, backcolor,1);
	}
	if (DiveMODE!=NODIVE_STATE)
	{
		DrawNitrogenSaturation();
		DrawHeliumSaturation();
		TempToStr(CNSStr, 100*CNS);
		TFT_DMA_DrawString(CNSStr,6,230,218,color,backcolor,1);
	}
}

void drawSetData(void)
{
	TFT_DMA_fillRect(0,0,319,239,TFT_rgb2color(0,0,0));
	char DD[2] = "  ";
	char DY[4] = "    ";
	TFT_DMA_DrawString("Set data:", 9, 20, 20, TFT_rgb2color(255,255,255), TFT_rgb2color(0,0,0),2);
	TFT_DMA_DrawString("Day:", 4, 20, 60, TFT_rgb2color(255,255,255), TFT_rgb2color(0,0,0),2);
	TFT_DMA_DrawString("Week day:", 9, 20, 100, TFT_rgb2color(255,255,255), TFT_rgb2color(0,0,0),2);
	TFT_DMA_DrawString("Month:", 6, 20, 140, TFT_rgb2color(255,255,255), TFT_rgb2color(0,0,0),2);
	TFT_DMA_DrawString("Year:", 5, 20, 180, TFT_rgb2color(255,255,255), TFT_rgb2color(0,0,0),2);
	itoa(DT.Day,DD,10);
	itoa(DT.Year,DY,10);
	switch (DT.WeekDay)
	{
		case 1:
			if (STATE==SETWEEKDAY_STATE) TFT_DMA_DrawString("MON", 3, 160, 100, TFT_rgb2color(0,0,0), TFT_rgb2color(255,255,0),2);
			else TFT_DMA_DrawString("MON", 3, 160, 100, TFT_rgb2color(255,255,0), TFT_rgb2color(0,0,0),2);
			break;
		case 2:
			if (STATE==SETWEEKDAY_STATE) TFT_DMA_DrawString("TUE", 3, 160, 100, TFT_rgb2color(0,0,0), TFT_rgb2color(255,255,0),2);
			else TFT_DMA_DrawString("TUE", 3, 160, 100, TFT_rgb2color(255,255,0), TFT_rgb2color(0,0,0),2);
			break;
		case 3:
			if (STATE==SETWEEKDAY_STATE) TFT_DMA_DrawString("WED", 3, 160, 100, TFT_rgb2color(0,0,0), TFT_rgb2color(255,255,0),2);
			else TFT_DMA_DrawString("WED", 3, 160, 100, TFT_rgb2color(255,255,0), TFT_rgb2color(0,0,0),2);
			break;
		case 4:
			if (STATE==SETWEEKDAY_STATE) TFT_DMA_DrawString("THU", 3, 160, 100, TFT_rgb2color(0,0,0), TFT_rgb2color(255,255,0),2);
			else TFT_DMA_DrawString("THU", 3, 160, 100, TFT_rgb2color(255,255,0), TFT_rgb2color(0,0,0),2);
			break;
		case 5:
			if (STATE==SETWEEKDAY_STATE) TFT_DMA_DrawString("FRI", 3, 160, 100, TFT_rgb2color(0,0,0), TFT_rgb2color(255,255,0),2);
			else TFT_DMA_DrawString("FRI", 3, 160, 100, TFT_rgb2color(255,255,0), TFT_rgb2color(0,0,0),2);
			break;
		case 6:
			if (STATE==SETWEEKDAY_STATE) TFT_DMA_DrawString("SAT", 3, 160, 100, TFT_rgb2color(0,0,0), TFT_rgb2color(255,255,0),2);
			else TFT_DMA_DrawString("SAT", 3, 160, 100, TFT_rgb2color(255,255,0), TFT_rgb2color(0,0,0),2);
			break;
		case 7:
			if (STATE==SETWEEKDAY_STATE) TFT_DMA_DrawString("SUN", 3, 160, 100, TFT_rgb2color(0,0,0), TFT_rgb2color(255,255,0),2);
			else TFT_DMA_DrawString("SUN", 3, 160, 100, TFT_rgb2color(255,255,0), TFT_rgb2color(0,0,0),2);
			break;
		default:
			break;
	}
	switch (DT.Month)
	{
		case 1:
			if (STATE==SETMON_STATE) TFT_DMA_DrawString("JAN", 3, 160, 140, TFT_rgb2color(0,0,0), TFT_rgb2color(255,255,0),2);
			else TFT_DMA_DrawString("JAN", 3, 160, 140, TFT_rgb2color(255,255,0), TFT_rgb2color(0,0,0),2);
			break;
		case 2:
			if (STATE==SETMON_STATE) TFT_DMA_DrawString("FEB", 3, 160, 140, TFT_rgb2color(0,0,0), TFT_rgb2color(255,255,0),2);
			else TFT_DMA_DrawString("FEB", 3, 160, 140, TFT_rgb2color(255,255,0), TFT_rgb2color(0,0,0),2);
			break;
		case 3:
			if (STATE==SETMON_STATE) TFT_DMA_DrawString("MAR", 3, 160, 140, TFT_rgb2color(0,0,0), TFT_rgb2color(255,255,0),2);
			else TFT_DMA_DrawString("MAR", 3, 160, 140, TFT_rgb2color(255,255,0), TFT_rgb2color(0,0,0),2);
			break;
		case 4:
			if (STATE==SETMON_STATE) TFT_DMA_DrawString("APR", 3, 160, 140, TFT_rgb2color(0,0,0), TFT_rgb2color(255,255,0),2);
			else TFT_DMA_DrawString("APR", 3, 160, 140, TFT_rgb2color(255,255,0), TFT_rgb2color(0,0,0),2);
			break;
		case 5:
			if (STATE==SETMON_STATE) TFT_DMA_DrawString("MAY", 3, 160, 140, TFT_rgb2color(0,0,0), TFT_rgb2color(255,255,0),2);
			else TFT_DMA_DrawString("MAY", 3, 160, 140, TFT_rgb2color(255,255,0), TFT_rgb2color(0,0,0),2);
			break;
		case 6:
			if (STATE==SETMON_STATE) TFT_DMA_DrawString("JUN", 3, 160, 140, TFT_rgb2color(0,0,0), TFT_rgb2color(255,255,0),2);
			else TFT_DMA_DrawString("JUN", 3, 160, 140, TFT_rgb2color(255,255,0), TFT_rgb2color(0,0,0),2);
			break;
		case 7:
			if (STATE==SETMON_STATE) TFT_DMA_DrawString("JUL", 3, 160, 140, TFT_rgb2color(0,0,0), TFT_rgb2color(255,255,0),2);
			else TFT_DMA_DrawString("JUL", 3, 160, 140, TFT_rgb2color(255,255,0), TFT_rgb2color(0,0,0),2);
			break;
		case 8:
			if (STATE==SETMON_STATE) TFT_DMA_DrawString("AUG", 3, 160, 140, TFT_rgb2color(0,0,0), TFT_rgb2color(255,255,0),2);
			else TFT_DMA_DrawString("AUG", 3, 160, 140, TFT_rgb2color(255,255,0), TFT_rgb2color(0,0,0),2);
			break;
		case 9:
			if (STATE==SETMON_STATE) TFT_DMA_DrawString("SEP", 3, 160, 140, TFT_rgb2color(0,0,0), TFT_rgb2color(255,255,0),2);
			else TFT_DMA_DrawString("SEP", 3, 160, 140, TFT_rgb2color(255,255,0), TFT_rgb2color(0,0,0),2);
			break;
		case 10:
			if (STATE==SETMON_STATE) TFT_DMA_DrawString("OCT", 3, 160, 140, TFT_rgb2color(0,0,0), TFT_rgb2color(255,255,0),2);
			else TFT_DMA_DrawString("OCT", 3, 160, 140, TFT_rgb2color(255,255,0), TFT_rgb2color(0,0,0),2);
			break;
		case 11:
			if (STATE==SETMON_STATE) TFT_DMA_DrawString("NOV", 3, 160, 140, TFT_rgb2color(0,0,0), TFT_rgb2color(255,255,0),2);
			else TFT_DMA_DrawString("NOV", 3, 160, 140, TFT_rgb2color(255,255,0), TFT_rgb2color(0,0,0),2);
			break;
		case 12:
			if (STATE==SETMON_STATE) TFT_DMA_DrawString("DEC", 3, 160, 140, TFT_rgb2color(0,0,0), TFT_rgb2color(255,255,0),2);
			else TFT_DMA_DrawString("DEC", 3, 160, 140, TFT_rgb2color(255,255,0), TFT_rgb2color(0,0,0),2);
			break;
		default:
			break;
	}
	if (STATE==SETDAY_STATE) TFT_DMA_DrawString(DD, 2, 160, 60, TFT_rgb2color(0,0,0), TFT_rgb2color(255,255,0),2);
	else TFT_DMA_DrawString(DD, 2, 160, 60, TFT_rgb2color(255,255,0), TFT_rgb2color(0,0,0),2);
	if (STATE==SETYEAR_STATE) TFT_DMA_DrawString(DY, 4, 160, 180, TFT_rgb2color(0,0,0), TFT_rgb2color(255,255,0),2);
	else TFT_DMA_DrawString(DY, 4, 160, 180, TFT_rgb2color(255,255,0), TFT_rgb2color(0,0,0),2);
}

void drawSetTime(void)
{
	TFT_DMA_fillRect(0,0,319,239,TFT_rgb2color(0,0,0));
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

    if (TFT_FLAG==0x00)
    {
    //	if (CounterLED==0) GPIOB->BSRR = GPIO_BSRR_BS14;
    //	else GPIOB->BSRR = GPIO_BSRR_BR14;
    //	if (CounterLED>=3) CounterLED = 0;
    //	else CounterLED++;

    	//if (Counter>=127) Counter = 0;
    	//else Counter++;
    }
    //else Counter = 127;
	if (Counter>=127) Counter = 0;
	else Counter++;
  }
}

void RTC_Configuration(void)
{
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE); /* Enable PWR and BKP clocks */

  PWR_BackupAccessCmd(ENABLE); /* Allow access to BKP Domain */

  //if ((PWR_GetFlagStatus(PWR_FLAG_SB) != RESET) || ((BKP->DR1&0x01)==0x01)) /* Check if the StandBy flag is set */
  /*if ((BKP->DR1&0x01)==0x01)
  {
	  STATE = RUN_STATE;
	  PREV_STATE = START_STATE;
    PWR_ClearFlag(PWR_FLAG_SB); */
  /*  RTC_WaitForSynchro();
  }*/
  if ((BKP->DR1&0x02)==0x00)
  {
	  /*uint32_t ADDR;
	  FLASH_Unlock();
	  for (ADDR = BASE_ADDRESS; ADDR<=END_ADDRESS; ADDR+=0x400)
	  {
		  flash_erase_page(ADDR);
	  }
	  FLASH_Lock();*/
	  set_current_logaddr(BASE_ADDRESS);
	  BKP->DR1 |= 0x02;
  }
  if ((BKP->DR1&0x01)==0x00)
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
    BKP_SetRTCCalibrationValue(0x00);
   // RTC_WaitForLastTask();
    STATE = SETDAY_STATE;
    PREV_STATE = START_STATE;
    BKP->DR1 |= 0x01;
  }
  else
  {
	  STATE = RUN_STATE;
	  PREV_STATE = START_STATE;
	  BKP_SetRTCCalibrationValue(0x00);
	  //RTC_WaitForLastTask();
	  RTC_WaitForSynchro();
  }
  RTC_ITConfig(RTC_IT_ALR, ENABLE);
  RTC_WaitForLastTask();
}

void DrawDiveDesktop(void)
{
	uint16_t i = 0;
	uint16_t color = TFT_rgb2color(255,127,0);
	uint16_t backcolor = TFT_rgb2color(0,0,0);
	TFT_DMA_fillRect(0,0,319,239,backcolor);
	TFT_DMA_drawRect(0,40,319,239,color);
	TFT_DMA_drawRect(1,41,318,238,color);
	TFT_DMA_drawHorizontalLine(0,319,102,color);
	TFT_DMA_drawHorizontalLine(0,319,103,color);
	TFT_DMA_drawHorizontalLine(0,319,192,color);
	TFT_DMA_drawHorizontalLine(0,319,193,color);
	color = TFT_rgb2color(0,255,0);
	for (i = 0; i<16; i++)
	{
		TFT_DMA_drawRect(20*i,0,20*(i+1)-1,39,color);
	}
	color = TFT_rgb2color(255,255,0);
	TFT_DMA_DrawString("Depth, m:",9,20,115,color,backcolor,1);

	if (DiveMODE==DIVE_STATE)
	{
		TFT_DMA_DrawString("Non-Deco:",9,190,115,color,backcolor,1);
		TFT_DMA_DrawString("Dive Time:",10,220,50,color,backcolor,1);
	}
	else if (DiveMODE==DECO_STATE)
	{
		TFT_DMA_DrawString("Deco Stop:",10,190,115,color,backcolor,1);
		TFT_DMA_DrawString("Dive Time:",10,220,50,color,backcolor,1);
	}
	else
	{
		TFT_DMA_DrawString("Global Time:",12,190,115,color,backcolor,1);
		TFT_DMA_DrawString("Surf time:",10,220,50,color,backcolor,1);
	}

	TFT_DMA_DrawString("CNS  O    :",11,220,195,color,backcolor,1);
	TFT_DMA_DrawString("2",1,267,199,color,backcolor,1);
	TFT_DMA_DrawString("O",1,144,195,color,backcolor,1);
	TFT_DMA_DrawString("2",1,155,199,color,backcolor,1);
	TFT_DMA_DrawString("Max Depth:",10,10,195,color,backcolor,1);
	TFT_DMA_DrawString("T,   C",6,30,50,color,backcolor,1);
	TFT_DMA_DrawString("o",1,47,42,color,backcolor,1);
	TFT_DMA_DrawString("Speed:",6,130,50,color,backcolor,1);
	color = TFT_rgb2color(255,255,255);

	char O2Printout[7] = "      %";
	char MaxDepthPrintout[7] = "      m";
	TempToStr(O2Printout,CO2*100);
	TempToStr(MaxDepthPrintout,10*PO2_lim/CO2-10.0);
	TFT_DMA_DrawString(O2Printout,7,140,218,color,backcolor,1);
	TFT_DMA_DrawString(MaxDepthPrintout,7,30,218,color,backcolor,1);
}

double get_M(int index)
{
	return M[index] - Delta_H*dM[index];
}

double get_M_He(int index)
{
	return M_He[index] - Delta_H*dM_He[index];
}

void setAltitude()
{
	double Data[2];
	MS5803_Init();
	MS5803_reset();
	MS5803_begin();
	MS5803_getFullData(CELSIUS,ADC_4096,Data);
	MS5803_Deinit();
	SensCalibr = (double)Data[1];
	P_atm = SensCalibr/101.325;
	Altitude = 29.271263*log(10.0/P_atm)*(273.15+((double)Data[0]/10));
	Delta_H = 10.0 - P_atm;
}

double PartialPressure(double H, double C)
{
    return (H + P_atm - 0.627)*C;
}

double getMaxOxygenTime(double PO)
{
	return 1684.41 - 1064.32*PO - 2360.09*pow(PO,2) + 2933.3*pow(PO,3) - 901.44230769*pow(PO,4);
}

void ResetPN()
{
	int i = 0;
	for (i = 0; i<16; i++)
	{
		PN[i] = PartialPressure(0.0, 0.79);
		PHe[i] = 0.0;
	}
}

void PN_Step(double H, double dt)
{
	int i = 0;
	for (i = 0; i<16; i++)
	{
		double pn = PN[i];
		double ph = PHe[i];
		PN[i] = pn*exp(-k[i]*dt) + k[i]*dt*PartialPressure(H,CN2);
		PHe[i] = ph*exp(-k_He[i]*dt) + k_He[i]*dt*PartialPressure(H,CHe);
		/*if ((DiveMODE = DIVE_STATE) && (PN[i]>=get_M(i) || PHe[i]>=get_M_He(i)))
		{
			DiveMODE = DECO_STATE;
			DrawDiveDesktop();
		}*/
	}
}

double NDLcalc(double Depth, double Hn, double Mm, double kk)
{
    if (Depth > Mm)
    {
    	double t = (1/kk)*log((Depth-Hn)/(Depth-Mm));
		if (t>1000)
		{
			t = 1000.0;
		}
		return t;
    }
    else return 1000.0;
}

void UpdateNDL(double H)
{
	double time = NDLcalc(PartialPressure(H,CN2), PN[0], get_M(0), k[0]);
	double time_He = NDLcalc(PartialPressure(H,CHe), PHe[0], get_M_He(0), k_He[0]);
	if (time_He < time) time = time_He;
	int i = 1;
	for (i = 1; i<16; i++)
	{
		double NDLT1 = NDLcalc(PartialPressure(H,CN2), PN[i], get_M(i), k[i]);
		double NDLT2 = NDLcalc(PartialPressure(H,CHe), PHe[i], get_M_He(i), k_He[i]);
		if (NDLT1<=time)
		{
			time = NDLT1;
		}
		if (NDLT2<=time)
		{
			time = NDLT2;
		}
	}
	NDL = time;
	if (NDL<=0.0)
	{
		DiveMODE = DECO_STATE;
		DrawDiveDesktop();
	}
}

void UpdateDeco(void)
{
	DecoStop = (PN[0]-get_M(0))/dM[0];
	double dst;
	int i = 1;
	for (i=1;i<16;i++)
	{
		dst = (PN[i]-get_M(i))/dM[i];
		if (dst>DecoStop)
		{
			DecoStop = dst;
		}
	}
	for (i=0;i<16;i++)
	{
		dst = (PHe[i]-get_M_He(i))/dM_He[i];
		if (dst>DecoStop)
		{
			DecoStop = dst;
		}
	}
	if (DecoStop<=0.0)
	{
		DiveMODE = DIVE_STATE;
		DrawDiveDesktop();
	}
}

void CalculateValues(double H, double dt)
{
	PN_Step(H, dt);
	if (DiveMODE==DECO_STATE)
	{
		UpdateDeco();
	}
	else
	{
		UpdateNDL(H);
	}
	double PO2 = (H + P_atm)*CO2/10.0;
	if (PO2>=0.5)
	{
		CNS = CNS + dt/getMaxOxygenTime(PO2);
	}
	else if (DiveMODE==SURF_STATE)
	{
		CNS = CNS*(1-0.0077*dt); //HT = 90 min
	}
}

void DrawNitrogenSaturation()
{
	double sat;
	int i = 0;
	for (i=0; i<16; i++)
	{
		sat = (PN[i]-PartialPressure(0.0, 0.79))/(get_M(i)-PartialPressure(0.0, 0.79));
		if (sat>=1.0) sat = 1.0;
		TFT_DMA_fillRect(1+20*i,1,8+20*i,38,TFT_rgb2color(0,0,0));
		TFT_DMA_fillRect(1+20*i,38,8+20*i,38-(int)(37*sat),TFT_rgb2color(255,0,0));
	}
}

void DrawHeliumSaturation()
{
	double sat;
	int i = 0;
	for (i=0; i<16; i++)
	{
		sat = PHe[i]/get_M_He(i);
		if (sat>=1.0) sat = 1.0;
		TFT_DMA_fillRect(10+20*i,1,18+20*i,38,TFT_rgb2color(0,0,0));
		TFT_DMA_fillRect(10+20*i,38,18+20*i,38-(int)(37*sat),TFT_rgb2color(255,255,0));
	}
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

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2ENR_AFIOEN, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2;		// INPUT BUTTONS
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	RCC_ClearFlag();

	NVIC_Configuration();
	  /* Configure EXTI Line to generate an interrupt on falling edge */
	EXTI_Configuration();

	AFIO->EXTICR[0] = 0x00000000;

	NVIC_EnableIRQ (EXTI2_IRQn);
	NVIC_EnableIRQ (EXTI1_IRQn);
	NVIC_EnableIRQ (EXTI0_IRQn);

	RTC_Configuration();

	ili9341_init();

	int i=0;
	for (i=0; i<16; i++)
	{
		k[i] = log(2)/k[i];
		k_He[i] = log(2)/k_He[i];
	}

	DiveMODE = NODIVE_STATE;
	setAltitude();
	ResetPN();

	//USART1_ON();
	//send_to_uart(0x01);
	DrawDiveDesktop();
	//send_to_uart(0x02);
	//USART1_OFF();
	//Delay_ms(1000);

	uint32_t T;

    while(1)
    {
    	if ((STATE==SETDAY_STATE)||(STATE==SETWEEKDAY_STATE)||(STATE==SETMON_STATE)||(STATE==SETYEAR_STATE))
		{
			if (PREV_STATE==MENU_STATE)
			{
				Delay_ms(500);
				init_clock();
				GPIOB->BSRR = GPIO_BSRR_BS12 | GPIO_BSRR_BS13 | GPIO_BSRR_BS15;
				ili9341_init();
				PREV_STATE = START_STATE;
				Delay_ms(500);
			}
			drawSetData();
			Delay_ms(500);
		}
		else if ((STATE==SETHOUR_STATE)||(STATE==SETMINUTE_STATE)||(STATE==SETSECOND_STATE))
		{
			if (PREV_STATE==MENU_STATE)
			{
				Delay_ms(500);
				init_clock();
				GPIOB->BSRR = GPIO_BSRR_BS12 | GPIO_BSRR_BS13 | GPIO_BSRR_BS15;
				ili9341_init();
				PREV_STATE = START_STATE;
				Delay_ms(500);
			}
			drawSetTime();
			Delay_ms(500);
		}
		else if (STATE==TIMEUPDATE_STATE)
		{
			Delay_ms(500);
			StoreDate(DT);
			RTC_SetCounter(((3600*HOUR+60*MINUTE+SECOND)<<8));
			RTC_WaitForLastTask();
			set_current_logaddr(BASE_ADDRESS);
			PREV_STATE = TIMEUPDATE_STATE;
			STATE = RUN_STATE;
			TFT_DMA_fillRect(0,0,319,239,TFT_rgb2color(0,0,0));
			BKP->DR1 = 0x01;
			DrawDiveDesktop();
		}
		else if (STATE==ALTCALIBR_STATE)
		{
			//Delay_ms(500);
			GPIOB->BSRR = GPIO_BSRR_BS12 | GPIO_BSRR_BS13 | GPIO_BSRR_BS15;
			setAltitude();
			ResetPN();
			//BKP->DR4 = 0x0000;//==========
			//BKP->DR1 = 0x0000;
			//set_current_logaddr(BASE_ADDRESS);//==========
			STATE = RUN_STATE;
			PREV_STATE = ALTCALIBR_STATE;
		}
		else
		{
			if (TFT_FLAG==0x01)
			{
				init_clock();

				GPIOB->BSRR |= GPIO_BSRR_BS12 | GPIO_BSRR_BS13 | GPIO_BSRR_BS15;
				TFT_sendCMD(0x11);
				//SPI_ON();
				Delay_ms(500);
				ili9341_init();
				TFT_FLAG=0x00;
				DrawDiveDesktop();
			}
			if (TFT_FLAG==0x11)
			{
				TFT_sendCMD(0x10);
				Delay_ms(500);
				//SPI_OFF();
				GPIOB->BSRR &= ~(GPIO_BSRR_BS12 | GPIO_BSRR_BS13 | GPIO_BSRR_BS15);
				TFT_FLAG = 0x10;
			}
			else if ((STATE&0x3000)!=0x3000)//PWR_FLAG==0)
			{
				if (Counter>=127)
				{
					init_clock();
					//if (TFT_FLAG==0x00) {GPIOB->BSRR |= GPIO_BSRR_BR12 | GPIO_BSRR_BR13 | GPIO_BSRR_BR15;}
					T = RTC_GetCounter();
					if (T>=0x01517FFF)
					{
						DT = GetDate();
						int i;
						int skipdays = (int)(T/0x01517FFF);
						for (i=0;i<skipdays;i++)
						{
							DT = IncrementDay(DT);
						}
						StoreDate(DT);
						T = T%0x01517FFF;
						RTC_SetCounter(T-2253*(skipdays));
						RTC_WaitForLastTask();
					}

					GetSensorMeasurements();
					if (Emulation==1) CurrentData[1] = 55.0;

					if (DiveMODE!=NODIVE_STATE)
					{
						double dt = ((double)((T>>8)-PrevTime))/60.0;
						if (DiveMODE==DIVE_STATE && CurrentData[1] < 12.7) CalculateValues(12.75, dt);
						else CalculateValues(CurrentData[1], dt);
						PrevTime = T>>8;
						CurrentTime = PrevTime - StartTime;
						if (DiveMODE==SURF_STATE)
						{
							CurrentSurfTime = PrevTime - StartSurfTime;
						}
						Velocity = (CurrentData[1]-PrevDepth)/dt;
						PrevDepth = CurrentData[1];
						if (DiveMODE!=SURF_STATE && CurrentData[1]<0.4)
						{
							DiveMODE = SURF_STATE;
							if (TFT_FLAG==0x00) {DrawDiveDesktop();}
							CurrentSurfTime = 0;
							StartSurfTime = T>>8;
							FLASH_Unlock();
							log_current_point(get_current_logaddr(),0,2048);
							FLASH_Lock();
							uint16_t Dives = BKP->DR4;
							BKP->DR4 = (uint16_t)(Dives + 1);
						}
						else if (DiveMODE!=SURF_STATE)
						{
							if (CurrentTime%5==0)
							{
								FLASH_Unlock();
								log_current_point(get_current_logaddr(),CurrentTime,CurrentData[1]);
								FLASH_Lock();
							}
						}
					}
					if ((DiveMODE==NODIVE_STATE || DiveMODE==SURF_STATE) && CurrentData[1]>0.5)
					{
						DiveMODE = DIVE_STATE;
						if (TFT_FLAG==0x00) {DrawDiveDesktop();}
						CurrentTime = 0;
						StartTime = T>>8;
						PrevTime = StartTime;
						FLASH_Unlock();
						DT = GetDate();//-----------------------------------
						uint32_t SendDate = ((uint32_t)DT.Day)*10000 + ((uint32_t)DT.Month)*100 + (((uint32_t)DT.Year)-2000);
						log_current_point(get_current_logaddr(),0,SendDate);//-----------------------
						log_current_point(get_current_logaddr(),CurrentTime,CurrentData[1]);
						FLASH_Lock();
					}
					if (STATE==RUN_STATE && TFT_FLAG==0x00)
					{
						if (PREV_STATE==MENU_STATE)
						{
							PREV_STATE = RUN_STATE;
							DrawDiveDesktop();
							Delay_ms(500);
						}
						RedrawValues();
					}
					if (STATE==MENU_STATE || STATE==CONNECTION_STATE)
					{
						if (MenuSwitchDelay==1)
						{
							Delay_ms(500);
							MenuSwitchDelay = 0;
						}
						if (TFT_FLAG==0x00) {DrawMenu(MenuItem);}

						if (STATE==CONNECTION_STATE)
						{
							USART1_ON();
							uint16_t Dives = BKP->DR4;
							send16_to_uart(Dives);
							uint32_t ADDR = get_current_logaddr();
							send32_to_uart(ADDR);
							if (ADDR > BASE_ADDRESS)
							{
								int i=0;
								uint32_t Pnts = (uint32_t)((ADDR-BASE_ADDRESS)/8);
								send32_to_uart(Pnts);
								uint32_t DATA1,DATA2;
								for (i=0;i<Pnts;i++)
								{
									DATA1 = flash_read(BASE_ADDRESS+8*i);
									DATA2 = flash_read(BASE_ADDRESS+8*i+4);
									send32_to_uart(DATA1);
									send32_to_uart(DATA2);
								}
							}
							Delay_ms(100);
							USART1_OFF();
							STATE = MENU_STATE;
							PREV_STATE = CONNECTION_STATE;
						}
					}
				}

				RTC_SetAlarm(RTC_GetCounter()+1);
				RTC_WaitForLastTask();
				PWR_EnterSTOPMode(PWR_Regulator_LowPower, PWR_STOPEntry_WFI);
			}
			else if ((STATE&0x3000)==0x3000)//(PWR_FLAG == 1)
			{
				init_clock();
				Delay_ms(400);
				GPIOB->BSRR = GPIO_BSRR_BR12 | GPIO_BSRR_BR13 | GPIO_BSRR_BR15;
				GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_15;
				GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
				GPIO_Init(GPIOB, &GPIO_InitStructure);
				PWR_WakeUpPinCmd(ENABLE);
				PWR_EnterSTANDBYMode();
			}
		}
    }
}

void EXTI0_IRQHandler(void)
{
	EXTI_ClearITPendingBit(EXTI_Line0);
	/*if (STATE==MENU_STATE)
	{
		MenuSwitchDelay = 1;
		if (MenuItem <= 1)
		{
			STATE = RUN_STATE;
			PREV_STATE = MENU_STATE;
		}
		else MenuItem--;
	}
	else
	{*/
		if ((STATE&0x3000)!=0x3000)//PWR_FLAG==0)
		{
			PREV_STATE = STATE;
			STATE |= SLEEP_STATE;
			//PWR_FLAG = 1;
		}
		else
		{
			PREV_STATE = STATE;
			STATE &= ~SLEEP_STATE;
			STATE |= RUN_STATE;
			init_clock();
			//RTC_ITConfig(RTC_IT_ALR, ENABLE);
			//RTC_WaitForLastTask();
		}
	//}
	//EXTI->PR|=0x01;
	EXTI_ClearFlag(EXTI_Line0);
}

void EXTI1_IRQHandler(void)
{
	//GPIO_InitTypeDef GPIO_InitStructure;
	EXTI_ClearITPendingBit(EXTI_Line1);
	if (STATE==RUN_STATE)
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
	else if (STATE==MENU_STATE)
	{
		MenuSwitchDelay = 1;
		switch (MenuItem)
		{
			case MENU_ALTITUDE:
				STATE = RUN_STATE;
				PREV_STATE = MENU_STATE;
				break;
			case MENU_SETOXLIM:
				changePO2Lim(1);
				break;
			case MENU_SETOX:
				changeOxygen(1);
				break;
			case MENU_SETHE:
				changeHelium(1);
				break;
			case MENU_SETEMUL:
				if (Emulation==0) Emulation=1;
				else Emulation = 0;
				break;
			case MENU_SETTIME:
				/*STATE = SETDAY_STATE;
				PREV_STATE = MENU_STATE;
				GPIOA->BSRR = GPIO_BSRR_BS14 | GPIO_BSRR_BS15;
				GPIOC->BSRR = GPIO_BSRR_BS0 | GPIO_BSRR_BR1;*/
				BKP->DR1 = 0x0000;
				BKP->DR4 = 0x0000;
				set_current_logaddr(BASE_ADDRESS);//==========
				break;
			case MENU_SETCONN:
				STATE = CONNECTION_STATE;
				PREV_STATE = MENU_STATE;
				break;
			default:
				break;
		}
	}
	else if (STATE==SETDAY_STATE)
	{
		PREV_STATE = STATE;
		STATE = SETWEEKDAY_STATE;
	}
	else if (STATE==SETWEEKDAY_STATE)
	{
		PREV_STATE = STATE;
		STATE = SETMON_STATE;
	}
	else if (STATE==SETMON_STATE)
	{
		PREV_STATE = STATE;
		STATE = SETYEAR_STATE;
	}
	else if (STATE==SETYEAR_STATE)
	{
		PREV_STATE = STATE;
		STATE = SETHOUR_STATE;
	}
	else if (STATE==SETHOUR_STATE)
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

	EXTI_ClearFlag(EXTI_Line1);
	//EXTI->PR|=0x02;
}

void EXTI2_IRQHandler(void)
{
	EXTI_ClearITPendingBit(EXTI_Line2);
	if (STATE == SETDAY_STATE)
	{
		if (DT.Day==31) DT.Day=1;
		else DT.Day = DT.Day + 1;
	}
	else if (STATE == SETWEEKDAY_STATE)
	{
		if (DT.WeekDay==7) DT.WeekDay=1;
		else DT.WeekDay = DT.WeekDay + 1;
	}
	else if (STATE == SETMON_STATE)
	{
		if (DT.Month==12) DT.Month=1;
		else DT.Month = DT.Month + 1;
	}
	else if (STATE == SETYEAR_STATE)
	{
		DT.Year = DT.Year + 1;
	}
	else if (STATE==SETHOUR_STATE)
	{
		if (HOUR==23) HOUR = 0;
		else HOUR = HOUR + 1;
	}
	else if (STATE==SETMINUTE_STATE)
	{
		if (MINUTE==59) MINUTE = 0;
		else MINUTE = MINUTE + 1;
	}
	else if (STATE==SETSECOND_STATE)
	{
		if (SECOND==59) SECOND = 0;
		else SECOND = SECOND + 1;
	}
	else if (STATE==RUN_STATE)
	{
		STATE = MENU_STATE;
		PREV_STATE = RUN_STATE;
		MenuItem = 0;
		MenuSwitchDelay = 1;
		//TFT_DMA_fillRect(0,0,319,239,TFT_rgb2color(0,0,0));
	}
	else if (STATE==MENU_STATE)
	{
		MenuSwitchDelay = 1;
		if (MenuItem==6)
		{
			STATE = RUN_STATE;
			PREV_STATE = MENU_STATE;
		}
		else MenuItem++;
	}
	else if (STATE==CONNECTION_STATE)
	{
		MenuSwitchDelay = 1;
		PREV_STATE = CONNECTION_STATE;
		STATE = MENU_STATE;
	}
	//EXTI->PR|=0x04;
	EXTI_ClearFlag(EXTI_Line2);
}
