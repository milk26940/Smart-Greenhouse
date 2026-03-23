# STM32 Sleep Tickless Idle Implementation Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 为 `发送端` STM32F103 + FreeRTOS 工程开启基础低功耗 `Sleep + Tickless Idle`。

**Architecture:** 保持现有 LoRa、串口、传感器和执行器任务模型不变，只在 FreeRTOS 配置层开启 tickless idle，并在应用层实现最小睡眠前后钩子。低功耗仅进入 Cortex-M3 Sleep，不主动关闭 USART/ADC/TIM 等外设时钟，确保现有中断唤醒路径保持兼容。

**Tech Stack:** STM32F103, FreeRTOS, STM32 HAL, Keil/CubeMX 工程

---

### Task 1: 开启 FreeRTOS Tickless 配置

**Files:**
- Modify: `发送端/Core/Inc/FreeRTOSConfig.h`

- [ ] **Step 1: 添加 tickless idle 配置宏**

加入：

```c
#define configUSE_TICKLESS_IDLE 1
#define configEXPECTED_IDLE_TIME_BEFORE_SLEEP 2
```

- [ ] **Step 2: 声明睡眠前后处理宏**

加入：

```c
void PRE_SLEEP_PROCESSING(uint32_t ulExpectedIdleTime);
void POST_SLEEP_PROCESSING(uint32_t ulExpectedIdleTime);

#define configPRE_SLEEP_PROCESSING(x) PRE_SLEEP_PROCESSING(x)
#define configPOST_SLEEP_PROCESSING(x) POST_SLEEP_PROCESSING(x)
```

### Task 2: 实现最小睡眠钩子

**Files:**
- Modify: `发送端/Core/Src/freertos_demo.c`

- [ ] **Step 1: 增加睡眠钩子函数**

实现：

```c
void PRE_SLEEP_PROCESSING(uint32_t ulExpectedIdleTime)
{
  (void)ulExpectedIdleTime;
  __DSB();
  __ISB();
}

void POST_SLEEP_PROCESSING(uint32_t ulExpectedIdleTime)
{
  (void)ulExpectedIdleTime;
  __DSB();
  __ISB();
}
```

- [ ] **Step 2: 保持当前任务逻辑不变**

不修改：
- `sensor_task`
- `motor_task`
- `lora_task`

### Task 3: 验证

**Files:**
- Modify: `发送端/Core/Inc/FreeRTOSConfig.h`
- Modify: `发送端/Core/Src/freertos_demo.c`

- [ ] **Step 1: 检查代码引用完整**

确认 `FreeRTOSConfig.h` 中宏和 `freertos_demo.c` 中函数签名一致。

- [ ] **Step 2: 尝试构建工程**

优先使用现有 Keil 工程：

`发送端/MDK-ARM/HAL_Key.uvprojx`

- [ ] **Step 3: 记录残余风险**

明确说明：
- 当前只实现 Sleep，不是 STOP
- 低功耗效果依赖系统空闲窗口
- 未做板级电流实测时，不能宣称节电幅度
