#include "MS5803_I2C.h"

volatile int32_t _temperature_actual;
volatile int32_t _pressure_actual;
volatile uint16_t coefficient[6]; // Coefficients;

static void Delay(void)
{
    volatile uint16_t i = 240;	//	100kHz on 72MHz
    while ( i ) {
        i--;
    }
}

void MS5803_Init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);
	GPIO_InitTypeDef GPIO_InitStructure;

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOB,&GPIO_InitStructure);

	GPIOB->BSRR |= GPIO_BSRR_BS5;

	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	GPIOB->BSRR |= GPIO_BSRR_BS6 | GPIO_BSRR_BS7;
}

void MS5803_Deinit(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	GPIO_InitStructure.GPIO_Pin = (GPIO_Pin_6 | GPIO_Pin_7 | GPIO_Pin_5);
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_Init(GPIOB,&GPIO_InitStructure);
}

uint8_t i2cSoft_Start()
{
    SDAH;
    SCLH;
    Delay();
    if ( !(SDAread) )
        return 0x00;
    SDAL;
    Delay();
    if ( SDAread )
        return 0x00;
    Delay();
    return 0x01;
}

void i2cSoft_Stop()
{
    SCLL;                       // Stop sequence
    Delay();
    SDAL;
    Delay();
    SCLH;
    Delay();
    SDAH;
    Delay();
}

void i2cSoft_Ack (void)
{
    SCLL;
    Delay();
    SDAL;
    Delay();
    SCLH;
    Delay();
    SCLL;
    Delay();
}

void i2cSoft_NoAck (void)
{
    SCLL;
    Delay();
    SDAH;
    Delay();
    SCLH;
    Delay();
    SCLL;
    Delay();
}

uint8_t i2cSoft_WaitAck(void)
{
    SCLL;
    Delay();
    SDAH;
    Delay();
    SCLH;
    Delay();
    if ( SDAread ) {
        SCLL;
        return 0x00;
    }
    SCLL;
    return 0x01;
}

void i2cSoft_PutByte ( uint8_t data )
{
    uint8_t i = 8;
    while ( i-- ) {
        SCLL;
        Delay();
        if ( data & 0x80 )
            SDAH;
        else
            SDAL;
        data <<= 1;
        Delay();
        SCLH;
        Delay();
    }
    SCLL;
}

uint8_t i2cSoft_GetByte (void)
{
    volatile uint8_t i = 8;
    uint8_t data = 0;

    SDAH;
    while ( i-- ) {
        data <<= 1;
        SCLL;
        Delay();
        SCLH;
        Delay();
        if ( SDAread ) {
            data |= 0x01;
        }
    }
    SCLL;
    return data;
}

int i2cSoft_WriteBuffer ( uint8_t chipAddress, uint8_t *buffer, uint32_t sizeOfBuffer )
{
    if ( !i2cSoft_Start() )
        return I2C_RESULT_ERROR;

    i2cSoft_PutByte( chipAddress );
    if ( !i2cSoft_WaitAck() ) {
       // i2cSoft_Stop();
        //return I2C_RESULT_ERROR;
    }

    while ( sizeOfBuffer != 0 ) {
        i2cSoft_PutByte( *buffer );
        if ( !i2cSoft_WaitAck() ) {
            i2cSoft_Stop();
            return I2C_RESULT_ERROR;
        }

        buffer++;
        sizeOfBuffer--;
    }
    i2cSoft_Stop();
    return I2C_RESULT_SUCCESS;
}

int i2cSoft_WriteCMD ( uint8_t chipAddress, uint8_t CMD )
{
    if ( !i2cSoft_Start() )
        return I2C_RESULT_ERROR;

    i2cSoft_PutByte( chipAddress );
    if ( !i2cSoft_WaitAck() ) {
       // i2cSoft_Stop();
        //return I2C_RESULT_ERROR;
    }

	i2cSoft_PutByte( CMD );
	if ( !i2cSoft_WaitAck() ) {
		//i2cSoft_Stop();
		//return I2C_RESULT_ERROR;
	}

    i2cSoft_Stop();
    return I2C_RESULT_SUCCESS;
}

int i2cSoft_ReadBuffer ( uint8_t chipAddress, uint8_t *buffer, uint32_t sizeOfBuffer )
{
    if ( !i2cSoft_Start() )
        return I2C_RESULT_ERROR;

    i2cSoft_PutByte( chipAddress + 1 );
    if ( !i2cSoft_WaitAck() ) {
       // i2cSoft_Stop();
       // return I2C_RESULT_ERROR;
    }

    while ( sizeOfBuffer != 0 ) {
        *buffer = i2cSoft_GetByte();

        buffer++;
        sizeOfBuffer--;
        if ( sizeOfBuffer == 0 ) {
            i2cSoft_NoAck();
            break;
        }
        else
            i2cSoft_Ack();
    }
    i2cSoft_Stop();
    return I2C_RESULT_SUCCESS;
}

void MS5803_reset(void)
// Reset device I2C
{
	i2cSoft_WriteCMD (0xEE, 0x1E);
	Delay_ms(3);
}

void MS5803_begin(void)
// Initialize library for subsequent pressure measurements
{  
	uint8_t i;//, highByte, lowByte;
	for(i = 0; i < 6; i++)
	{
		uint8_t bytes[2];
		i2cSoft_WriteCMD (0xEE, 0xA2 + 2*i);
		i2cSoft_ReadBuffer (0xEE, bytes, 2);
		Delay_ms(3);
		coefficient[i] = (bytes[0] << 8)|bytes[1];
	}
}

void MS5803_getFullData(temperature_units units, precision _precision, double Data[])
{
	getMeasurements(_precision);
	double temperature_reported = (double)(_temperature_actual) / 100;
	// If Fahrenheit is selected return the temperature converted to F
	if(units == FAHRENHEIT){
		temperature_reported = (((temperature_reported) * 9) / 5) + 32;
		}
	Data[0] = temperature_reported;
	Data[1] = (double)(_pressure_actual)/10;
}

void getMeasurements(precision _precision)

{
	//Retrieve ADC result
	int32_t temperature_raw = getADCconversion(TEMPERATURE, _precision);
	int32_t pressure_raw = getADCconversion(PRESSURE, _precision);
	
	
	//Create Variables for calculations
	int32_t temp_calc;
	
	int32_t dT;
		
	//Now that we have a raw temperature, let's compute our actual.
	dT = temperature_raw - ((int32_t)coefficient[4] << 8);
	temp_calc = (((int64_t)dT * coefficient[5]) >> 23) + 2000;
	
	// TESTING  _temperature_actual = temp_calc;
	
	//Now we have our first order Temperature, let's calculate the second order.
	int64_t T2, OFF2, SENS2, OFF, SENS; //working variables

	if (temp_calc < 2000) 
	// If temp_calc is below 20.0C
	{	
		T2 = 3 * (((int64_t)dT * dT) >> 33);
		OFF2 = 3 * ((temp_calc - 2000) * (temp_calc - 2000)) / 2;
		SENS2 = 5 * ((temp_calc - 2000) * (temp_calc - 2000)) / 8;
		
		if(temp_calc < -1500)
		// If temp_calc is below -15.0C 
		{
			OFF2 = OFF2 + 7 * ((temp_calc + 1500) * (temp_calc + 1500));
			SENS2 = SENS2 + 4 * ((temp_calc + 1500) * (temp_calc + 1500));
		}
    } 
	else
	// If temp_calc is above 20.0C
	{ 
		T2 = ((7*((uint64_t)dT) * ((uint64_t)dT)) >> 37);
		OFF2 = ((temp_calc - 2000) * (temp_calc - 2000)) / 16;
		SENS2 = 0;
	}
	
	// Now bring it all together to apply offsets 
	
	OFF = ((int64_t)coefficient[1] << 16) + (((coefficient[3] * (int64_t)dT)) >> 7);
	SENS = ((int64_t)coefficient[0] << 15) + (((coefficient[2] * (int64_t)dT)) >> 8);
	
	_temperature_actual = temp_calc - T2;
	OFF = OFF - OFF2;
	SENS = SENS - SENS2;

	_pressure_actual = (((SENS * pressure_raw) / 2097152 ) - OFF) / 32768;
}

uint32_t getADCconversion(measurement _measurement, precision _precision)
// Retrieve ADC measurement from the device.  
// Select measurement type and precision
{	
	//uint8_t highByte, midByte, lowByte;
	uint8_t bytes[3];
	
	i2cSoft_WriteCMD (0xEE, 0x40 + _measurement + _precision);
	// Wait for conversion to complete
	Delay_ms(1); //general delay
	switch( _precision )
	{ 
		case ADC_256 : Delay_ms(1); break;
		case ADC_512 : Delay_ms(3); break;
		case ADC_1024: Delay_ms(4); break;
		case ADC_2048: Delay_ms(6); break;
		case ADC_4096: Delay_ms(10); break;
	}	
	
	i2cSoft_WriteCMD (0xEE, 0x00);
	i2cSoft_ReadBuffer (0xEE, bytes, 3);
	
	return ((uint32_t)bytes[0] << 16) + ((uint32_t)bytes[1] << 8) + bytes[2];
}


