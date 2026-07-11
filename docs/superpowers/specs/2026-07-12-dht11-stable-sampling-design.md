# DHT11 稳定采样修复设计

日期：2026-07-12

状态：已实施并通过实板验收

## 1. 问题与目标

实板 DHT11 已确认连接到 `D14 / GPIO14`，模块端 `G/V/S` 与扩展板 `G/V/14(S)` 对应，但 Dashboard 温度和湿度持续显示“等待”。当前固件把 DHT11 与光敏、声音、PIR 一起每 `200ms` 读取；这个周期不符合 DHT11 的慢速采样特性，瞬时失败还会立即把 `dhtValid` 改为 `false`。

本次只修复 DHT11 采样稳定性，不改变 GPIO、模式规则、串口 JSON 字段或 Dashboard 展示合同。

## 2. 采用方案

- 光敏 `GPIO1`、声音 `GPIO4`、PIR `GPIO5` 继续每 `200ms` 读取，保证灯光和人体响应速度。
- DHT11 `GPIO14` 使用独立的 `2000ms` 采样周期。
- 第一次进入主循环时立即尝试读取 DHT11，不额外等待两秒。
- 成功读取后更新温度、湿度、`dhtValid=true` 和最近成功时间。
- 单次读取失败时保留最近一次有效温湿度，不让网页闪回“等待”。
- 如果启动后从未成功，或距离最近成功读取达到 `6000ms`，才设置 `dhtValid=false`，使 `health.dht` 显示 `missing`。

## 3. 固件边界

在 `firmware/src/main.cpp` 中拆分两个读取路径：

```text
每 200ms -> readFastSensors() -> 光敏、声音、PIR、可选安全输入
每 2000ms -> readDhtSensor() -> DHT11 温度、湿度、有效性和时间戳
```

新增常量：

```text
DHT_INTERVAL_MS = 2000
DHT_STALE_MS = 6000
```

新增状态：最近一次 DHT 尝试时间、最近一次 DHT 成功时间。自动控制继续只在 `dhtValid=true` 时判断温度提醒，因此失效数据不会触发误报警。

实际实施时使用固定版本 `adafruit/DHT sensor library@1.4.6` 替换手写脉冲解析，采样周期与失效窗口仍按本设计执行。固件版本为 `0.1.2`。

## 4. 协议与界面

串口字段保持不变：

- 有效时：`sensors.temperature`、`sensors.humidity` 为数值，`health.dht="ok"`。
- 启动未成功或连续失效约六秒：温湿度为 `null`，`health.dht="missing"`。
- 短暂失败未超过六秒：继续上报最近有效数值，避免 Dashboard 抖动。

Dashboard 不需要修改。恢复有效数据后，温度、湿度卡片应自动显示数值；在 `study` 模式把温度阈值临时调低时，现有 `temperature` 橙色提醒继续工作。

## 5. 验证

自动检查：

1. 固件合同测试确认快速传感器周期仍为 `200ms`。
2. 固件合同测试确认 DHT 周期为 `2000ms`、失效窗口为 `6000ms`。
3. PlatformIO 编译通过。

实板检查：

1. 断开 Dashboard 的 Web Serial，烧录新固件。
2. 重新连接 Dashboard，等待最多约三秒，确认温度、湿度出现数值且 `health.dht` 为 `ok`。
3. 确认遮光和 PIR 响应速度没有变慢。
4. 在 `study` 模式把温度阈值临时调到当前温度以下，确认橙色温度提醒、当前值/阈值、蜂鸣器和 OLED 同步。
5. 把温度阈值恢复为默认 `29 C`。

## 6. 安全与回退

- 不改变 `D12` 学习灯、`D13` 蜂鸣器及其他传感器映射。
- 不带电插拔 DHT11。
- 烧录前必须先在网页点击“断开”，释放 `/dev/cu.usbserial-120`。
- 如果新固件仍始终显示 `missing`，再进入模块供电、模块型号和信号波形排查，不继续通过放宽超时掩盖硬件故障。

## 7. 实施结果

- 合同测试、Dashboard 回归和 PlatformIO 编译通过。
- 固件烧录及写入哈希校验通过。
- 实板串口读到温度 `27.1 C`、湿度 `69.0%`、`health.dht="ok"`、`health.oled="ready"`。
- Dashboard 截图确认温度 `27.1 C`、湿度 `70%`，光敏、声音和 PIR 同时保持实时更新。
- `study` 模式把温度阈值设为 `26 C` 后，温度卡片显示橙色提醒，原因包含当前值与阈值，蜂鸣器响，OLED 显示 `TEMP:HI`、`BZ:ON`。
- 温度阈值恢复为默认 `29 C` 后，提醒解除、蜂鸣器静音，OLED 恢复 `TEMP:OK`、`BZ:OFF`。
