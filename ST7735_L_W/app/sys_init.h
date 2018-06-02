#ifndef SYS_INIT_H
#define SYS_INIT_H

uint32_t GetTicks(void);
void Delay_us(uint32_t dlyTicks);
void Delay_ms(uint32_t dlyTicks);
void init_clock(void);

#endif
