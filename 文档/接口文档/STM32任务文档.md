# STM32接口任务说明

## 已实现内容

- 新增 LoRa ASCII 协议
  - 上行：环境数据 + 电机状态
  - 下行：电机启停 + 速度控制
- 新增传感器采集模块
  - DHT11 温湿度
  - 光敏 ADC 采样
  - CO2 串口接收
- 新增电机控制模块
  - PWM 调速
  - GPIO 启停
- 保留原有 LoRa 初始化和 `USART1/USART2` 转发调试逻辑

## 代码模块对应

- `发送端/Core/Src/lora_protocol.c`
  - 协议组包与命令解析
- `发送端/Core/Src/sensor_service.c`
  - 传感器采集与 CO2 串口缓存
- `发送端/Core/Src/motor_service.c`
  - 电机 PWM 与 GPIO 控制
- `发送端/Core/Src/Serial.c`
  - LoRa 初始化、透传收发、周期上报调度

## 联调建议

- 先只接 `USART1 + USART2 + LoRa`，观察 `TX:` 和 `RX:` 调试输出
- 再单独验证 CO2 串口能否收到纯数字文本
- 最后接电机驱动，先测试 `CTRL,M=ON,S=30` 再逐步升速
