#ifndef SYS_INIT_H
#define SYS_INIT_H

#define L1_ON	(GPIOA->BSRR |= GPIO_BSRR_BS5)
#define L1_OFF	(GPIOA->BSRR |= GPIO_BSRR_BR5)

#define L2_ON	(GPIOA->BSRR |= GPIO_BSRR_BS4)
#define L2_OFF	(GPIOA->BSRR |= GPIO_BSRR_BR4)

#define L3_ON	(GPIOA->BSRR |= GPIO_BSRR_BS6)
#define L3_OFF	(GPIOA->BSRR |= GPIO_BSRR_BR6)

#define L4_ON	(GPIOA->BSRR |= GPIO_BSRR_BS3)
#define L4_OFF	(GPIOA->BSRR |= GPIO_BSRR_BR3)

#define L5_ON	(GPIOA->BSRR |= GPIO_BSRR_BS7)
#define L5_OFF	(GPIOA->BSRR |= GPIO_BSRR_BR7)

#define L6_ON	(GPIOA->BSRR |= GPIO_BSRR_BS2)
#define L6_OFF	(GPIOA->BSRR |= GPIO_BSRR_BR2)

#define L7_ON	(GPIOB->BSRR |= GPIO_BSRR_BS0)
#define L7_OFF	(GPIOB->BSRR |= GPIO_BSRR_BR0)

#define L8_ON	(GPIOA->BSRR |= GPIO_BSRR_BS1)
#define L8_OFF	(GPIOA->BSRR |= GPIO_BSRR_BR1)

#define L9_ON	(GPIOB->BSRR |= GPIO_BSRR_BS1)
#define L9_OFF	(GPIOB->BSRR |= GPIO_BSRR_BR1)

#define L10_ON	(GPIOA->BSRR |= GPIO_BSRR_BS0)
#define L10_OFF	(GPIOA->BSRR |= GPIO_BSRR_BR0)

void WAIT(uint32_t dlyTicks);
void init_clock(void);
void Start(void);

#endif
