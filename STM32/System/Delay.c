#include "stm32f10x.h"
#include "FreeRTOS.h"
#include "task.h"

/**
  * @brief  微秒级延时（非阻塞，使用空循环）
  * @param  xus 延时时长，范围：0~233015
  * @retval 无
  * @note   此函数不使用SysTick，避免与FreeRTOS冲突
  */
void Delay_us(uint32_t xus)
{
	uint32_t count;
	while (xus--)
	{
		// 72MHz时钟，1us约72个周期
		// 考虑循环开销，实际测试调整
		count = 8;
		while (count--);
	}
}

/**
  * @brief  毫秒级延时
  * @param  xms 延时时长，范围：0~4294967295
  * @retval 无
  * @note   如果FreeRTOS已启动，使用vTaskDelay；否则使用空循环延时
  */
void Delay_ms(uint32_t xms)
{
	// 检查FreeRTOS调度器是否已启动
	if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)
	{
		// FreeRTOS已启动，使用vTaskDelay（会自动让出CPU）
		vTaskDelay(pdMS_TO_TICKS(xms));
	}
	else
	{
		// FreeRTOS未启动，使用us级延时循环
		while (xms--)
		{
			Delay_us(1000);
		}
	}
}

/**
  * @brief  秒级延时
  * @param  xs 延时时长，范围：0~4294967295
  * @retval 无
  */
void Delay_s(uint32_t xs)
{
	while(xs--)
	{
		Delay_ms(1000);
	}
}
