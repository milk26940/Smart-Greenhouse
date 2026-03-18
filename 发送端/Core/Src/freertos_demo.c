#include "freertos_demo.h"

#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"
#include "app_node_role.h"
#include "main.h"
#include "Serial.h"
#include "motor_service.h"
#include "sensor_service.h"

static QueueHandle_t motor_cmd_queue = NULL;
static SemaphoreHandle_t sensor_mutex = NULL;
static SemaphoreHandle_t motor_mutex = NULL;
static SensorData shared_sensor_data = {250, 600, 350, 0, 400};
static ActuatorStatus shared_actuator_status = {0U, 0U, 0U};

static void app_fault_halt(const char *reason)
{
  taskDISABLE_INTERRUPTS();
  Serial_Printf("RTOS FAULT: %s\r\n", (char *)reason);
  while (1)
  {
  }
}

static SensorData app_read_sensor_snapshot(void)
{
  SensorData snapshot;

  xSemaphoreTake(sensor_mutex, portMAX_DELAY);
  snapshot = shared_sensor_data;
  xSemaphoreGive(sensor_mutex);
  return snapshot;
}

static ActuatorStatus app_read_actuator_snapshot(void)
{
  ActuatorStatus snapshot;

  xSemaphoreTake(motor_mutex, portMAX_DELAY);
  snapshot = shared_actuator_status;
  xSemaphoreGive(motor_mutex);
  return snapshot;
}

static void sensor_task(void *argument)
{
  TickType_t last_wake_time = xTaskGetTickCount();
  SensorData sample;

  (void)argument;
  SensorService_Init();

  for (;;)
  {
    SensorService_Sample(&sample);

    xSemaphoreTake(sensor_mutex, portMAX_DELAY);
    shared_sensor_data = sample;
    xSemaphoreGive(sensor_mutex);

    vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(2000U));
  }
}

static void motor_task(void *argument)
{
  LoraMotorCommand command;

  (void)argument;
  MotorService_Init();

  xSemaphoreTake(motor_mutex, portMAX_DELAY);
  shared_actuator_status = MotorService_GetStatus();
  xSemaphoreGive(motor_mutex);

  for (;;)
  {
    if (xQueueReceive(motor_cmd_queue, &command, portMAX_DELAY) == pdPASS)
    {
      MotorService_ApplyCommand(&command);

      xSemaphoreTake(motor_mutex, portMAX_DELAY);
      shared_actuator_status = MotorService_GetStatus();
      xSemaphoreGive(motor_mutex);
    }
  }
}

static void lora_task(void *argument)
{
  TickType_t next_uplink_tick;
#if (APP_NODE_ROLE == APP_NODE_ROLE_EXECUTOR) && (APP_NODE_DOWNLINK_COMPAT != 0U)
  LoraMotorCommand command;
#endif

  (void)argument;
  Serial_Init();
  Serial_RunStartupConfig();
  next_uplink_tick = xTaskGetTickCount();

  for (;;)
  {
    if ((int32_t)(xTaskGetTickCount() - next_uplink_tick) >= 0)
    {
      if (APP_NODE_ROLE == APP_NODE_ROLE_SENSOR)
      {
        SensorData sensor_snapshot = app_read_sensor_snapshot();

        (void)Serial_SendEnvironmentFrame(&sensor_snapshot);
      }
      else
      {
        ActuatorStatus actuator_snapshot = app_read_actuator_snapshot();

        (void)Serial_SendStatusFrame(&actuator_snapshot);
      }
      next_uplink_tick = xTaskGetTickCount() + pdMS_TO_TICKS(5000U);
    }

#if (APP_NODE_ROLE == APP_NODE_ROLE_EXECUTOR) && (APP_NODE_DOWNLINK_COMPAT != 0U)
    if (Serial_WaitDownlinkCommand(&command, 100U) != 0U)
    {
      (void)xQueueSend(motor_cmd_queue, &command, 0U);
    }
#else
    vTaskDelay(pdMS_TO_TICKS(100U));
#endif
  }
}

void freertos_start(void)
{
  sensor_mutex = xSemaphoreCreateMutex();
  motor_mutex = xSemaphoreCreateMutex();
  motor_cmd_queue = xQueueCreate(4U, sizeof(LoraMotorCommand));

  if ((sensor_mutex == NULL) || (motor_mutex == NULL) || (motor_cmd_queue == NULL))
  {
    return;
  }

  if (APP_NODE_ROLE == APP_NODE_ROLE_SENSOR)
  {
    xTaskCreate(sensor_task, "sensor", 256U, NULL, 2U, NULL);
  }
  else
  {
    xTaskCreate(motor_task, "motor", 256U, NULL, 3U, NULL);
  }
  xTaskCreate(lora_task, "lora", 384U, NULL, 2U, NULL);
  vTaskStartScheduler();
}

void vApplicationMallocFailedHook(void)
{
  app_fault_halt("malloc failed");
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
  (void)xTask;
  app_fault_halt(pcTaskName != NULL ? pcTaskName : "stack overflow");
}
