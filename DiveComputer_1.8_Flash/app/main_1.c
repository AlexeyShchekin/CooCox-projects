#include <math.h>
#include <stm32f10x.h>
#include <stm32f10x_gpio.h>
#include <stm32f10x_rcc.h>
#include <stm32f10x_bkp.h>
#include <stm32f10x_rtc.h>
#include <stm32f10x_spi.h>
#include "sys_init.h"
#include "st7735.h"
#include "MS5803_I2C.h"
#include "auxiliary.h"
#include "SD_log.h"

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
#define MENU_CHARGE			7
#define MENU_SETEMUL		4
#define MENU_SETTIME		5

#define SLEEP_STATE			0x3000
#define CONNECTION_STATE	0x4000

uint32_t BASE_ADDRESS = 0x00000000;

volatile int HOUR = 0;
volatile int MINUTE = 0;
volatile int SECOND = 0;
volatile DateTime DT = {12,1,9,2016,1};

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

//volatile uint8_t Emulation = 0;
volatile uint8_t Charge = 0;

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

uint8_t SDBuffer[512];
uint16_t SDBufferPos = 0;

uint8_t SD_ReadSector(uint32_t BlockNumb)
{
	uint16_t i=0;

	if(SD_sendCommand(READ_SINGLE_BLOCK, BlockNumb)) return 1;
	CS_SD_ENABLE;

	while(SD_SPI_read() != 0xfe)
	if(i++ > 0xfffe) {CS_SD_DISABLE; return 1;}

	for(i=0; i<512; i++) SDBuffer[i] = SD_SPI_read();

	SD_SPI_read();
	SD_SPI_read();
	SD_SPI_read();

	CS_SD_DISABLE;

	return 0;
}

uint8_t SD_WriteSector(uint32_t BlockNumb)
{
	uint8_t     response;
	uint16_t    i,wait=0;

	if( SD_sendCommand(WRITE_SINGLE_BLOCK, BlockNumb)) return 1;

	CS_SD_ENABLE;
	SD_SPI_send(0xfe);

	for(i=0; i<512; i++) SD_SPI_send(SDBuffer[i]);

	SD_SPI_send(0xff);
	SD_SPI_send(0xff);

	response = SD_SPI_read();

	if( (response & 0x1f) != 0x05)
	{ CS_SD_DISABLE; return 1; }

	while(!SD_SPI_read())
	if(wait++ > 0xfffe){CS_SD_DISABLE; return 1;}

	CS_SD_DISABLE;
	SD_SPI_send(0xff);
	CS_SD_ENABLE;

	while(!SD_SPI_read())
	if(wait++ > 0xfffe){CS_SD_DISABLE; return 1;}
	CS_SD_DISABLE;

	return 0;
}

void SDBuff_clear(void)
{
	int i;
	for(i=0;i<512;i++)
	{
		SDBuffer[i]=0;
	}
	SDBufferPos = 0;
}

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
	uint16_t color = RGB565(255,255,255);
	uint16_t marker = RGB565(255,255,0);
	uint16_t backcolor = RGB565(0,0,0);
	char AltStr[7];
	char POLimStr[5];
	char PO2Str[5];
	char PHeStr[5];
	ST7735_FillRect_DMA(0,0,159,127, backcolor);

	//if (Altitude < 0) AltToStr(AltStr,-Altitude);
	//else AltToStr(AltStr,Altitude);
	AltToStr(AltStr,Altitude);
	ST7735_PutStr5x7_DMA(15, 5, "Altitude: ", 10, color, backcolor, 1);
	ST7735_PutStr5x7_DMA(100, 5, AltStr, 7, color, backcolor, 1);

	TempToStr(POLimStr,PO2_lim);
	ST7735_PutStr5x7_DMA(15,20,"Set PO2 limit: ",15, color,backcolor,1);
	ST7735_PutStr5x7_DMA(100,20,POLimStr,5, color,backcolor,1);

	TempToStr(PO2Str,100*CO2);
	ST7735_PutStr5x7_DMA(15,35,"Set PO2: ",9,color,backcolor,1);
	ST7735_PutStr5x7_DMA(100,35,PO2Str,5,color,backcolor,1);

	TempToStr(PHeStr,100*CHe);
	ST7735_PutStr5x7_DMA(15,50,"Set He: ",8,color,backcolor,1);
	ST7735_PutStr5x7_DMA(100,50,PHeStr,5,color,backcolor,1);

	//if (Emulation) ST7735_PutStr5x7_DMA(15,65,"Emulation: YES",14,color,backcolor,1);
	//else ST7735_PutStr5x7_DMA(15,65,"Emulation: NO",13,color,backcolor,1);

	ST7735_PutStr5x7_DMA(15,80,"Reset time?",11,color,backcolor,1);

	ST7735_PutStr5x7_DMA(15,95,"Set connection?",15,color,backcolor,1);

	if (Charge==1) ST7735_PutStr5x7_DMA(15,110,"Charge: YES",11,color,backcolor,1);
	else ST7735_PutStr5x7_DMA(15,110,"Charge: NO",10,color,backcolor,1);

	switch (MenuItem)
	{
		case MENU_ALTITUDE:
			ST7735_FillRect_DMA(0,5,10,15, marker);
			break;
		case MENU_SETOXLIM:
			ST7735_FillRect_DMA(0,20,10,30,marker);
			break;
		case MENU_SETOX:
			ST7735_FillRect_DMA(0,35,10,45,marker);
			break;
		case MENU_SETHE:
			ST7735_FillRect_DMA(0,50,10,60,marker);
			break;
		case MENU_SETEMUL:
			ST7735_FillRect_DMA(0,65,10,75,marker);
			break;
		case MENU_SETTIME:
			ST7735_FillRect_DMA(0,80,10,90,marker);
			break;
		case MENU_SETCONN:
			if (STATE==MENU_STATE) ST7735_FillRect_DMA(0,95,10,105,marker);
			if (STATE==CONNECTION_STATE) ST7735_FillRect_DMA(0,95,10,105,RGB565(0,255,0));
			break;
		case MENU_CHARGE:
			ST7735_FillRect_DMA(0,110,10,120,marker);
			break;
		default:
			break;
	}
}

void GetSensorMeasurements(void)
{
//	Sensor_ON();
	double Data[2];
	MS5803_Init();
	MS5803_reset();
	MS5803_begin();
	MS5803_getFullData(CELSIUS,ADC_4096,Data);
//	Sensor_OFF();
	MS5803_Deinit();
	CurrentData[0] = Data[0];
	CurrentData[1] = (Data[1]-SensCalibr)/101.325;
}

void RedrawValues(void)
{
	uint16_t color = RGB565(255,255,255);
	uint16_t backcolor = RGB565(0,0,0);
	char TStr[5] = "     ";
	char HStr[5] = "     ";
	char TmStr[8] = "        ";
	char CNSStr[6] = "     %";
	char DayStr[2];

	TempToStr(TStr, CurrentData[0]);
	TempToStr(HStr, CurrentData[1]);

	ST7735_PutStr5x7_DMA(17, 37, TStr, 5, color, backcolor, 1);
	ST7735_PutStr10x16_DMA(10, 72, HStr, 5, color, backcolor, 1);

	DT = GetDate();
	//itoa((int)(DT.Day),DayStr,10);
	IntToStr(DayStr,(int)(DT.Day));
	ST7735_PutStr5x7_DMA(55, 57, DayStr, 2, color, backcolor, 1);
	switch(DT.Month)
	{
		case 1:
			ST7735_PutStr5x7_DMA(67, 57, "JAN", 3, color, backcolor, 1);
			break;
		case 2:
			ST7735_PutStr5x7_DMA(67, 57, "FEB", 3, color, backcolor, 1);
			break;
		case 3:
			ST7735_PutStr5x7_DMA(67, 57, "MAR", 3, color, backcolor, 1);
			break;
		case 4:
			ST7735_PutStr5x7_DMA(67, 57, "APR", 3, color, backcolor, 1);
			break;
		case 5:
			ST7735_PutStr5x7_DMA(67, 57, "MAY", 3, color, backcolor, 1);
			break;
		case 6:
			ST7735_PutStr5x7_DMA(67, 57, "JUN", 3, color, backcolor, 1);
			break;
		case 7:
			ST7735_PutStr5x7_DMA(67, 57, "JUL", 3, color, backcolor, 1);
			break;
		case 8:
			ST7735_PutStr5x7_DMA(67, 57, "AUG", 3, color, backcolor, 1);
			break;
		case 9:
			ST7735_PutStr5x7_DMA(67, 57, "SEP", 3, color, backcolor, 1);
			break;
		case 10:
			ST7735_PutStr5x7_DMA(67, 57, "OCT", 3, color, backcolor, 1);
			break;
		case 11:
			ST7735_PutStr5x7_DMA(67, 57, "NOV", 3, color, backcolor, 1);
			break;
		case 12:
			ST7735_PutStr5x7_DMA(67, 57, "DEC", 3, color, backcolor, 1);
			break;
		default:
			break;
	}
	switch(DT.WeekDay)
	{
		case 1:
			ST7735_PutStr5x7_DMA(60, 70, "MON", 3, color, backcolor, 1);
			break;
		case 2:
			ST7735_PutStr5x7_DMA(60, 70, "TUE", 3, color, backcolor, 1);
			break;
		case 3:
			ST7735_PutStr5x7_DMA(60, 70, "WED", 3, color, backcolor, 1);
			break;
		case 4:
			ST7735_PutStr5x7_DMA(60, 70, "THU", 3, color, backcolor, 1);
			break;
		case 5:
			ST7735_PutStr5x7_DMA(60, 70, "FRI", 3, color, backcolor, 1);
			break;
		case 6:
			ST7735_PutStr5x7_DMA(60, 70, "SAT", 3, color, backcolor, 1);
			break;
		case 7:
			ST7735_PutStr5x7_DMA(60, 70, "SUN", 3, color, backcolor, 1);
			break;
		default:
			break;
	}
	itoa(DT.Year,HStr,10);
	ST7735_PutStr5x7_DMA(57, 82, HStr, 4, color, backcolor, 1);

	uint32_t T,H,M,S;
	if (DiveMODE==DECO_STATE)
	{
		TempToStr(HStr, DecoStop);
		ST7735_PutStr5x7_DMA(105, 72, HStr, 5, color, backcolor, 1);
	}
	else
	{
		if (DiveMODE==NODIVE_STATE || DiveMODE==SURF_STATE) T = (uint32_t)(RTC_GetCounter()>>8);
		else T = (uint32_t)(NDL*60.0);
		H = (uint32_t)(T/3600);
		M = (uint32_t)(T/60) - 60*H;
		S = T - 3600*H - 60*M;
		TimeToStr(TmStr,H,M,S);
		ST7735_PutStr7x11_DMA(90, 72, TmStr, 8, color, backcolor, 1);
	}
	if (DiveMODE==DIVE_STATE||DiveMODE==DECO_STATE)
	{
		H = (uint32_t)(CurrentTime/3600);
		M = (uint32_t)(CurrentTime/60) - 60*H;
		S = CurrentTime - 3600*H - 60*M;
		TimeToStr(TmStr,H,M,S);
		ST7735_PutStr5x7_DMA(110, 37, TmStr, 8, color, backcolor, 1);
		TempToStr(HStr, Velocity);
		ST7735_PutStr5x7_DMA(70, 37, HStr, 5, color, backcolor, 1);
	}
	if (DiveMODE==SURF_STATE)
	{
		H = (uint32_t)(CurrentSurfTime/3600);
		M = (uint32_t)(CurrentSurfTime/60) - 60*H;
		S = CurrentSurfTime - 3600*H - 60*M;
		TimeToStr(TmStr,H,M,S);
		ST7735_PutStr5x7_DMA(110, 37, TmStr, 8, color, backcolor, 1);
	}
	if (DiveMODE!=NODIVE_STATE)
	{
		DrawNitrogenSaturation();
		DrawHeliumSaturation();
		TempToStr(CNSStr, 100*CNS);
		ST7735_PutStr5x7_DMA(115, 112, CNSStr,6, color, backcolor, 1);
	}
}

void drawSetData(void)
{
	ST7735_FillRect_DMA(0,0,159,127, RGB565(0,0,0));
	char DayStr[2];
	char DY[4];
	ST7735_PutStr5x7_DMA(20, 10, "Set data:", 9, RGB565(255,255,255), RGB565(0,0,0), 1);
	ST7735_PutStr5x7_DMA(20, 30, "Day:", 4, RGB565(255,255,255), RGB565(0,0,0), 1);
	ST7735_PutStr5x7_DMA(20, 50, "Week day:", 9, RGB565(255,255,255), RGB565(0,0,0), 1);
	ST7735_PutStr5x7_DMA(20, 70, "Month:", 6, RGB565(255,255,255), RGB565(0,0,0), 1);
	ST7735_PutStr5x7_DMA(20, 90, "Year:", 5, RGB565(255,255,255), RGB565(0,0,0), 1);
	//itoa((int)(DT.Day),DD,10);

	IntToStr(DayStr,(int)(DT.Day));
	itoa((int)(DT.Year),DY,10);

	if (STATE==SETDAY_STATE) ST7735_PutStr5x7_DMA(80, 30, DayStr, 2, RGB565(0,0,0), RGB565(255,255,0), 1);
	else ST7735_PutStr5x7_DMA(80, 30, DayStr, 2, RGB565(255,255,0), RGB565(0,0,0), 1);
	if (STATE==SETYEAR_STATE) ST7735_PutStr5x7_DMA(80, 90, DY, 4, RGB565(0,0,0), RGB565(255,255,0), 1);
	else ST7735_PutStr5x7_DMA(80, 90, DY, 4, RGB565(255,255,0), RGB565(0,0,0), 1);

	switch (DT.WeekDay)
	{
		case 1:
			if (STATE==SETWEEKDAY_STATE) ST7735_PutStr5x7_DMA(80, 50, "MON", 3, RGB565(0,0,0), RGB565(255,255,0), 1);
			else ST7735_PutStr5x7_DMA(80, 50, "MON", 3, RGB565(255,255,0), RGB565(0,0,0), 1);
			break;
		case 2:
			if (STATE==SETWEEKDAY_STATE) ST7735_PutStr5x7_DMA(80, 50, "TUE", 3, RGB565(0,0,0), RGB565(255,255,0), 1);
			else ST7735_PutStr5x7_DMA(80, 50, "TUE", 3, RGB565(255,255,0), RGB565(0,0,0), 1);
			break;
		case 3:
			if (STATE==SETWEEKDAY_STATE) ST7735_PutStr5x7_DMA(80, 50, "WED", 3, RGB565(0,0,0), RGB565(255,255,0), 1);
			else ST7735_PutStr5x7_DMA(80, 50, "WED", 3, RGB565(255,255,0), RGB565(0,0,0), 1);
			break;
		case 4:
			if (STATE==SETWEEKDAY_STATE) ST7735_PutStr5x7_DMA(80, 50, "THU", 3, RGB565(0,0,0), RGB565(255,255,0), 1);
			else ST7735_PutStr5x7_DMA(80, 50, "THU", 3, RGB565(255,255,0), RGB565(0,0,0), 1);
			break;
		case 5:
			if (STATE==SETWEEKDAY_STATE) ST7735_PutStr5x7_DMA(80, 50, "FRI", 3, RGB565(0,0,0), RGB565(255,255,0), 1);
			else ST7735_PutStr5x7_DMA(80, 50, "FRI", 3, RGB565(255,255,0), RGB565(0,0,0), 1);
			break;
		case 6:
			if (STATE==SETWEEKDAY_STATE) ST7735_PutStr5x7_DMA(80, 50, "SAT", 3, RGB565(0,0,0), RGB565(255,255,0), 1);
			else ST7735_PutStr5x7_DMA(80, 50, "SAT", 3, RGB565(255,255,0), RGB565(0,0,0), 1);
			break;
		case 7:
			if (STATE==SETWEEKDAY_STATE) ST7735_PutStr5x7_DMA(80, 50, "SUN", 3, RGB565(0,0,0), RGB565(255,255,0), 1);
			else ST7735_PutStr5x7_DMA(80, 50, "SUN", 3, RGB565(255,255,0), RGB565(0,0,0), 1);
			break;
		default:
			break;
	}
	switch (DT.Month)
	{
		case 1:
			if (STATE==SETMON_STATE) ST7735_PutStr5x7_DMA(80, 70, "JAN", 3, RGB565(0,0,0), RGB565(255,255,0), 1);
			else ST7735_PutStr5x7_DMA(80, 70, "JAN", 3, RGB565(255,255,0), RGB565(0,0,0), 1);
			break;
		case 2:
			if (STATE==SETMON_STATE) ST7735_PutStr5x7_DMA(80, 70, "FEB", 3, RGB565(0,0,0), RGB565(255,255,0), 1);
			else ST7735_PutStr5x7_DMA(80, 70, "FEB", 3, RGB565(255,255,0), RGB565(0,0,0), 1);
			break;
		case 3:
			if (STATE==SETMON_STATE) ST7735_PutStr5x7_DMA(80, 70, "MAR", 3, RGB565(0,0,0), RGB565(255,255,0), 1);
			else ST7735_PutStr5x7_DMA(80, 70, "MAR", 3, RGB565(255,255,0), RGB565(0,0,0), 1);
			break;
		case 4:
			if (STATE==SETMON_STATE) ST7735_PutStr5x7_DMA(80, 70, "APR", 3, RGB565(0,0,0), RGB565(255,255,0), 1);
			else ST7735_PutStr5x7_DMA(80, 70, "APR", 3, RGB565(255,255,0), RGB565(0,0,0), 1);
			break;
		case 5:
			if (STATE==SETMON_STATE) ST7735_PutStr5x7_DMA(80, 70, "MAY", 3, RGB565(0,0,0), RGB565(255,255,0), 1);
			else ST7735_PutStr5x7_DMA(80, 70, "MAY", 3, RGB565(255,255,0), RGB565(0,0,0), 1);
			break;
		case 6:
			if (STATE==SETMON_STATE) ST7735_PutStr5x7_DMA(80, 70, "JUN", 3, RGB565(0,0,0), RGB565(255,255,0), 1);
			else ST7735_PutStr5x7_DMA(80, 70, "JUN", 3, RGB565(255,255,0), RGB565(0,0,0), 1);
			break;
		case 7:
			if (STATE==SETMON_STATE) ST7735_PutStr5x7_DMA(80, 70, "JUL", 3, RGB565(0,0,0), RGB565(255,255,0), 1);
			else ST7735_PutStr5x7_DMA(80, 70, "JUL", 3, RGB565(255,255,0), RGB565(0,0,0), 1);
			break;
		case 8:
			if (STATE==SETMON_STATE) ST7735_PutStr5x7_DMA(80, 70, "AUG", 3, RGB565(0,0,0), RGB565(255,255,0), 1);
			else ST7735_PutStr5x7_DMA(80, 70, "AUG", 3, RGB565(255,255,0), RGB565(0,0,0), 1);
			break;
		case 9:
			if (STATE==SETMON_STATE) ST7735_PutStr5x7_DMA(80, 70, "SEP", 3, RGB565(0,0,0), RGB565(255,255,0), 1);
			else ST7735_PutStr5x7_DMA(80, 70, "SEP", 3, RGB565(255,255,0), RGB565(0,0,0), 1);
			break;
		case 10:
			if (STATE==SETMON_STATE) ST7735_PutStr5x7_DMA(80, 70, "OCT", 3, RGB565(0,0,0), RGB565(255,255,0), 1);
			else ST7735_PutStr5x7_DMA(80, 70, "OCT", 3, RGB565(255,255,0), RGB565(0,0,0), 1);
			break;
		case 11:
			if (STATE==SETMON_STATE) ST7735_PutStr5x7_DMA(80, 70, "NOV", 3, RGB565(0,0,0), RGB565(255,255,0), 1);
			else ST7735_PutStr5x7_DMA(80, 70, "NOV", 3, RGB565(255,255,0), RGB565(0,0,0), 1);
			break;
		case 12:
			if (STATE==SETMON_STATE) ST7735_PutStr5x7_DMA(80, 70, "DEC", 3, RGB565(0,0,0), RGB565(255,255,0), 1);
			else ST7735_PutStr5x7_DMA(80, 70, "DEC", 3, RGB565(255,255,0), RGB565(0,0,0), 1);
			break;
		default:
			break;
	}
}

void drawSetTime(void)
{
	ST7735_FillRect_DMA(0,0,159,127, RGB565(0,0,0));
	char Th[2];
	char Tm[2];
	char Ts[2];
	ST7735_PutStr5x7_DMA(20, 20, "Set time:", 9, RGB565(255,255,255), RGB565(0,0,0), 1);
	ST7735_PutStr5x7_DMA(20, 50, "Hours:", 6, RGB565(255,255,255), RGB565(0,0,0), 1);
	ST7735_PutStr5x7_DMA(20, 80, "Minutes:", 8, RGB565(255,255,255), RGB565(0,0,0), 1);
	ST7735_PutStr5x7_DMA(20, 110, "Seconds:", 8, RGB565(255,255,255), RGB565(0,0,0), 1);
	//itoa((int)HOUR,Th,10);
	IntToStr(Th,HOUR);
	//itoa((int)MINUTE,Tm,10);
	IntToStr(Tm,MINUTE);
	//itoa((int)SECOND,Ts,10);
	IntToStr(Ts,SECOND);
	if (STATE==SETHOUR_STATE) ST7735_PutStr5x7_DMA(100, 50, Th, 2, RGB565(0,0,0), RGB565(255,255,0), 1);
	else ST7735_PutStr5x7_DMA(100, 50, Th, 2, RGB565(255,255,0), RGB565(0,0,0), 1);
	if (STATE==SETMINUTE_STATE) ST7735_PutStr5x7_DMA(100, 80, Tm, 2, RGB565(0,0,0), RGB565(255,255,0), 1);
	else ST7735_PutStr5x7_DMA(100, 80, Tm, 2, RGB565(255,255,0), RGB565(0,0,0), 1);
	if (STATE==SETSECOND_STATE) ST7735_PutStr5x7_DMA(100, 110, Ts, 2, RGB565(0,0,0), RGB565(255,255,0), 1);
	else ST7735_PutStr5x7_DMA(100, 110, Ts, 2, RGB565(255,255,0), RGB565(0,0,0), 1);
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

    /*if (TFT_FLAG==0x00)
    {
    	if (CounterLED==0) GPIOB->BSRR |= GPIO_BSRR_BS13;
    	else GPIOB->BSRR |= GPIO_BSRR_BR13;
    	if (CounterLED>=3) CounterLED = 0;
    	else CounterLED++;
    }*/

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
	uint16_t color = RGB565(255,0,0);
	uint16_t backcolor = RGB565(0,0,0);
	ST7735_FillRect_DMA(0,0,159,127, backcolor);

	ST7735_Rect(0,20,159,127,color);
	ST7735_HLine(0, 159, 51, color);
	ST7735_HLine(0, 159, 96, color);

	color = RGB565(0,255,0);
	for (i = 0; i<16; i++)
	{
		ST7735_Rect(10*i,0,10*(i+1)-1,19,color);
	}
	color = RGB565(255,255,0);

	if (DiveMODE==DIVE_STATE)
	{
		ST7735_PutStr5x7_DMA(95, 56, "Non-Deco:", 9, color,backcolor, 1);
		ST7735_PutStr5x7_DMA(113, 25, "Dive:", 5, color,backcolor, 1);
		ST7735_PutStr5x7_DMA(65, 25, "Speed:", 6, color,backcolor, 1);
		ST7735_PutStr5x7_DMA(10, 56, "Depth, m:", 9, color,backcolor, 1);
	}
	else if (DiveMODE==DECO_STATE)
	{
		ST7735_PutStr5x7_DMA(95, 56, "Deco Stop:", 10, color,backcolor, 1);
		ST7735_PutStr5x7_DMA(113, 25, "Dive:", 5, color,backcolor, 1);
		ST7735_PutStr5x7_DMA(65, 25, "Speed:", 6, color,backcolor, 1);
		ST7735_PutStr5x7_DMA(10, 56, "Depth, m:", 9, color,backcolor, 1);
	}
	else
	{
		ST7735_PutStr5x7_DMA(110, 56, "Time:", 5, color,backcolor, 1);
		ST7735_PutStr5x7_DMA(113, 25, "Surf:", 5, color,backcolor, 1);
		ST7735_PutStr5x7_DMA(65, 25, "Depth, m:", 6, color,backcolor, 1);
	}

	ST7735_PutStr5x7_DMA(110, 99, "CNS O :", 7, color,backcolor, 1);
	ST7735_PutStr5x7_DMA(140, 101, "2", 1, color,backcolor, 1);
	ST7735_PutStr5x7_DMA(75, 99, "O", 1, color,backcolor, 1);
	ST7735_PutStr5x7_DMA(80, 101, "2", 1, color,backcolor, 1);
	ST7735_PutStr5x7_DMA(5, 99, "Max Depth:", 10, color,backcolor, 1);
	ST7735_PutStr5x7_DMA(15, 25, "T, C", 4, color,backcolor, 1);
	ST7735_PutStr5x7_DMA(25, 21, "o", 1, color,backcolor, 1);

	color = RGB565(255,255,255);
	char O2Printout[6] = "     %";
	char MaxDepthPrintout[7] = "      m";
	TempToStr(O2Printout,CO2*100);
	TempToStr(MaxDepthPrintout,10*PO2_lim/CO2-10.0);
	ST7735_PutStr5x7_DMA(70, 112, O2Printout, 6, color,backcolor, 1);
	ST7735_PutStr5x7_DMA(15, 112, MaxDepthPrintout, 7, color,backcolor, 1);
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
	//Sensor_ON();
	double Data[2];
	MS5803_Init();
	MS5803_reset();
	MS5803_begin();
	MS5803_getFullData(CELSIUS,ADC_4096,Data);
	//Sensor_OFF();
	MS5803_Deinit();
	SensCalibr = Data[1];
	P_atm = SensCalibr/101.325;
	Altitude = 29.271263*log(10.0/P_atm)*(273.15+(Data[0]));
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
		ST7735_FillRect_DMA(1+10*i,1,4+10*i,18, RGB565(0,0,0));
		ST7735_FillRect_DMA(1+10*i,18-(int)(17*sat),4+10*i,18, RGB565(255,0,0));
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
		ST7735_FillRect_DMA(5+10*i,1,8+10*i,18, RGB565(0,0,0));
		ST7735_FillRect_DMA(5+10*i,18-(int)(17*sat),8+10*i,18, RGB565(255,255,0));
	}
}

int main(void)
{
	init_clock();
	//SDBuff_clear();
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

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3;		// INPUT BUTTONS AND BATTERY ADC
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	RCC_ClearFlag();

	NVIC_Configuration();
	  /* Configure EXTI Line to generate an interrupt on falling edge */
	EXTI_Configuration();

	AFIO->EXTICR[0] = 0x00000000;

	//NVIC_EnableIRQ (EXTI3_IRQn);
	NVIC_EnableIRQ (EXTI2_IRQn);
	NVIC_EnableIRQ (EXTI1_IRQn);
	NVIC_EnableIRQ (EXTI0_IRQn);

	RTC_Configuration();

	MS5803_Init();
	ST7735_Init();
	ST7735_AddrSet(0,0,159,127);
	ST7735_Orientation(scr_CW);
	ST7735_Clear(0x0000);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOB,&GPIO_InitStructure);

	GPIOB->BSRR |= GPIO_BSRR_BR12;

	//ChargeOFF();
	USART1_ON();
	if (SD_init()==0)
	{
		//SD_RESULT = 0x01;
		send_to_uart(0x01);
	}
	else send_to_uart(0x00);//SD_RESULT = 0x00;
	USART1_OFF();
	ST7735_SPI_SpeedSwitch();

	int i=0;
	for (i=0; i<16; i++)
	{
		k[i] = log(2)/k[i];
		k_He[i] = log(2)/k_He[i];
	}

	DiveMODE = NODIVE_STATE;
	setAltitude();
	ResetPN();

	DrawDiveDesktop();

	uint32_t T;

    while(1)
    {
    	if ((STATE==SETDAY_STATE)||(STATE==SETWEEKDAY_STATE)||(STATE==SETMON_STATE)||(STATE==SETYEAR_STATE))
		{
			if (PREV_STATE==MENU_STATE)
			{
				Delay_ms(500);
				init_clock();
				GPIOB->BSRR |= GPIO_BSRR_BS13 | GPIO_BSRR_BS14;
				//GPIOC->BSRR = GPIO_BSRR_BS0 | GPIO_BSRR_BR1;
				//ili9341_init();
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
				GPIOB->BSRR |= GPIO_BSRR_BS13 | GPIO_BSRR_BS14;
			//	GPIOC->BSRR = GPIO_BSRR_BS0 | GPIO_BSRR_BR1;
			//	ili9341_init();
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
		//	set_current_logaddr(BASE_ADDRESS);
			PREV_STATE = TIMEUPDATE_STATE;
			STATE = RUN_STATE;
			//TFT_DMA_fillRect(0,0,319,239,TFT_rgb2color(0,0,0));
			BKP->DR1 = 0x01;
			DrawDiveDesktop();
			//RTC_ITConfig(RTC_IT_ALR, ENABLE);
			//RTC_WaitForLastTask();
		}
		else if (STATE==ALTCALIBR_STATE)
		{
			//Delay_ms(500);
			GPIOB->BSRR |= GPIO_BSRR_BS13 | GPIO_BSRR_BS14;
			setAltitude();
			ResetPN();
			//BKP->DR4 = 0x0000;//==========
			//BKP->DR1 = 0x0000;
			//set_current_logaddr(BASE_ADDRESS);//==========
			STATE = RUN_STATE;
			PREV_STATE = ALTCALIBR_STATE;
		}
		else //if (STATE==RUN_STATE)
		{
			if (TFT_FLAG==0x01)
			{
				init_clock();
				GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
				GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
				GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
				GPIO_Init(GPIOB, &GPIO_InitStructure);
				GPIOB->BSRR |= GPIO_BSRR_BS14 | GPIO_BSRR_BS13;
				//GPIOC->BSRR |= GPIO_BSRR_BS0 | GPIO_BSRR_BR1;
				ST7735_cmd(0x11);
				Delay_ms(500);
				ST7735_Init();
				ST7735_AddrSet(0,0,159,127);
				ST7735_Orientation(scr_CW);
				ST7735_Clear(0x0000);
				TFT_FLAG=0x00;
				DrawDiveDesktop();
			}
			if (TFT_FLAG==0x11)
			{
				ST7735_cmd(0x10);
				Delay_ms(500);
				GPIOB->BSRR |= GPIO_BSRR_BR13;
				//GPIOC->BSRR &= ~(GPIO_BSRR_BS0 | GPIO_BSRR_BS1);
				/*GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1;
				GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
				GPIO_Init(GPIOC, &GPIO_InitStructure);*/
				GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
				GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
				GPIO_Init(GPIOB, &GPIO_InitStructure);
				TFT_FLAG = 0x10;
			}
			else if ((STATE&0x3000)!=0x3000)//PWR_FLAG==0)
			{
				RTC_ITConfig(RTC_IT_ALR, DISABLE);
				if (Counter>=127)
				{
					init_clock();
					//GPIOB->BSRR |= GPIO_BSRR_BS14;
					if (TFT_FLAG==0x00) {GPIOB->BSRR |= GPIO_BSRR_BS13 | GPIO_BSRR_BS14;}
					else GPIOB->BSRR |= GPIO_BSRR_BR13;
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
					//if (Emulation==1) CurrentData[1] = 55.0;

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
							if (TFT_FLAG==0x00)
							{
								GPIOB->BSRR |= GPIO_BSRR_BS13;
								DrawDiveDesktop();
							}
							CurrentSurfTime = 0;
							StartSurfTime = T>>8;
							//USART1_ON();
							RTC_ITConfig(RTC_IT_ALR, DISABLE);
							//**************************************
							uint32_t LogSector = get_current_logaddr();
							Delay_us(40);//send_to_uart(0x0A);
							CS_H();
							if (SD_init()==0)
							{
								Delay_us(40);//send_to_uart(0x0B);
								SD_WriteSector(LogSector);
								Delay_us(40);//send_to_uart(0x0C);
								ST7735_SPI_SpeedSwitch();
								Delay_us(40);//send_to_uart(0x0D);
								SDBuff_clear();//******************
								Delay_us(40);//send_to_uart(0x0E);
								set_current_logaddr(LogSector + 1);
								Delay_us(40);//send_to_uart(0x0F);
							}
							CS_SD_DISABLE;
							//SDBufferPos = 0;
							//***************************************
							uint16_t Dives = BKP->DR4;
							BKP->DR4 = (uint16_t)(Dives + 1);
							Delay_us(40);//send_to_uart(0x1A);
							RTC_ITConfig(RTC_IT_ALR, ENABLE);
							//USART1_OFF();
						}
						else if (DiveMODE!=SURF_STATE)
						{
							if (CurrentTime%2==0)
							{
								if (SDBufferPos>=512)
								{
									//GPIOB->BSRR |= GPIO_BSRR_BS13;// | GPIO_BSRR_BS14;
									//USART1_ON();
									RTC_ITConfig(RTC_IT_ALR, DISABLE);
									uint32_t LogSector = get_current_logaddr();
									Delay_us(40);//send_to_uart(0x0A);
									CS_H();
									if (SD_init()==0)
									{
										Delay_us(40);//send_to_uart(0x0B);
										SD_WriteSector(LogSector);
										Delay_us(40);//send_to_uart(0x0C);
										ST7735_SPI_SpeedSwitch();
										Delay_us(40);//send_to_uart(0x0D);
										SDBuff_clear();//******************
										Delay_us(40);//send_to_uart(0x0E);
										set_current_logaddr(LogSector + 1);
										Delay_us(40);//send_to_uart(0x0F);
									}
									CS_SD_DISABLE;
									//SDBufferPos = 0;
									Delay_us(40);//send_to_uart(0x1A);
									RTC_ITConfig(RTC_IT_ALR, ENABLE);
									//USART1_OFF();
								}
								else
								{
									uint32_t Height = (uint32_t)(CurrentData[1]*10);
									SDBuffer[SDBufferPos++] = (uint8_t)((CurrentTime>>24)&0xFF);
									SDBuffer[SDBufferPos++] = (uint8_t)((CurrentTime>>16)&0xFF);
									SDBuffer[SDBufferPos++] = (uint8_t)((CurrentTime>>8)&0xFF);
									SDBuffer[SDBufferPos++] = (uint8_t)((CurrentTime)&0xFF);
									SDBuffer[SDBufferPos++] = (uint8_t)((Height>>24)&0xFF);
									SDBuffer[SDBufferPos++] = (uint8_t)((Height>>16)&0xFF);
									SDBuffer[SDBufferPos++] = (uint8_t)((Height>>8)&0xFF);
									SDBuffer[SDBufferPos++] = (uint8_t)((Height)&0xFF);
								}
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
						//FLASH_Unlock();
						DT = GetDate();//-----------------------------------
						uint32_t SendDate = ((uint32_t)DT.Day)*10000 + ((uint32_t)DT.Month)*100 + (((uint32_t)DT.Year)-2000);
						SDBuffer[SDBufferPos++] = 0xFA;
						SDBuffer[SDBufferPos++] = 0xFB;
						SDBuffer[SDBufferPos++] = 0xFC;
						SDBuffer[SDBufferPos++] = 0xFD;
						SDBuffer[SDBufferPos++] = (uint8_t)((SendDate>>24)&0xFF);
						SDBuffer[SDBufferPos++] = (uint8_t)((SendDate>>16)&0xFF);
						SDBuffer[SDBufferPos++] = (uint8_t)((SendDate>>8)&0xFF);
						SDBuffer[SDBufferPos++] = (uint8_t)((SendDate)&0xFF);
						uint32_t Height = (uint32_t)(CurrentData[1]*10);
						SDBuffer[SDBufferPos++] = (uint8_t)((CurrentTime>>24)&0xFF);
						SDBuffer[SDBufferPos++] = (uint8_t)((CurrentTime>>16)&0xFF);
						SDBuffer[SDBufferPos++] = (uint8_t)((CurrentTime>>8)&0xFF);
						SDBuffer[SDBufferPos++] = (uint8_t)((CurrentTime)&0xFF);
						SDBuffer[SDBufferPos++] = (uint8_t)((Height>>24)&0xFF);
						SDBuffer[SDBufferPos++] = (uint8_t)((Height>>16)&0xFF);
						SDBuffer[SDBufferPos++] = (uint8_t)((Height>>8)&0xFF);
						SDBuffer[SDBufferPos++] = (uint8_t)((Height)&0xFF);
						//log_current_point(get_current_logaddr(),0,SendDate);//-----------------------
						//log_current_point(get_current_logaddr(),CurrentTime,CurrentData[1]);
						//FLASH_Lock();
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
							//send16_to_uart(readADC1());
							uint16_t Dives = BKP->DR4;
							send16_to_uart(Dives);
							uint32_t ADDR = get_current_logaddr();
							send32_to_uart(ADDR);
							if (ADDR > BASE_ADDRESS)
							{
								int j,m;
								CS_H();
								//SD_SPI_SpeedSwitch();
								if (SD_init()==0)
								{
									for (j=BASE_ADDRESS; j<ADDR; j++)
									{
										SD_ReadSector(j);
										for (m=0; m<512; m++)
										{
											send_to_uart(SDBuffer[m]);
										}
									}
									ST7735_SPI_SpeedSwitch();
									//SDBuff_clear();
								}
								CS_SD_DISABLE;
							}
							Delay_ms(100);
							USART1_OFF();
							STATE = MENU_STATE;
							PREV_STATE = CONNECTION_STATE;
						}
					}
				}

				RTC_ITConfig(RTC_IT_ALR, ENABLE);
				/*if (TFT_FLAG==0x00)*/
				RTC_SetAlarm(RTC_GetCounter()+1);
				//else RTC_SetAlarm(RTC_GetCounter()+127);
				RTC_WaitForLastTask();
				PWR_EnterSTOPMode(PWR_Regulator_LowPower, PWR_STOPEntry_WFI);
			}
			else if ((STATE&0x3000)==0x3000 && (DiveMODE==NODIVE_STATE || DiveMODE==SURF_STATE))//(PWR_FLAG == 1)
			{
				//RTC_ITConfig(RTC_IT_ALR, DISABLE);
				//RTC_WaitForLastTask();
				//SPI_OFF();
				init_clock();
				Delay_ms(400);
				GPIOB->BSRR |= GPIO_BSRR_BR13 | GPIO_BSRR_BR14;
				//GPIOC->BSRR = GPIO_BSRR_BR0 | GPIO_BSRR_BR1;
				//GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1;
				//GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
				//GPIO_Init(GPIOC, &GPIO_InitStructure);
				GPIO_InitStructure.GPIO_Pin = GPIO_Pin_14 | GPIO_Pin_13;
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
		if ((STATE&0x3000)!=0x3000 && (DiveMODE==NODIVE_STATE || DiveMODE==SURF_STATE))//PWR_FLAG==0)
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
				//if (Emulation==0) Emulation=1;
				//else Emulation = 0;
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
			case MENU_CHARGE:
				if (Charge==0)
				{
					Charge = 1;
					GPIOB->BSRR |= GPIO_BSRR_BS12;
				}
				else
				{
					Charge = 0;
					GPIOB->BSRR |= GPIO_BSRR_BR12;
				}
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
		if (MenuItem==7)
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
/*
void EXTI3_IRQHandler(void)
{
	EXTI_ClearITPendingBit(EXTI_Line3);
	if (STATE == SETDAY_STATE)
	{
		if (DT.Day==1) DT.Day=31;
		else DT.Day = DT.Day - 1;
	}
	else if (STATE == SETWEEKDAY_STATE)
	{
		if (DT.WeekDay==1) DT.WeekDay=7;
		else DT.WeekDay = DT.WeekDay - 1;
	}
	else if (STATE == SETMON_STATE)
	{
		if (DT.Month==1) DT.Month=12;
		else DT.Month = DT.Month - 1;
	}
	else if (STATE == SETYEAR_STATE)
	{
		DT.Year = DT.Year - 1;
	}
	else if (STATE==SETHOUR_STATE)
	{
		if (HOUR==0) HOUR = 23;
		else HOUR = HOUR - 1;
	}
	else if (STATE==SETMINUTE_STATE)
	{
		if (MINUTE==0) MINUTE = 59;
		else MINUTE = MINUTE - 1;
	}
	else if (STATE==SETSECOND_STATE)
	{
		if (SECOND==0) SECOND = 59;
		else SECOND = SECOND - 1;
	}
	else if (STATE==RUN_STATE)
	{
		STATE = ALTCALIBR_STATE;
		PREV_STATE = RUN_STATE;
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
				changePO2Lim(0);
				break;
			case MENU_SETOX:
				changeOxygen(0);
				break;
			case MENU_SETHE:
				changeHelium(0);
				break;
			case MENU_SETEMUL:
				if (Emulation==0) Emulation=1;
				else Emulation = 0;
				break;
			case MENU_SETTIME:
				//STATE = SETDAY_STATE;
				//PREV_STATE = MENU_STATE;
				//GPIOA->BSRR = GPIO_BSRR_BS14 | GPIO_BSRR_BS15;
				//GPIOC->BSRR = GPIO_BSRR_BS0 | GPIO_BSRR_BR1;
				BKP->DR1 = 0x0000;
				BKP->DR4 = 0x0000;
				//set_current_logaddr(BASE_ADDRESS);//==========
				break;
			case MENU_SETCONN:
				STATE = CONNECTION_STATE;
				PREV_STATE = MENU_STATE;
				break;
			default:
				break;
		}
	}
	//EXTI->PR|=0x08;
	EXTI_ClearFlag(EXTI_Line3);
}*/
