#include "motor_service.h"

#include "main.h"
#include "tim.h"

static ActuatorStatus actuator_status = {0U, 0U, 0U};
static uint8_t fan_speed_percent = 0U;

static void motor_apply_output(void)
{
  uint32_t pulse = (uint32_t)fan_speed_percent;
  GPIO_PinState in1_state = GPIO_PIN_RESET;
  GPIO_PinState in2_state = GPIO_PIN_RESET;

  if ((actuator_status.fan_on != 0U) && (pulse > 0U))
  {
    in1_state = GPIO_PIN_SET;
    in2_state = GPIO_PIN_RESET;
  }

  HAL_GPIO_WritePin(MOTOR_EN_GPIO_Port,
                    MOTOR_EN_Pin,
                    in1_state);
  HAL_GPIO_WritePin(MOTOR_DIR_GPIO_Port,
                    MOTOR_DIR_Pin,
                    in2_state);
  HAL_GPIO_WritePin(LED1_GPIO_Port,
                    LED1_Pin,
                    actuator_status.pump_on ? GPIO_PIN_RESET : GPIO_PIN_SET);
  HAL_GPIO_WritePin(LED2_GPIO_Port,
                    LED2_Pin,
                    actuator_status.fill_light_on ? GPIO_PIN_RESET : GPIO_PIN_SET);
  __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, pulse);
}

void MotorService_Init(void)
{
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3);
  motor_apply_output();
}

void MotorService_ApplyCommand(const LoraMotorCommand *command)
{
  if (command == NULL)
  {
    return;
  }

  actuator_status.fan_on = command->enabled ? 1U : 0U;
  fan_speed_percent = (command->speed_percent > 100U) ? 100U : command->speed_percent;

  if (actuator_status.fan_on == 0U)
  {
    fan_speed_percent = 0U;
  }

  motor_apply_output();
}

ActuatorStatus MotorService_GetStatus(void)
{
  return actuator_status;
}
