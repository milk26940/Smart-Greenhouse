/**
 * @file Serial.c
 * @brief 发送端串口通信与 LoRa 业务逻辑实现
 */

#include "Serial.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "FreeRTOS.h"
#include "lora_downlink_framer.h"
#include "queue.h"
#include "task.h"
#include "app_node_role.h"
#include "lora_protocol.h"
#include "sensor_service.h"
#include "usart.h"

char Serial_RxPacket[1] = {0};
uint8_t Serial_RxFlag = 0;
uint8_t Serial_RxData = 0;
uint16_t Serial_RxIndex = 0;
void *Serial_RxSemaphore = NULL;

static uint8_t lora_rx_byte = 0;
static uint8_t rx_debug_buffer[128];
static uint16_t rx_debug_length = 0;
static uint32_t rx_last_tick = 0;
static QueueHandle_t lora_rx_queue = NULL;
static LoraDownlinkFramer downlink_framer;

static void debug_send_bytes(const uint8_t *data, uint16_t length)
{
  HAL_UART_Transmit(&huart1, (uint8_t *)data, length, HAL_MAX_DELAY);
}

static void debug_send_tagged_frame(const char *tag,
                                    const uint8_t *data,
                                    uint16_t length,
                                    uint8_t append_crlf,
                                    uint8_t force_crlf)
{
  static const uint8_t crlf[] = "\r\n";

  debug_send_bytes((const uint8_t *)tag, (uint16_t)strlen(tag));
  if (length > 0U)
  {
    debug_send_bytes(data, length);
  }

  if (append_crlf || force_crlf)
  {
    debug_send_bytes(crlf, 2);
  }
}

static void debug_flush_rx_buffer(uint8_t force_crlf)
{
  if (rx_debug_length == 0U)
  {
    return;
  }

  debug_send_tagged_frame("RX:", rx_debug_buffer, rx_debug_length, 0, force_crlf);
  rx_debug_length = 0U;
}

static void lora_send_bytes(const uint8_t *data, uint16_t length, uint8_t append_crlf)
{
  debug_flush_rx_buffer(1);
  debug_send_tagged_frame("TX:", data, length, append_crlf, append_crlf ? 0U : 1U);
  HAL_UART_Transmit(&huart2, (uint8_t *)data, length, HAL_MAX_DELAY);

  if (append_crlf)
  {
    static const uint8_t crlf[] = "\r\n";
    HAL_UART_Transmit(&huart2, (uint8_t *)crlf, 2, HAL_MAX_DELAY);
  }
}

static uint8_t lora_aux_is_idle(void)
{
  return HAL_GPIO_ReadPin(LORA_AUX_GPIO_Port, LORA_AUX_Pin) == GPIO_PIN_RESET;
}

static void lora_wait_idle(uint32_t timeout_ms)
{
  uint32_t start_tick = HAL_GetTick();

  while (!lora_aux_is_idle())
  {
    if ((HAL_GetTick() - start_tick) >= timeout_ms)
    {
      break;
    }
  }
}

static void lora_send_command(const char *cmd, uint8_t append_crlf)
{
  lora_wait_idle(500U);
  lora_send_bytes((const uint8_t *)cmd, (uint16_t)strlen(cmd), append_crlf);
  vTaskDelay(pdMS_TO_TICKS(100U));
  lora_wait_idle(500U);
}

void Serial_RunStartupConfig(void)
{
  lora_send_command("+++", 0U);
  lora_send_command("AT+MODE0", 1U);
  lora_send_command("AT+LEVEL2", 1U);
  lora_send_command("AT+CHANNEL01", 1U);
  lora_send_command("AT+RESET", 1U);
  vTaskDelay(pdMS_TO_TICKS(500U));
  lora_wait_idle(1000U);
}

void Serial_Init(void)
{
  if (lora_rx_queue == NULL)
  {
    lora_rx_queue = xQueueCreate(64U, sizeof(uint8_t));
  }
  LoraDownlinkFramer_Init(&downlink_framer);
  HAL_UART_Receive_IT(&huart2, &lora_rx_byte, 1);
}

void Serial_Process(void)
{
}

uint8_t Serial_WaitDownlinkCommand(LoraMotorCommand *command, uint32_t wait_ms)
{
  uint8_t byte = 0;
  int parse_result;

  if ((command == NULL) || (lora_rx_queue == NULL))
  {
    return 0U;
  }

  if (xQueueReceive(lora_rx_queue, &byte, pdMS_TO_TICKS(wait_ms)) != pdPASS)
  {
    if ((rx_debug_length > 0U) && ((HAL_GetTick() - rx_last_tick) >= 20U))
    {
      debug_flush_rx_buffer(1U);
    }
    return 0U;
  }

  if (rx_debug_length >= sizeof(rx_debug_buffer))
  {
    debug_flush_rx_buffer(1U);
  }

  rx_debug_buffer[rx_debug_length++] = byte;
  rx_last_tick = HAL_GetTick();

  if ((byte == '\n') || (byte == '\r'))
  {
    debug_flush_rx_buffer(0U);
  }

  parse_result = LoraDownlinkFramer_PushByte(&downlink_framer, byte, command);
  if (parse_result > 0)
  {
    Serial_Printf("CMD:OK\r\n");
    return 1U;
  }
  if (parse_result < 0)
  {
    Serial_Printf("CMD:ERR\r\n");
  }
  return 0U;
}

uint8_t Serial_SendEnvironmentFrame(const SensorData *sensor)
{
  char payload[96];
  int payload_length;

  if (sensor == NULL)
  {
    return 0U;
  }

  payload_length = LoraProtocol_BuildEnvironmentFrame(payload,
                                                      sizeof(payload),
                                                      APP_NODE_ID,
                                                      sensor);
  if (payload_length <= 0)
  {
    Serial_Printf("UPLINK:ERR\r\n");
    return 0U;
  }

  lora_send_bytes((const uint8_t *)payload, (uint16_t)payload_length, 0U);
  lora_wait_idle(1000U);
  return 1U;
}

uint8_t Serial_SendStatusFrame(const ActuatorStatus *status)
{
  char payload[96];
  int payload_length;

  if (status == NULL)
  {
    return 0U;
  }

  payload_length = LoraProtocol_BuildStatusFrame(payload,
                                                 sizeof(payload),
                                                 APP_NODE_ID,
                                                 status);
  if (payload_length <= 0)
  {
    Serial_Printf("UPLINK:ERR\r\n");
    return 0U;
  }

  lora_send_bytes((const uint8_t *)payload, (uint16_t)payload_length, 0U);
  lora_wait_idle(1000U);
  return 1U;
}

void Serial_SendByte(uint8_t Byte)
{
  HAL_UART_Transmit(&huart1, &Byte, 1, HAL_MAX_DELAY);
}

void Serial_SendString(char *String)
{
  Serial_Printf("%s", String);
}

void Serial_Printf(char *format, ...)
{
  char buffer[128];
  va_list args;

  va_start(args, format);
  vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);

  debug_send_bytes((uint8_t *)buffer, (uint16_t)strlen(buffer));
}

char *Serial_GetRxPacket(void)
{
  return NULL;
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART2)
  {
    BaseType_t higher_priority_task_woken = pdFALSE;

    if (lora_rx_queue != NULL)
    {
      xQueueSendFromISR(lora_rx_queue, &lora_rx_byte, &higher_priority_task_woken);
    }

    HAL_UART_Receive_IT(&huart2, &lora_rx_byte, 1);
    portYIELD_FROM_ISR(higher_priority_task_woken);
  }
  else if (huart->Instance == USART3)
  {
    SensorService_OnUart3RxCplt(huart);
  }
}
