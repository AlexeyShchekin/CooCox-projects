// ILI9341_LowLevel.h

// Инициализация SPI и пинов
void InitPins ( void );

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
