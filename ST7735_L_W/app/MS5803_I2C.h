#ifndef MS5803_I2C_h
#define MS5803_I2C_h

#include <stm32f10x.h>
#include "sys_init.h"

#define I2C_GPIO                    GPIOB
#define I2C_RCC_APB2Periph_GPIO     RCC_APB2Periph_GPIOB
#define GPIO_Pin_SDA                GPIO_Pin_7
#define GPIO_Pin_SCL                GPIO_Pin_6

#define SCLH                        (I2C_GPIO->BSRR |= GPIO_BSRR_BS6)
#define SCLL                        (I2C_GPIO->BSRR |= GPIO_BSRR_BR6)

#define SDAH                        (I2C_GPIO->BSRR |= GPIO_BSRR_BS7)
#define SDAL                        (I2C_GPIO->BSRR |= GPIO_BSRR_BR7)
#define SCLread                     ((I2C_GPIO->IDR & GPIO_Pin_SCL)==GPIO_Pin_SCL)
#define SDAread                     ((I2C_GPIO->IDR & GPIO_Pin_SDA)==GPIO_Pin_SDA)

#define I2C_RESULT_SUCCESS          0
#define I2C_RESULT_ERROR            (-1)

// Define units for conversions. 
typedef enum
{
	CELSIUS,
	FAHRENHEIT,
} temperature_units;

// Define measurement type.
typedef enum
{	
	PRESSURE = 0x00,
	TEMPERATURE = 0x10
} measurement;

// Define constants for Conversion precision
typedef enum
{
	ADC_256  = 0x00,
	ADC_512  = 0x02,
	ADC_1024 = 0x04,
	ADC_2048 = 0x06,
	ADC_4096 = 0x08
} precision;

//Commands
#define CMD_RESET 0x1E // reset command 
#define CMD_ADC_READ 0x00 // ADC read command 
#define CMD_ADC_CONV 0x40 // ADC conversion command 

#define CMD_PROM 0xA0 // Coefficient location

void MS5803_Init(void);
uint8_t i2cSoft_Start(void);
void i2cSoft_Stop(void);
void i2cSoft_Ack(void);
void i2cSoft_NoAck(void);
uint8_t i2cSoft_WaitAck(void);
void i2cSoft_PutByte ( uint8_t data );
uint8_t i2cSoft_GetByte(void);
int i2cSoft_WriteBuffer ( uint8_t chipAddress, uint8_t *buffer, uint32_t sizeOfBuffer );
int i2cSoft_WriteCMD ( uint8_t chipAddress, uint8_t CMD );
int i2cSoft_ReadBuffer ( uint8_t chipAddress, uint8_t *buffer, uint32_t sizeOfBuffer );

void MS5803_reset(void);	 //Reset device
void MS5803_begin(void); // Collect coefficients from sensor
void MS5803_getFullData(temperature_units units, precision _precision, float Data[]);

void getMeasurements(precision _precision);

uint32_t getADCconversion(measurement _measurement, precision _precision);	// Retrieve ADC result

#endif
