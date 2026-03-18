#ifndef __SERIAL_H
#define __SERIAL_H

#include <stdint.h>
#include <stdio.h>
#include "main.h"
#include "lora_protocol.h"

extern char Serial_RxPacket[];
extern uint8_t Serial_RxFlag;
extern uint8_t Serial_RxData;
extern uint16_t Serial_RxIndex;
extern void *Serial_RxSemaphore;

void Serial_Init(void);
void Serial_Process(void);
void Serial_RunStartupConfig(void);
uint8_t Serial_WaitDownlinkCommand(LoraMotorCommand *command, uint32_t wait_ms);
uint8_t Serial_SendUplinkFrame(const SensorData *sensor, const MotorStatus *motor);
void Serial_Printf(char *format, ...);
void Serial_SendString(char *String);
void Serial_SendByte(uint8_t Byte);
char *Serial_GetRxPacket(void);

#endif
