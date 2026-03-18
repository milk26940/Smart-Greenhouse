#include "co2_uart_protocol.h"

void Co2UartParser_Init(Co2UartParser *parser)
{
  if (parser == 0)
  {
    return;
  }

  parser->length = 0U;
}

uint8_t Co2UartParser_PushByte(Co2UartParser *parser, uint8_t byte, uint16_t *co2_ppm)
{
  uint16_t value;
  uint8_t checksum;

  if ((parser == 0) || (co2_ppm == 0))
  {
    return 0U;
  }

  if ((parser->length == 0U) && (byte != 0x2CU))
  {
    return 0U;
  }

  parser->buffer[parser->length++] = byte;
  if (parser->length < sizeof(parser->buffer))
  {
    return 0U;
  }

  checksum = (uint8_t)(parser->buffer[0] +
                       parser->buffer[1] +
                       parser->buffer[2] +
                       parser->buffer[3] +
                       parser->buffer[4]);

  if ((parser->buffer[0] == 0x2CU) &&
      (parser->buffer[3] == 0x03U) &&
      (parser->buffer[4] == 0xFFU) &&
      (parser->buffer[5] == checksum))
  {
    value = (uint16_t)(((uint16_t)parser->buffer[1] << 8) | parser->buffer[2]);
    *co2_ppm = value;
    parser->length = 0U;
    return 1U;
  }

  parser->length = 0U;
  if (byte == 0x2CU)
  {
    parser->buffer[parser->length++] = byte;
  }
  return 0U;
}
