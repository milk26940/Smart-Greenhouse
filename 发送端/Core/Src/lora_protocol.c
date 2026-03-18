#include "lora_protocol.h"

#include <stdio.h>
#include <string.h>

int LoraProtocol_BuildUplinkFrame(char *buffer,
                                  uint16_t buffer_size,
                                  const SensorData *sensor,
                                  const MotorStatus *motor)
{
  int written;

  if ((buffer == NULL) || (sensor == NULL) || (motor == NULL) || (buffer_size == 0U))
  {
    return -1;
  }

  written = snprintf(buffer,
                     buffer_size,
                     "ENV,T=%d.%d,H=%u.%u,L=%u,CO2=%u,M=%s,S=%u",
                     sensor->temperature_tenths / 10,
                     sensor->temperature_tenths >= 0 ? sensor->temperature_tenths % 10 : -(sensor->temperature_tenths % 10),
                     sensor->humidity_tenths / 10U,
                     sensor->humidity_tenths % 10U,
                     sensor->light_raw,
                     sensor->co2_ppm,
                     motor->enabled ? "ON" : "OFF",
                     motor->speed_percent);

  if ((written < 0) || ((uint16_t)written >= buffer_size))
  {
    return -1;
  }

  return written;
}

uint8_t LoraProtocol_ParseDownlinkCommand(const char *frame, LoraMotorCommand *command)
{
  char mode[4];
  int speed = 0;

  if ((frame == NULL) || (command == NULL))
  {
    return 0;
  }

  if (sscanf(frame, "CTRL,M=%3[^,],S=%d", mode, &speed) != 2)
  {
    return 0;
  }

  if ((strcmp(mode, "ON") != 0) && (strcmp(mode, "OFF") != 0))
  {
    return 0;
  }

  if ((speed < 0) || (speed > 100))
  {
    return 0;
  }

  command->enabled = (strcmp(mode, "ON") == 0) ? 1U : 0U;
  command->speed_percent = (uint8_t)speed;
  return 1U;
}
