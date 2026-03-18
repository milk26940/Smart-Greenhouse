#include "sensor_service.h"

#include "FreeRTOS.h"
#include "adc.h"
#include "co2_uart_protocol.h"
#include "main.h"
#include "queue.h"
#include "usart.h"

static SensorData last_sensor_data = {250, 600, 0, 400};
static uint8_t co2_rx_byte = 0;
static QueueHandle_t co2_rx_queue = NULL;
static Co2UartParser co2_parser;

static void sensor_delay_us(uint32_t us)
{
  uint32_t count = (SystemCoreClock / 8000000U) * us;

  while (count-- != 0U)
  {
    __NOP();
  }
}

static void dht11_set_output(uint8_t high_level)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  GPIO_InitStruct.Pin = DHT11_DATA_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(DHT11_DATA_GPIO_Port, &GPIO_InitStruct);
  HAL_GPIO_WritePin(DHT11_DATA_GPIO_Port,
                    DHT11_DATA_Pin,
                    high_level ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static void dht11_set_input(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  GPIO_InitStruct.Pin = DHT11_DATA_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(DHT11_DATA_GPIO_Port, &GPIO_InitStruct);
}

static uint8_t dht11_wait_for_level(GPIO_PinState state, uint32_t timeout_us)
{
  while (timeout_us-- != 0U)
  {
    if (HAL_GPIO_ReadPin(DHT11_DATA_GPIO_Port, DHT11_DATA_Pin) == state)
    {
      return 1U;
    }
    sensor_delay_us(1);
  }

  return 0U;
}

static uint8_t dht11_read(SensorData *sensor)
{
  uint8_t bytes[5] = {0};
  uint8_t byte_index;
  uint8_t bit_index;

  dht11_set_output(0U);
  HAL_Delay(20);
  dht11_set_output(1U);
  sensor_delay_us(30);
  dht11_set_input();

  if (!dht11_wait_for_level(GPIO_PIN_RESET, 100U))
  {
    return 0U;
  }
  if (!dht11_wait_for_level(GPIO_PIN_SET, 100U))
  {
    return 0U;
  }
  if (!dht11_wait_for_level(GPIO_PIN_RESET, 100U))
  {
    return 0U;
  }

  for (byte_index = 0U; byte_index < 5U; ++byte_index)
  {
    for (bit_index = 0U; bit_index < 8U; ++bit_index)
    {
      if (!dht11_wait_for_level(GPIO_PIN_SET, 70U))
      {
        return 0U;
      }

      sensor_delay_us(40);
      bytes[byte_index] <<= 1;
      if (HAL_GPIO_ReadPin(DHT11_DATA_GPIO_Port, DHT11_DATA_Pin) == GPIO_PIN_SET)
      {
        bytes[byte_index] |= 0x01U;
      }

      if (!dht11_wait_for_level(GPIO_PIN_RESET, 70U))
      {
        return 0U;
      }
    }
  }

  if (((uint8_t)(bytes[0] + bytes[1] + bytes[2] + bytes[3])) != bytes[4])
  {
    return 0U;
  }

  sensor->humidity_tenths = (uint16_t)bytes[0] * 10U + bytes[1];
  sensor->temperature_tenths = (int16_t)bytes[2] * 10 + bytes[3];
  return 1U;
}

static uint16_t light_sensor_read(void)
{
  if (HAL_ADC_Start(&hadc1) != HAL_OK)
  {
    return last_sensor_data.light_raw;
  }

  if (HAL_ADC_PollForConversion(&hadc1, 10U) != HAL_OK)
  {
    HAL_ADC_Stop(&hadc1);
    return last_sensor_data.light_raw;
  }

  last_sensor_data.light_raw = (uint16_t)HAL_ADC_GetValue(&hadc1);
  HAL_ADC_Stop(&hadc1);
  return last_sensor_data.light_raw;
}

static void co2_drain_queue(void)
{
  uint8_t byte = 0;
  uint16_t co2_ppm = 0U;

  while (xQueueReceive(co2_rx_queue, &byte, 0U) == pdPASS)
  {
    if (Co2UartParser_PushByte(&co2_parser, byte, &co2_ppm) != 0U)
    {
      last_sensor_data.co2_ppm = co2_ppm;
    }
  }
}

void SensorService_Init(void)
{
  if (co2_rx_queue == NULL)
  {
    co2_rx_queue = xQueueCreate(32U, sizeof(uint8_t));
  }
  Co2UartParser_Init(&co2_parser);
  HAL_ADCEx_Calibration_Start(&hadc1);
  HAL_UART_Receive_IT(&huart3, &co2_rx_byte, 1);
}

void SensorService_Process(void)
{
  co2_drain_queue();
}

SensorData SensorService_Read(void)
{
  SensorService_Sample(&last_sensor_data);
  return last_sensor_data;
}

void SensorService_Sample(SensorData *out)
{
  SensorData current = last_sensor_data;

  co2_drain_queue();

  if (dht11_read(&current) != 0U)
  {
    last_sensor_data.temperature_tenths = current.temperature_tenths;
    last_sensor_data.humidity_tenths = current.humidity_tenths;
  }

  last_sensor_data.light_raw = light_sensor_read();
  if (out != NULL)
  {
    *out = last_sensor_data;
  }
}

SensorData SensorService_ReadLatest(void)
{
  return last_sensor_data;
}

void SensorService_OnUart3RxCplt(UART_HandleTypeDef *huart)
{
  BaseType_t higher_priority_task_woken = pdFALSE;

  if (huart->Instance != USART3)
  {
    return;
  }

  if (co2_rx_queue != NULL)
  {
    xQueueSendFromISR(co2_rx_queue, &co2_rx_byte, &higher_priority_task_woken);
  }

  HAL_UART_Receive_IT(&huart3, &co2_rx_byte, 1);
  portYIELD_FROM_ISR(higher_priority_task_woken);
}
