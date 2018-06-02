#ifndef SYS_INIT_H
#define SYS_INIT_H

void Delay_us(uint32_t dlyTicks);
void Delay_ms(uint32_t dlyTicks);
uint32_t Ticks(void);
void init_clock(void);
//void initRCC_clock(void);

#endif
