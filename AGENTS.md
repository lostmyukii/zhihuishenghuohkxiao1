# AGENTS.md

本仓库是 `N16R8 智慧低碳学习小屋`，面向小学组智慧生活赛项。任何 agent 继续工作前必须先读本文件、`开发文档.md`、`设计方案.md`。

## 项目原则

- 作品主线是小学组四项任务：`数据采集 -> 智能控制 -> 语音交互 -> 节能响应`。
- 每个网页状态、语音命令、图片标注、硬件动作都必须对应真实 N16R8 行为。
- Dashboard 第一屏是操作台，不做落地页；不能用静态 demo 数据冒充开发板在线。
- 串口协议是 `115200` 单行 JSON：`hello / telemetry / command / ack`。
- MQTT 和可视化沿用初中能力，但在小学项目中是展示增强项，不盖过小学任务映射。

## 当前权威资料

| 文件 | 用途 |
| --- | --- |
| `开发文档.md` | 开发合同、硬件连接、串口协议、MQTT 路线、实施顺序。 |
| `设计方案.md` | 规则审计与作品定位。 |
| `assets/wiring/n16r8-primary-wiring.png` | image 模型生成的硬件连接示意图。 |
| `assets/wiring/prompts/n16r8-primary-wiring.md` | 连线示意图提示词。 |
| `infographic/smartlife-primary-house/infographic.png` | 小学组任务映射信息图。 |

如果图片局部小字和 `开发文档.md` 表格不一致，以 `开发文档.md` 和实板丝印复核为准。

## 硬件合同

当前小学 N16R8 合同如下，除非用户明确改硬件，不要改：

| 模块 | GPIO/接口 | 角色 |
| --- | --- | --- |
| 光敏传感器 | `ADC1 / GPIO1` | 光照采集、光暗开灯、节能判断 |
| DHT11 温湿度 | `D14 / GPIO14` | 温度阈值、舒适判断 |
| 声音传感器 | `ADC4 / GPIO4` | 学习噪声提醒 |
| PIR 人体红外 | `D5 / GPIO5` | 有人/无人、离家提醒、节能响应 |
| 8键AD | `ADC3 / GPIO3` | 模式切换、阈值焦点 |
| 学习灯/继电器 | `GPIO48` | 低压灯或指示灯 |
| 风扇 PWM | `D11 / GPIO11` | 通风/降温 |
| 舵机窗帘 | `D9 / GPIO9` | 窗帘模型 |
| RGB 灯环 | `GPIO47` | 模式状态灯 |
| 无源蜂鸣器 | `D13 / GPIO13` | 短提醒 |
| OLED | `SDA=GPIO41, SCL=GPIO42` | 本地数据显示 |

可选扩展：`MQ-2 ADC2/GPIO2`、`水滴 D8/GPIO8`、`火焰 D6/GPIO6`。MQ-2 的 `AO` 必须限压到 `0~3.3V`。

不要盲目照搬初中项目中的 `GPIO46 / GPIO12 / GPIO45` 映射。

## 安全红线

- 不接 `220V`。
- 所有模块共地。
- ESP32-S3 输入信号不超过 `3.3V`。
- 上电默认：风扇关、继电器关、蜂鸣器静音、RGB 安全色。
- Web 或语音的自由文本不能直接控制危险动作，必须经过白名单意图。
- `actuator.buzzer=false` 只代表停止手动蜂鸣测试，不代表关闭自动安全提醒。

## 通信路线

推荐路线：

```text
N16R8 USB -> HTTPS Dashboard Web Serial -> WSS Cloud Relay -> MQTT Broker -> 其他电脑/手机 Dashboard
```

约束：

- 插 USB 的电脑必须用 Chrome/Edge 打开 HTTPS Dashboard 并保持页面打开。
- Safari/iPhone 可观看 Dashboard，但不作为 USB 直连网关。
- 若改 JS，要同步更新 `dashboard/index.html` 的脚本版本和 `dashboard/sw.js` 的缓存名。

小学组 MQTT topic 前缀：

```text
smartlife/primary/n16r8
```

## 串口协议要求

- 波特率：`115200`。
- 每行一个 JSON。
- 固件启动后输出 `hello`。
- 固件持续输出 `telemetry`。
- Dashboard/网关下发 `command` 或 `voiceIntent`。
- 固件必须返回 `ack`。
- 不要把中文调试日志混进正式 JSON 行。

## 8键AD 约定

| 键 | 功能 |
| --- | --- |
| A | 学习模式 |
| B | 休息模式 |
| C | 离家模式 |
| D | 节能模式 |
| 左/右 | 切换阈值焦点 |
| 上/下 | 调整当前阈值 |

默认阈值焦点建议为 `lightThreshold`。OLED 和 Dashboard 都要显示当前焦点。

## 开发顺序

1. 固件：GPIO、上电安全、传感读取、模式规则、`hello/telemetry/ack`。
2. Dashboard：Web Serial、真实在线状态、传感器/执行器卡片、模式按钮、阈值面板。
3. Gateway/Relay：WSS/MQTT，topic 前缀可配置。
4. 语音：先文本/快捷按钮，再接 HTTPS STT。
5. 模型：按 `assets/wiring/n16r8-primary-wiring.png` 和 `开发文档.md` 布线。

## 验证命令

实现对应文件后，至少运行：

```bash
node --check dashboard/app.js
node --check dashboard/sw.js
python3 -m py_compile tools/n16r8_gateway.py
python3 -m py_compile tools/voice_transcribe_server.py
/Users/yukii/.platformio/penv/bin/pio run -d firmware
```

如果相关文件尚未创建，不要声称这些检查通过。文档阶段只检查 Markdown、图片路径和关键表格。

## 文档与图片规则

- 连线图用 image 模型生成的位图保存在 `assets/wiring/`。
- 不用 SVG/HTML 假装成 image 模型生成图。
- 不要用程序去覆盖或修补生成图里的文字；如果图中文字严重错误，应保留旧图并重新生成。
- 文档中要明确“图片用于总览，精确端口以表格为准”。

## 工作习惯

- 先读现有文档，再改文件。
- 不改无关文件。
- 不写入 API key、MQTT 密码、Wi-Fi 密码到仓库文档。
- 不把 mock 数据伪装成真实硬件数据。
- 如果实板丝印和本文档冲突，先记录差异，再更新 `开发文档.md` 和本文件。
