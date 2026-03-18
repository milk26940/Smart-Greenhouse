#ifndef __SENSOR_SERVICE_H
#define __SENSOR_SERVICE_H

#include "lora_protocol.h"
#include "stm32f1xx_hal.h"

void SensorService_Init(void);
void SensorService_Process(void);
SensorData SensorService_Read(void);
void SensorService_Sample(SensorData *out);
SensorData SensorService_ReadLatest(void);
void SensorService_OnUart3RxCplt(UART_HandleTypeDef *huart);

#endif
