#ifndef __LORA_DOWNLINK_FRAMER_H
#define __LORA_DOWNLINK_FRAMER_H

#include <stdint.h>

#include "lora_protocol.h"

typedef struct
{
  char buffer[64];
  uint16_t length;
  uint8_t overflowed;
} LoraDownlinkFramer;

void LoraDownlinkFramer_Init(LoraDownlinkFramer *framer);
int LoraDownlinkFramer_PushByte(LoraDownlinkFramer *framer,
                                uint8_t byte,
                                LoraMotorCommand *command);

#endif
