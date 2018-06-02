#ifndef WB_FLASH_H
#define WB_FLASH_H

#define WBFLASH_CS_LOW		GPIOB->BSRR |= GPIO_BSRR_BR13;
#define WBFLASH_CS_HIGH		GPIOB->BSRR |= GPIO_BSRR_BS13;
#define WBFLASH_CLK_LOW		GPIOA->BSRR |= GPIO_BSRR_BR5;
#define WBFLASH_CLK_HIGH	GPIOA->BSRR |= GPIO_BSRR_BS5;
#define WBFLASH_MOSI_LOW	GPIOA->BSRR |= GPIO_BSRR_BR7;
#define WBFLASH_MOSI_HIGH	GPIOA->BSRR |= GPIO_BSRR_BS7;

void InitWBSPI_Pins(void);
void SendFlashCmd(uint8_t cmd);
/*void WriteEnable(void);
void ChipErase(void);
void FlashPowerDown(void);
void FlashPowerUp(void);*/

//uint16_t ReadStatusRegister(uint8_t cmd);
uint8_t ReadData(uint32_t addr);
//void SectorErase(uint32_t addr);
//void ProgramData(uint32_t addr, uint8_t value);
void ProgramArray8(uint32_t addr, uint8_t value[8]);

#endif
