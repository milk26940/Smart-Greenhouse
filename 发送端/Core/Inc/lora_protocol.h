#ifndef __LORA_PROTOCOL_H
#define __LORA_PROTOCOL_H

#include <stdint.h>

typedef struct
{
  int16_t temperature_tenths;
  uint16_t humidity_tenths;
  uint16_t light_raw;
  uint16_t co2_ppm;
} SensorData;

typedef struct
{
  uint8_t enabled;
  uint8_t speed_percent;
} MotorStatus;

typedef struct
{
  uint8_t enabled;
  uint8_t speed_percent;
} LoraMotorCommand;

int LoraProtocol_BuildUplinkFrame(char *buffer,
                                  uint16_t buffer_size,
                                  const SensorData *sensor,
                                  const MotorStatus *motor);

uint8_t LoraProtocol_ParseDownlinkCommand(const char *frame, LoraMotorCommand *command);

#endif
