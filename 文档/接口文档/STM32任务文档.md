# STM32接口任务说明

## 已实现内容

- 新增 LoRa ASCII 协议
  - 上行：采集节点环境数据 `ENV` + 执行节点状态 `STAT`
  - 下行：兼容旧版电机启停 + 速度控制
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
  - `ENV/STAT` 协议组包与兼容命令解析
- `发送端/Core/Src/sensor_service.c`
  - DHT11、光照 ADC 与 CO2 串口缓存
- `发送端/Core/Src/motor_service.c`
  - 风扇 PWM 与 GPIO 控制，水泵/补光灯状态映射
- `发送端/Core/Src/Serial.c`
  - LoRa 初始化、透传收发、按角色周期上报调度
- `发送端/Core/Inc/app_node_role.h`
  - 采集节点 / 执行节点编译期角色切换

## 联调建议

- 先只接 `USART1 + USART2 + LoRa`，观察 `TX:` 和 `RX:` 调试输出
- 再验证 `USART3` 的 CO2 数据是否能稳定更新 `CO2=<value>`
- 最后接电机驱动，先测试 `CTRL,M=ON,S=30` 再逐步升速
