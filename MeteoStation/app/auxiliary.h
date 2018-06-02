#ifndef AUXILIARY_H
#define AUXILIARY_H
#include <stdlib.h>

int uart_putc(int c, USART_TypeDef* USARTx);
int uart_getc (USART_TypeDef* USARTx);

void send_to_uart(uint8_t data);
void send_str_to_uart(char *str, int length);
void str_to_uart(char *str);
void send16_to_uart(uint16_t data);
void send32_to_uart(uint32_t data);
void USART1_ON(void);
void USART3_ON(void);
void USART1_OFF(void);
void USART3_OFF(void);
void NVIC_Configuration(void);
void EXTI_Configuration(void);

void InitSensors(void);
void TempToStr(float temp, char dest[]);

#endif
