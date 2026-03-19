# MX6U 最小工程骨架

本目录提供 MX6U 侧的最小工程模板，用于后续在虚拟机中继续补充 LoRa 接收、协议解析和本地运行逻辑。

当前阶段目标：

- 提供统一的 `Makefile` 构建入口
- 提供最小 LoRa 上行帧解析器
- 在开发板侧通过 `/dev/ttymxc2` 进入串口收包循环
- 提供 3 个采集节点和 2 个执行节点的状态缓存与离线判定
- 提供本地规则判断、趋势分析和异常检测的最小决策引擎
- 明确虚拟机 NFS 构建与开发板 NFS 运行流程
- 提供首版 MQTT 上云发布链路，后续网页可直接订阅同一主题

## 目录说明

- `Makefile`：唯一构建入口，预留交叉编译变量
- `include/`：公共头文件目录
- `src/`：协议解析、分帧和串口读取源码
- `src/mx6u_state.c`：固定节点状态缓存、在线状态刷新和汇总输出
- `src/mx6u_decision.c`：规则判断、趋势分析、异常检测和推荐动作输出
- `src/mx6u_vision.cpp`：OpenCV 轻量视觉分析和 `VISION SNAPSHOT` 输出
- `src/mx6u_mqtt.c`：最小 MQTT 3.1.1 发布客户端和 `MQTT SNAPSHOT` 输出
- `build/`：默认构建输出目录，运行 `make` 后生成

## 仓库目录与实际工作目录

- 当前仓库内 `MX6U/` 保存源码模板和配置
- 虚拟机实际工作目录统一为 `/home/lan/linux/nfs/smart-greenhouse/mx6u`
- 开发板通过 `/mnt/nfs/smart-greenhouse/mx6u` 直接访问构建产物

建议做法是将本目录内容同步到虚拟机 NFS 工作目录后，再在虚拟机中加载交叉编译环境执行构建。

## 构建方式

### 1. 虚拟机交叉编译

先在虚拟机中加载工具链环境，再进入 `/home/lan/linux/nfs/smart-greenhouse/mx6u` 执行：

```bash
make CROSS_COMPILE=arm-poky-linux-gnueabi- SYSROOT=/path/to/sysroot
```

如果工具链环境已经导出了编译器和 sysroot，也可以直接执行：

```bash
make
```

### 2. 本机结构验证

当前仓库提供的最小程序可以用主机 GCC 做语法级验证，例如在 Windows 下执行：

```powershell
mingw32-make -C MX6U CROSS_COMPILE=
```

这一步只用于验证工程骨架和 `main.c` 的可编译性，不替代虚拟机中的正式交叉编译。

## 运行方式

构建完成后，产物默认位于 `build/mx6u_skeleton`。

在开发板侧通过 NFS 挂载目录运行：

```bash
/mnt/nfs/smart-greenhouse/mx6u/build/mx6u_skeleton
```

当前程序默认行为：

- 打开 `/dev/ttymxc2`
- 按 `9600 8N1 raw` 方式读取 LoRa 串口文本
- 按 `\r` / `\n` 分帧
- 调用现有 `ENV/STAT` 解析器并打印结构化结果
- 维护固定节点集合：采集节点 `1/2/3`，执行节点 `4/5`
- 每收到一帧后打印一次当前状态汇总快照
- 按 `15 秒` 超时规则将已上报节点标记为 `OFFLINE`
- 基于 `ENV + STAT + 在线状态` 输出本地规则判断和推荐动作
- 使用最近 3 次 `ENV node=1` 样本做温度/光照趋势判断与提前干预提示
- 每 `10` 秒尝试从 `/dev/video2` 抓拍一帧并输出 `VISION SNAPSHOT`
- 每次收到有效 `ENV/STAT` 后尝试发布一份 MQTT 整包 JSON 到 `broker.emqx.io:1883`
- 默认主题：`smart-greenhouse/node1/state`
- MQTT 发布节流为每 `10` 秒最多一次，其余帧会在 `MQTT SNAPSHOT` 中显示 `reason=throttled`
- 第一版只做本地决策闭环，不直接向 STM32 下发控制命令

当前视觉模块说明：

- 默认摄像头节点：`/dev/video2`
- 默认分析分辨率：`320x240`
- 不保存原图，只保留上一帧灰度图用于运动检测
- 若摄像头打开或抓拍失败，程序会降级为 `available=NO`，不影响串口接收和决策主流程

当前 MQTT 发布说明：
- 设备端 Broker：`broker.emqx.io:1883`
- 主题：`smart-greenhouse/node1/state`
- 首版消息体为单主题整包 JSON，包含 `env`、`decision`、`vision`、`presence`
- 后续网页端应使用 `wss://broker.emqx.io:8084/mqtt` 订阅同一主题
- 本地离线验证可临时设置 `MX6U_MQTT_ENABLE=0`，此时程序会继续运行，但 `MQTT SNAPSHOT` 显示 `reason=disabled`

如果需要在本机或无串口环境下验证，可临时使用测试回退模式：

```powershell
set MX6U_SERIAL_DEVICE=stdin
echo ENV,N=1,T=25.3,H=60.1,SOIL=41.2,L=1234,CO2=567 | .\build\mx6u_skeleton.exe
```

开发板串口当前已知配置参考 `注意事项.md` 中的 `stty -F /dev/ttymxc2 raw -echo 9600 cs8 -cstopb -parenb`。
