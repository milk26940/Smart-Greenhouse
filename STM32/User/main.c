#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "OLED.h"
#include "Serial.h"
#include "LED.h"
#include "string.h"
#include "freertos_demo.h"

int main(void)
{
	/* 启动FreeRTOS调度器 */
	freertos_start();
}
