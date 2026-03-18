#ifndef __CO2_UART_PROTOCOL_H
#define __CO2_UART_PROTOCOL_H

#include <stdint.h>

typedef struct
{
  uint8_t buffer[6];
  uint8_t length;
} Co2UartParser;

void Co2UartParser_Init(Co2UartParser *parser);
uint8_t Co2UartParser_PushByte(Co2UartParser *parser, uint8_t byte, uint16_t *co2_ppm);

#endif
