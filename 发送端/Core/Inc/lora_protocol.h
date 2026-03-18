#ifndef __LORA_PROTOCOL_H
#define __LORA_PROTOCOL_H

#include <stdint.h>

typedef struct
{
  int16_t temperature_tenths;
  uint16_t humidity_tenths;
  uint16_t soil_tenths;
  uint16_t light_raw;
  uint16_t co2_ppm;
} SensorData;

typedef struct
{
  uint8_t fan_on;
  uint8_t pump_on;
  uint8_t fill_light_on;
} ActuatorStatus;

typedef struct
{
  uint8_t enabled;
  uint8_t speed_percent;
} LoraMotorCommand;

int LoraProtocol_BuildEnvironmentFrame(char *buffer,
                                       uint16_t buffer_size,
                                       uint8_t node_id,
                                       const SensorData *sensor);

int LoraProtocol_BuildStatusFrame(char *buffer,
                                  uint16_t buffer_size,
                                  uint8_t node_id,
                                  const ActuatorStatus *status);

uint8_t LoraProtocol_ParseDownlinkCommand(const char *frame, LoraMotorCommand *command);

#endif
