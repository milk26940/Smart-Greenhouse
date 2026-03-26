# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 项目概述

智慧农业大棚系统是一个物联网项目，包含三个核心组件：

| 组件 | 技术栈 | 说明 |
|------|--------|------|
| **发送端** | STM32F103ZET6, FreeRTOS, HAL | 传感器采集 + LoRa 上行 |
| **MX6U 网关** | i.MX6ULL, OpenCV, MQTT | LoRa 接收、协议解析、边缘决策、视觉上云 |
| **前端** | Vue 3, TypeScript, Pinia, MQTT.js | Web 可视化界面 |

## 常用命令

### 前端 (Vue 3)
```bash
cd 网页/vue-project
npm install              # 安装依赖
npm run dev             # 开发模式热重载
npm run build           # 生产构建
npm run test:unit       # Vitest 单元测试
npm run test:e2e        # Playwright E2E 测试
npm run lint            # ESLint 检查
```

### MX6U 交叉编译 (在虚拟机 192.168.137.3 中执行)
```bash
cd /home/lan/linux/nfs/smart-greenhouse/mx6u
make CROSS_COMPILE=arm-poky-linux-gnueabi- SYSROOT=/path/to/sysroot
```

Windows 本机验证 (仅语法检查):
```bash
mingw32-make -C MX6U CROSS_COMPILE=
```

### STM32
- **IDE**: Keil MDK-ARM v5.32
- **工程文件**: `发送端/MDK-ARM/HAL_Key.uvprojx`
- **CubeMX 配置**: `发送端/HAL_Key.ioc`

## 系统架构

```
┌─────────────────────────────────────────────────────────────┐
│                     智慧农业大棚系统                          │
├─────────────────────────────────────────────────────────────┤
│  STM32 采集节点 (N=1,2,3)  ──LoRa──>  MX6U 网关            │
│  STM32 执行节点 (N=4,5)      9600 8N1   /dev/ttymxc2        │
│                                                      │
│                              ├── 状态缓存 (15秒离线判定)     │
│                              ├── 规则决策引擎               │
│                              ├── OpenCV (/dev/video2)      │
│                              └── MQTT ──> broker.emqx.io   │
│                                                            │
│                                              WebSocket ───> │
│                                                  Vue Web    │
└─────────────────────────────────────────────────────────────┘
```

## 节点角色配置

STM32 固件通过编译期宏切换角色 (位于 `发送端/Core/Inc/app_node_role.h`):

- `APP_NODE_ROLE_SENSOR` - 采集节点，发送 ENV 帧
- `APP_NODE_ROLE_EXECUTOR` - 执行节点，发送 STAT 帧
- `APP_NODE_ID` - 节点编号

## 通信协议

### LoRa 上行帧格式

**采集节点 ENV 帧:**
```
ENV,N=<node_id>,T=<temp>,H=<humidity>,SOIL=<soil>,L=<light>,CO2=<co2>
```

**执行节点 STAT 帧:**
```
STAT,N=<node_id>,F=<ON|OFF>,P=<ON|OFF>,LED=<ON|OFF>
```

### MQTT
- **Broker**: `broker.emqx.io:1883`
- **主题**: `smart-greenhouse/node1/state`
- **前端订阅**: `wss://broker.emqx.io:8084/mqtt`

## 网络配置

| 设备 | IP | 说明 |
|------|-----|------|
| PC 主机 | 192.168.137.1 | Windows |
| 虚拟机 | 192.168.137.3 | Linux, NFS 服务器 |
| MX6U 开发板 | 192.168.137.106 | eth1 |

**NFS 挂载:**
- 虚拟机导出: `/home/lan/linux/nfs`
- 开发板挂载: `/mnt/nfs`
- 挂载命令: `mount -t nfs -o vers=3,nolock,proto=tcp 192.168.137.3:/home/lan/linux/nfs /mnt/nfs`

**SSH 连接开发板:**
```bash
ssh -o HostKeyAlgorithms=+ssh-rsa -o PubkeyAcceptedAlgorithms=+ssh-rsa root@192.168.137.106
```

## 关键文件位置

| 文件 | 位置 | 用途 |
|------|------|------|
| `AGENTS.md` | 根目录 | AI 代理工作规范 (必读) |
| `发送端/HAL_Key.uvprojx` | 发送端/MDK-ARM/ | Keil 工程文件 |
| `发送端/HAL_Key.ioc` | 发送端/ | STM32CubeMX 配置 |
| `app_node_role.h` | 发送端/Core/Inc/ | STM32 节点角色配置 |
| `MX6U/Makefile` | MX6U/ | MX6U 构建入口 |
| `MX6U/src/` | MX6U/ | 网关源码 (协议解析、决策、MQTT) |
| `网页/vue-project/` | 网页/ | Vue 前端项目 |

## 开发约定

- 修改前先阅读 `AGENTS.md` 和 `文档/` 目录下相关文档
- MX6U 侧代码统一在虚拟机 NFS 目录 `/home/lan/linux/nfs` 下开发和交叉编译
- 开发板只通过 `/mnt/nfs` 访问构建产物
- 涉及 STM32/Keil/CubeMX 时，新增源文件需同步检查 Keil 工程和 CubeMX 元数据
- 声称"已完成""已修复"前需执行验证步骤
