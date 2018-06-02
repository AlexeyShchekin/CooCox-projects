// ILI9341_LowLevel.h

#define TFT_PORT				GPIOB
#define TFT_SPI					SPI1

#define PIN_RST		GPIO_Pin_0
#define PIN_DC		GPIO_Pin_1
#define PIN_CS		GPIO_Pin_4

#define PIN_SCK		GPIO_Pin_5
#define PIN_MISO	GPIO_Pin_6
#define PIN_MOSI	GPIO_Pin_7

void ILI9341_SPI_SpeedSwitch(void);
void SPI_ON(void);
void SPI_OFF(void);

void Sensor_ON(void);
void Sensor_OFF(void);

//void InitPins ( void );

void TFT_CS_LOW ( void );
void TFT_CS_HIGH ( void );
void TFT_DC_LOW ( void );
void TFT_DC_HIGH ( void );
void TFT_RST_LOW ( void );
void TFT_RST_HIGH ( void );

uint8_t TFT_sendByte ( uint8_t byte );
void TFT_sendWord ( uint16_t data );
void TFT_sendCMD ( uint8_t index );
uint8_t TFT_sendDATA ( uint8_t data );
uint8_t TFT_Read_Register ( uint8_t Addr, uint8_t xParameter );
