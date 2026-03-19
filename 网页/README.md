# 智慧农业大棚网页

当前网页目录采用前后端分离：

- 前端：`网页/vue-project`
- 后端：`网页/demo`
- 数据库：`MySQL`

## 后端

后端使用 `Spring Boot + JPA`，默认端口 `8080`。

默认数据库环境变量：

```powershell
$env:GREENHOUSE_DB_URL='jdbc:mysql://localhost:3306/greenhouse?createDatabaseIfNotExist=true&useSSL=false&serverTimezone=Asia/Shanghai&allowPublicKeyRetrieval=true&characterEncoding=UTF-8'
$env:GREENHOUSE_DB_USERNAME='root'
$env:GREENHOUSE_DB_PASSWORD='123456'
```

启动：

```powershell
cd D:\STM32Project\智慧农业大棚\网页\demo
.\mvnw.cmd spring-boot:run
```

接口：

- `GET /api/health`
- `GET /api/realtime`
- `GET /api/history?hours=24`
- `GET /api/stats?hours=24`
- `GET /api/presence`
- `POST /api/telemetry`

说明：

- 应用首次启动且表为空时，会自动写入一批演示数据，便于前端直接查看历史查询和统计效果。
- 测试环境使用 `H2`，正式运行按 `MySQL` 配置连接。

## 前端

前端使用 `Vue 3 + TypeScript + Vite`，开发端口默认 `5173`，已代理 `/api` 到后端 `8080`。

启动：

```powershell
cd D:\STM32Project\智慧农业大棚\网页\vue-project
npm run dev
```

构建：

```powershell
npm run build
```

单元测试：

```powershell
npm run test:unit -- --run
```

### MQTT 面板

当前首页已增加浏览器直连 MQTT Broker 的独立面板，用于快速判断连接状态、订阅结果和原始消息日志。

默认配置：

- Broker：`wss://test.mosquitto.org:8081/mqtt`
- Topic：`lora/temperature`

说明：

- 浏览器不能直接使用 `mqtt://host:1883`，页面里实际使用的是 `ws://` 或 `wss://` WebSocket 地址。
- 面板支持编辑连接参数、手动连接/断开、清空日志，并实时显示连接日志和最近收到的一条消息。
