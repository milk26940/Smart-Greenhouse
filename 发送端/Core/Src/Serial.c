/**
 * @file Serial.c
 * @brief 发送端串口通信与 LoRa 业务逻辑实现 (HAL库版)
 * @details
 *   - 负责初始化 USART1 (调试输出) 和 USART2 (LoRa 通信)
 *   - 实现 LoRa 模组的 AT 指令配置流程
 *   - 实现每秒递增发送测试数据 (0001, 0002...)
 *   - 实现 USART2 接收数据的实时转发 (USART2 RX -> USART1 TX)
 */

#include "Serial.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "usart.h"

/* 全局变量定义 */
char Serial_RxPacket[1] = {0};
uint8_t Serial_RxFlag = 0;
uint8_t Serial_RxData = 0;
uint16_t Serial_RxIndex = 0;
void *Serial_RxSemaphore = NULL;

/* 内部状态变量 */
static uint8_t lora_rx_byte = 0;     /* USART2 单字节接收缓冲 */
static uint8_t startup_done = 0;     /* 标记是否已完成上电配置 */
static uint16_t temperature_step = 0;/* 温度模拟步进 */
static uint32_t next_tx_tick = 0;    /* 下一次发送的时间戳 */
static uint8_t rx_debug_buffer[128]; /* 接收数据调试缓冲 */
static uint16_t rx_debug_length = 0; /* 接收数据长度 */
static uint32_t rx_last_tick = 0;    /* 最后一次接收的时间戳 */

/**
 * @brief 通过 USART1 发送调试数据
 * @param data 数据指针
 * @param length 数据长度
 * @note 此函数用于将任何数据“镜像”发送到 PC 调试窗口
 */
static void debug_send_bytes(const uint8_t *data, uint16_t length)
{
  HAL_UART_Transmit(&huart1, (uint8_t *)data, length, HAL_MAX_DELAY);
}

/**
 * @brief 发送带标签的调试帧
 * @param tag 标签字符串 (如 "TX:", "RX:")
 * @param data 数据内容
 * @param length 数据长度
 * @param append_crlf 是否追加回车换行
 * @param force_crlf 是否强制换行 (即使 append_crlf 为 0)
 */
static void debug_send_tagged_frame(const char *tag,
                                    const uint8_t *data,
                                    uint16_t length,
                                    uint8_t append_crlf,
                                    uint8_t force_crlf)
{
  static const uint8_t crlf[] = "\r\n";

  /* 1. 发送标签 */
  debug_send_bytes((const uint8_t *)tag, (uint16_t)strlen(tag));

  /* 2. 发送数据内容 */
  if (length > 0U)
  {
    debug_send_bytes(data, length);
  }

  /* 3. 发送换行符 */
  if (append_crlf || force_crlf)
  {
    debug_send_bytes(crlf, 2);
  }
}

/**
 * @brief 刷新接收调试缓冲区
 * @param force_crlf 是否强制换行
 * @note 将缓冲区中的接收数据带上 "RX:" 标签发送到调试口
 */
static void debug_flush_rx_buffer(uint8_t force_crlf)
{
  if (rx_debug_length == 0U)
  {
    return;
  }

  debug_send_tagged_frame("RX:", rx_debug_buffer, rx_debug_length, 0, force_crlf);
  rx_debug_length = 0;
}

/**
 * @brief 通过 USART2 向 LoRa 模组发送数据，并同步转发到 USART1
 * @param data 数据指针
 * @param length 数据长度
 * @param append_crlf 是否追加回车换行符 (1: 是, 0: 否)
 */
static void lora_send_bytes(const uint8_t *data, uint16_t length, uint8_t append_crlf)
{
  /* 1. 先把之前接收到的数据刷出去，避免显示混淆 */
  debug_flush_rx_buffer(1);

  /* 2. 镜像发送 TX 数据到 USART1 */
  debug_send_tagged_frame("TX:", data, length, append_crlf, append_crlf ? 0 : 1);

  /* 3. 实际发送到 USART2 (LoRa) */
  HAL_UART_Transmit(&huart2, (uint8_t *)data, length, HAL_MAX_DELAY);

  /* 4. 如果需要，发送回车换行到 USART2 */
  if (append_crlf)
  {
    static const uint8_t crlf[] = "\r\n";
    HAL_UART_Transmit(&huart2, (uint8_t *)crlf, 2, HAL_MAX_DELAY);
  }
}

/**
 * @brief 检查 LoRa 模组 AUX 引脚状态
 * @return 1: 空闲 (低电平), 0: 忙碌 (高电平)
 */
static uint8_t lora_aux_is_idle(void)
{
  return HAL_GPIO_ReadPin(LORA_AUX_GPIO_Port, LORA_AUX_Pin) == GPIO_PIN_RESET;
}

/**
 * @brief 等待 LoRa 模组空闲
 * @param timeout_ms 超时时间 (毫秒)
 */
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

/**
 * @brief 发送 LoRa AT 指令
 * @param cmd 指令字符串
 * @param append_crlf 是否追加回车换行
 * @note 发送前后会检查 AUX 状态并加入延时
 */
static void lora_send_command(const char *cmd, uint8_t append_crlf)
{
  lora_wait_idle(500); /* 发送前等待空闲 */
  lora_send_bytes((const uint8_t *)cmd, (uint16_t)strlen(cmd), append_crlf);
  HAL_Delay(100);      /* 指令间隔延时 */
  lora_wait_idle(500); /* 发送后再次确认空闲 */
}

/**
 * @brief 执行 LoRa 模组的上电初始化配置流程
 * @details
 *   1. +++        : 进入配置模式
 *   2. AT+MODE0   : 设置为透传模式
 *   3. AT+LEVEL2  : 设置速率等级
 *   4. AT+CHANNEL01: 设置信道
 *   5. AT+RESET   : 重启生效
 */
static void lora_run_startup_config(void)
{
  lora_send_command("+++", 0);
  lora_send_command("AT+MODE0", 1);
  lora_send_command("AT+LEVEL2", 1);
  lora_send_command("AT+CHANNEL01", 1);
  lora_send_command("AT+RESET", 1);
  HAL_Delay(500);
  lora_wait_idle(1000);
}

static void build_temperature_payload(char *payload, uint16_t payload_size)
{
  uint16_t temp_tenths = (uint16_t)(235U + ((temperature_step % 10U) * 3U));

  snprintf(payload, payload_size, "T=%u.%u", temp_tenths / 10U, temp_tenths % 10U);
}

/**
 * @brief 串口初始化
 * @note 开启 USART2 的接收中断
 */
void Serial_Init(void)
{
  HAL_UART_Receive_IT(&huart2, &lora_rx_byte, 1);
}

/**
 * @brief 串口及业务主处理函数
 * @note 需要在主循环中周期调用
 * @details
 *   1. 检查接收缓冲区超时并刷新
 *   2. 上电首次调用时执行 AT 配置
 *   3. 之后每 5 秒发送一次模拟温度数据
 */
void Serial_Process(void)
{
  char payload[12];

  /* 1. 检查接收超时 (20ms 无新数据则刷新缓冲区) */
  if ((rx_debug_length > 0U) && ((HAL_GetTick() - rx_last_tick) >= 20U))
  {
    debug_flush_rx_buffer(1);
  }

  /* 2. 处理上电初始化 */
  if (!startup_done)
  {
    lora_run_startup_config();
    startup_done = 1;
    next_tx_tick = HAL_GetTick() + 5000U;
    return;
  }

  /* 3. 检查发送时间间隔 (5000ms) */
  if ((int32_t)(HAL_GetTick() - next_tx_tick) < 0)
  {
    return;
  }

  /* 4. 检查模组是否忙碌 */
  if (!lora_aux_is_idle())
  {
    return; /* 如果忙，本轮跳过，等待下一轮 */
  }

  /* 5. 构造并发送温度模拟数据 */
  build_temperature_payload(payload, sizeof(payload));
  lora_send_bytes((const uint8_t *)payload, (uint16_t)strlen(payload), 0);

  /* 6. 发送后等待并更新状态 */
  lora_wait_idle(1000);
  temperature_step++;
  next_tx_tick = HAL_GetTick() + 5000U;
}

/**
 * @brief 发送单字节到 USART1
 */
void Serial_SendByte(uint8_t Byte)
{
  HAL_UART_Transmit(&huart1, &Byte, 1, HAL_MAX_DELAY);
}

/**
 * @brief 发送字符串
 */
void Serial_SendString(char *String)
{
  Serial_Printf("%s", String);
}

/**
 * @brief 自定义格式化打印
 */
void Serial_Printf(char *format, ...)
{
  char buffer[128];
  va_list args;

  va_start(args, format);
  vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);

  debug_send_bytes((uint8_t *)buffer, (uint16_t)strlen(buffer));
}

/**
 * @brief 获取接收数据包 (未实现)
 */
char *Serial_GetRxPacket(void)
{
  return NULL;
}

/**
 * @brief HAL 库串口接收完成回调
 * @note 处理 USART2 接收到的数据，并直接转发到 USART1
 * @details
 *   - 使用缓冲区累积接收数据
 *   - 遇到换行符或缓冲区满时刷新到调试口
 *   - 每次接收后记录时间戳用于超时检测
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART2)
  {
    /* 1. 缓冲区满，强制刷新 */
    if (rx_debug_length >= sizeof(rx_debug_buffer))
    {
      debug_flush_rx_buffer(1);
    }

    /* 2. 存入缓冲区 */
    rx_debug_buffer[rx_debug_length++] = lora_rx_byte;
    rx_last_tick = HAL_GetTick();

    /* 3. 遇到换行符，立即刷新 */
    if ((lora_rx_byte == '\n') || (lora_rx_byte == '\r'))
    {
      debug_flush_rx_buffer(0);
    }

    /* 4. 重新开启中断接收下一个字节 */
    HAL_UART_Receive_IT(&huart2, &lora_rx_byte, 1);
  }
}
