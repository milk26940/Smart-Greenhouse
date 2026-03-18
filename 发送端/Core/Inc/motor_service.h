#ifndef __MOTOR_SERVICE_H
#define __MOTOR_SERVICE_H

#include "lora_protocol.h"

void MotorService_Init(void);
void MotorService_ApplyCommand(const LoraMotorCommand *command);
MotorStatus MotorService_GetStatus(void);

#endif
