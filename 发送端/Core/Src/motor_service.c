#include "motor_service.h"

#include "main.h"
#include "tim.h"

static MotorStatus motor_status = {0U, 0U};

static void motor_apply_output(void)
{
  uint32_t pulse = (uint32_t)motor_status.speed_percent;

  HAL_GPIO_WritePin(MOTOR_EN_GPIO_Port,
                    MOTOR_EN_Pin,
                    motor_status.enabled ? GPIO_PIN_SET : GPIO_PIN_RESET);
  HAL_GPIO_WritePin(MOTOR_DIR_GPIO_Port,
                    MOTOR_DIR_Pin,
                    motor_status.enabled ? GPIO_PIN_SET : GPIO_PIN_RESET);
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

  motor_status.enabled = command->enabled ? 1U : 0U;
  motor_status.speed_percent = (command->speed_percent > 100U) ? 100U : command->speed_percent;

  if (motor_status.enabled == 0U)
  {
    motor_status.speed_percent = 0U;
  }

  motor_apply_output();
}

MotorStatus MotorService_GetStatus(void)
{
  return motor_status;
}
