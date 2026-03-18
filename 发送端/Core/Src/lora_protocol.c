#include "lora_protocol.h"

#include <stdio.h>
#include <string.h>

static int lora_protocol_finalize_write(int written, uint16_t buffer_size)
{
  if ((written < 0) || ((uint16_t)written >= buffer_size))
  {
    return -1;
  }

  return written;
}

int LoraProtocol_BuildEnvironmentFrame(char *buffer,
                                       uint16_t buffer_size,
                                       uint8_t node_id,
                                       const SensorData *sensor)
{
  int written;

  if ((buffer == NULL) || (sensor == NULL) || (buffer_size == 0U) || (node_id == 0U))
  {
    return -1;
  }

  written = snprintf(buffer,
                     buffer_size,
                     "ENV,N=%u,T=%d.%d,H=%u.%u,SOIL=%u.%u,L=%u,CO2=%u",
                     node_id,
                     sensor->temperature_tenths / 10,
                     sensor->temperature_tenths >= 0 ? sensor->temperature_tenths % 10 : -(sensor->temperature_tenths % 10),
                     sensor->humidity_tenths / 10U,
                     sensor->humidity_tenths % 10U,
                     sensor->soil_tenths / 10U,
                     sensor->soil_tenths % 10U,
                     sensor->light_raw,
                     sensor->co2_ppm);

  return lora_protocol_finalize_write(written, buffer_size);
}

int LoraProtocol_BuildStatusFrame(char *buffer,
                                  uint16_t buffer_size,
                                  uint8_t node_id,
                                  const ActuatorStatus *status)
{
  int written;

  if ((buffer == NULL) || (status == NULL) || (buffer_size == 0U) || (node_id == 0U))
  {
    return -1;
  }

  written = snprintf(buffer,
                     buffer_size,
                     "STAT,N=%u,F=%s,P=%s,LED=%s",
                     node_id,
                     status->fan_on ? "ON" : "OFF",
                     status->pump_on ? "ON" : "OFF",
                     status->fill_light_on ? "ON" : "OFF");

  return lora_protocol_finalize_write(written, buffer_size);
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
