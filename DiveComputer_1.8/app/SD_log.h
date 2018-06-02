#ifndef SD_LOG_H
#define SD_LOG_H

#define GO_IDLE_STATE 0
#define SEND_IF_COND 8
#define READ_SINGLE_BLOCK 17
#define WRITE_SINGLE_BLOCK 24
#define SD_SEND_OP_COND 41
#define APP_CMD 55
#define READ_OCR 58

#define CS_SD_ENABLE         GPIOA->BSRR = GPIO_BSRR_BR4;
#define CS_SD_DISABLE    	  GPIOA->BSRR = GPIO_BSRR_BS4;

uint8_t SD_SPI_send (uint8_t data);
uint8_t SD_SPI_read (void);
void SD_SPI_SpeedSwitch(void);
void SD_SPI_init(void);
uint8_t SD_sendCommand(uint8_t cmd, uint32_t arg);
uint8_t SD_init(void);

#endif
