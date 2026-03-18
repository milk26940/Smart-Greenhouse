# LoRa 通信协议

## 1. 说明

本文档定义 STM32 节点与 MX6U 之间的 LoRa 透传字符串协议。

当前主范围只覆盖 `上行`：

- 采集节点上报环境数据
- 执行节点上报设备状态

下行控制本轮仅作为兼容项保留。

基本约定：

- 编码方式：ASCII 文本
- 默认上报周期：5 秒
- 帧结束：建议使用 `\r\n`、`\r` 或 `\n`
- 所有上行帧必须带节点编号 `N=<node_id>`

## 2. 节点角色

同一 STM32 工程通过编译期宏切换角色：

- `APP_NODE_ROLE_SENSOR`：采集节点
- `APP_NODE_ROLE_EXECUTOR`：执行节点
- `APP_NODE_ID`：节点编号

配置文件： [app_node_role.h](d:\STM32Project\智慧农业大棚\发送端\Core\Inc\app_node_role.h)

## 3. 上行协议

### 3.1 采集节点环境帧

格式：

```text
ENV,N=<node_id>,T=<temperature>,H=<humidity>,SOIL=<soil_moisture>,L=<light_raw>,CO2=<co2_ppm>
```

字段说明：

- `ENV`：环境数据帧标识
- `N`：采集节点编号
- `T`：温度，单位摄氏度，保留 1 位小数
- `H`：湿度，单位 `%RH`，保留 1 位小数
- `SOIL`：土壤湿度，单位 `%`，保留 1 位小数
- `L`：光照 ADC 原始值，范围 `0~4095`
- `CO2`：二氧化碳浓度，单位 `ppm`

示例：

```text
ENV,N=1,T=25.3,H=60.1,SOIL=41.2,L=1234,CO2=567
```

### 3.2 执行节点状态帧

格式：

```text
STAT,N=<node_id>,F=<ON|OFF>,P=<ON|OFF>,LED=<ON|OFF>
```

字段说明：

- `STAT`：状态帧标识
- `N`：执行节点编号
- `F`：风扇状态
- `P`：水泵状态
- `LED`：补光灯状态

示例：

```text
STAT,N=2,F=ON,P=OFF,LED=ON
```

## 4. STM32 固件实现对应关系

### 4.1 采集节点

环境帧由 [lora_protocol.c](d:\STM32Project\智慧农业大棚\发送端\Core\Src\lora_protocol.c) 中的环境构帧接口生成，字段来源如下：

- `T/H`：DHT11
- `L`：ADC 光照通道
- `SOIL`：土壤湿度字段，等待独立土壤湿度传感器接入
- `CO2`：`USART3` 二氧化碳串口模块

### 4.2 执行节点

状态帧由状态构帧接口生成，当前执行器映射如下：

- `F`：`MOTOR_*` 风扇输出
- `P`：`LED1`
- `LED`：`LED2`

`LED3` 不纳入协议字段。

## 5. 下行兼容说明

当前代码仍兼容旧版电机控制字符串，供现有联调用：

```text
CTRL,M=<ON|OFF>,S=<speed_percent>
```

说明：

- 该兼容命令只影响执行节点中的风扇输出
- 水泵和补光灯状态不通过旧命令控制
- 非 `CTRL,` 开头的字符串会被忽略
- `ENV` 和 `STAT` 上行帧不会被误判为下行控制命令

示例：

```text
CTRL,M=ON,S=80
CTRL,M=OFF,S=0
```

## 6. 调试与透传约束

- `USART2`：LoRa 串口
- `USART1`：调试镜像串口
- LoRa 上电初始化命令保持不变：

```text
+++
AT+MODE0
AT+LEVEL2
AT+CHANNEL01
AT+RESET
```

- 调试串口会输出 `TX:`、`RX:`、`CMD:OK`、`CMD:ERR` 便于联调

## 7. 当前实现备注

- 采集节点构建后只发送 `ENV`
- 执行节点构建后只发送 `STAT`
- 上报周期默认 5 秒
- 当前 `T/H` 来自 DHT11 单总线
- 当前 `CO2` 来自 `USART3` 串口模块
- 当前 `SOIL` 字段保留，默认沿用最近值，待土壤湿度传感器接入
