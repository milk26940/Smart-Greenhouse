#include "lora_downlink_framer.h"

#include <string.h>

void LoraDownlinkFramer_Init(LoraDownlinkFramer *framer)
{
  if (framer == 0)
  {
    return;
  }

  framer->length = 0U;
  framer->overflowed = 0U;
  framer->buffer[0] = '\0';
}

int LoraDownlinkFramer_PushByte(LoraDownlinkFramer *framer,
                                uint8_t byte,
                                LoraMotorCommand *command)
{
  if ((framer == 0) || (command == 0))
  {
    return -1;
  }

  if ((byte == '\r') || (byte == '\n'))
  {
    if (framer->overflowed != 0U)
    {
      LoraDownlinkFramer_Init(framer);
      return 0;
    }

    if (framer->length == 0U)
    {
      return 0;
    }

    framer->buffer[framer->length] = '\0';
    if (strncmp(framer->buffer, "CTRL,", 5) != 0)
    {
      LoraDownlinkFramer_Init(framer);
      return 0;
    }

    if (LoraProtocol_ParseDownlinkCommand(framer->buffer, command) != 0U)
    {
      LoraDownlinkFramer_Init(framer);
      return 1;
    }

    LoraDownlinkFramer_Init(framer);
    return -1;
  }

  if (framer->overflowed != 0U)
  {
    return -1;
  }

  if (framer->length >= (sizeof(framer->buffer) - 1U))
  {
    framer->overflowed = 1U;
    framer->length = 0U;
    framer->buffer[0] = '\0';
    return -1;
  }

  framer->buffer[framer->length++] = (char)byte;
  return 0;
}
