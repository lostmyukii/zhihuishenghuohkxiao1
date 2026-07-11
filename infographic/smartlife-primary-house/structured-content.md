# N16R8 智慧低碳学习小屋

## Overview

本信息图展示 `N16R8 智慧低碳学习小屋` 如何把小学组智慧生活任务落到真实硬件、生活场景、MQTT 与网页可视化上。图面重点是“小学组任务映射”，不是单纯罗列模块。

## Learning Objectives

The viewer will understand:

1. 小学组智慧生活的四项任务如何展开。
2. 每项任务对应哪些传感器、判断逻辑、执行器和可视化反馈。
3. 初中项目的 MQTT 与可视化能力如何作为小学组展示增强项。

---

## Section 1: 中心作品

**Key Concept**: 作品以真实小屋模型为中心，让传感、判断、执行和网页同步同时可见。

**Content**:

- "作品名称：**N16R8 智慧低碳学习小屋**"
- "不是“智能家居大全”，而是“学生学习空间的舒适与节能助手”。"
- "场景可落在一间学习房加客厅中控：书桌、灯、门口 PIR、中控 OLED。"

**Visual Element**:

- Type: central illustrated model
- Subject: 一间学习房 + 客厅中控 + 玄关的小屋，中央放 N16R8，连到书桌灯、蜂鸣器、OLED、PIR
- Treatment: 友好的小学科技作品模型，线束整齐，硬件点位真实

**Text Labels**:

- Headline: "N16R8 智慧低碳学习小屋"
- Subhead: "小学组智慧生活任务映射"
- Labels: "学习房", "客厅中控", "玄关", "N16R8 中控", "OLED 看板"

---

## Section 2: 小学任务一 数据采集

**Key Concept**: 数据采集要让评委看到真实数值变化。

**Content**:

- "数据采集 | 采得到、读得稳、表达清楚。"
- "采集光照、温湿度、噪声、人体状态，并在 OLED 与网页看板同步显示。"
- "遮挡光敏、热水杯靠近 DHT、拍手、PIR 前挥手"

**Visual Element**:

- Type: task card
- Subject: 光敏、DHT11、声音传感器、PIR 四个小图标连到 OLED 和网页仪表盘
- Treatment: 蓝色感知线，数值小卡片

**Text Labels**:

- Headline: "1 数据采集"
- Subhead: "采得到 · 读得稳 · 表达清楚"
- Labels: "光照", "温湿度", "噪声", "人体", "OLED/网页同步"

---

## Section 3: 小学任务二 智能控制

**Key Concept**: 智能控制要体现条件满足时系统自动响应。

**Content**:

- "智能控制 | 根据预设光照或温度阈值自动触发生活设备。"
- "光线暗自动开学习灯，温度高在 OLED/网页提示，噪声高短提醒。"
- "灯自动亮，温度/噪声提醒出现，页面显示触发原因"

**Visual Element**:

- Type: task card
- Subject: 光线暗 -> 学习灯亮；温度高 -> OLED/网页提醒；噪声高 -> 蜂鸣短提醒
- Treatment: 橙色执行线，带箭头的因果关系

**Text Labels**:

- Headline: "2 智能控制"
- Subhead: "阈值判断 -> 自动执行"
- Labels: "光暗开灯", "温度高提醒", "噪声提醒"

---

## Section 4: 小学任务三 语音交互

**Key Concept**: 语音交互要能触发真实模式，也能回读当前状态。

**Content**:

- "语音交互 | 指令能精准触发功能，并能回读状态。"
- "网页语音/小智语音发布安全白名单指令，如“开始学习”“开启节能”“现在光照多少”。"
- "开始学习 | `setMode: study` | 切换学习模式"
- "开启节能 | `setMode: energy` | 切换节能模式"
- "现在光线怎么样 | `queryLight` | 回读光照与灯状态"

**Visual Element**:

- Type: task card
- Subject: 麦克风气泡、语音命令卡片、模式按钮、返回状态文字
- Treatment: 紫色交互线，安全白名单徽章

**Text Labels**:

- Headline: "3 语音交互"
- Subhead: "说指令，也要回读状态"
- Labels: "开始学习", "开启节能", "现在光线怎么样", "安全白名单"

---

## Section 5: 小学任务四 节能响应

**Key Concept**: 节能响应要说明为什么关闭学习灯或调整状态。

**Content**:

- "节能响应 | 在合适时机关掉或切换到节能状态，解释为什么省电。"
- "无人或光照充足时关闭学习灯，网页显示节能分与节能原因。"
- "节能模式下：无人或光照足时关闭学习灯，并显示节能原因。"

**Visual Element**:

- Type: task card
- Subject: PIR 无人、太阳充足、学习灯关闭、节能分上升
- Treatment: 绿色节能线，分数环或电池图标

**Text Labels**:

- Headline: "4 节能响应"
- Subhead: "合适时机关掉，更要讲清原因"
- Labels: "无人", "光照足", "关闭学习灯", "节能分"

---

## Section 6: 真实硬件合同

**Key Concept**: 所有视觉元素必须对应真实 N16R8 端口和模块。

**Content**:

- "感知 | 光敏传感器 | `ADC1 / GPIO1`"
- "感知 | DHT11 温湿度 | `D14 / GPIO14`"
- "感知 | 声音传感器 | `ADC4 / GPIO4`"
- "感知 | PIR 人体红外 | `D5 / GPIO5`"
- "执行 | 继电器/学习灯 | `D12 / GPIO12`"
- "执行 | 无源蜂鸣器 | `D13 / GPIO13`"
- "显示 | OLED | `SDA=GPIO41, SCL=GPIO42`"

**Visual Element**:

- Type: compact hardware legend
- Subject: 蓝色感知、橙色执行、紫色显示/通信三组模块
- Treatment: 小标签贴纸，端口号清晰

**Text Labels**:

- Headline: "真实硬件合同"
- Labels: "感知", "执行", "显示", "GPIO"

---

## Section 7: MQTT 与可视化链路

**Key Concept**: 初中项目的 MQTT 与可视化作为小学作品的展示增强项。

**Content**:

- "N16R8 USB -> HTTPS Dashboard Web Serial -> WSS Cloud Relay -> MQTT Broker -> 其他电脑/手机网页看板"
- "Web Serial 看板：复用初中可视化框架，改成小学 profile 与 `smartlife/primary/n16r8` topic。"
- "MQTT topic 能看到 `hello/telemetry/event/health/ack`。"

**Visual Element**:

- Type: horizontal connection ribbon
- Subject: N16R8 USB 到电脑网页，再到云端 Relay、MQTT、手机/平板
- Treatment: 清晰箭头，避免过度复杂

**Text Labels**:

- Headline: "沿用初中亮点：MQTT + 可视化"
- Labels: "Web Serial", "WSS Relay", "MQTT", "Dashboard", "hello/telemetry/ack"

---

## Section 8: 5 分钟演示闭环

**Key Concept**: 评委应该能按一条顺序看到完整闭环。

**Content**:

- "0:30-1:15 | 遮挡光敏、热水杯靠近 DHT、拍手、PIR 挥手。 | 数据采集稳定，OLED/网页同步"
- "1:15-2:00 | 切到学习模式，遮光后灯自动亮；拍手或升温后出现提醒。 | 智能控制闭环"
- "2:00-2:45 | 说“开启节能”或点击语音快捷键；离开 PIR 范围。 | 语音触发和节能响应"
- "2:45-3:30 | 打开另一台设备网页或刷新页面，看 MQTT/WSS 状态同步。 | 沿用初中可视化与 MQTT"

**Visual Element**:

- Type: mini timeline
- Subject: 四步演示路径
- Treatment: 底部小时间线，编号 1-4

**Text Labels**:

- Headline: "5 分钟演示闭环"
- Labels: "采集", "控制", "语音+节能", "MQTT 同步"

---

## Data Points (Verbatim)

### Statistics

- "5 分钟演示脚本"
- "0:30-1:15"
- "1:15-2:00"
- "2:00-2:45"
- "2:45-3:30"

### Quotes

- "小学规则主线：数据采集 -> 智能控制 -> 语音交互 -> 节能响应"
- "真实硬件主线：N16R8 ESP32-S3 -> 光/DHT/声/PIR -> 学习灯/蜂鸣器/OLED"
- "初中功能迁移：Web Serial -> WSS Relay -> MQTT -> 可视化 Dashboard"

### Key Terms

- **N16R8 智慧低碳学习小屋**: 作品名称。
- **Web Serial**: HTTPS Dashboard 通过浏览器授权 USB 串口。
- **MQTT**: 展示增强项，用于状态同步和网页可视化。
- **小学组任务映射**: 数据采集、智能控制、语音交互、节能响应。

---

## Design Instructions

### Style Preferences

- 使用 image 模型生成位图信息图。
- 面向小学组，风格应清晰、友好、有教学感。
- 图中硬件和场景要对应真实 N16R8 模块，不要画成虚构智能家居。

### Layout Preferences

- 以“智慧低碳学习小屋”为中心。
- 四个小学组任务围绕中心展开。
- 底部保留 MQTT/可视化链路和 5 分钟演示闭环。

### Other Requirements

- 输出中文信息图。
- 重点展示小学组任务映射。
- 保留 MQTT 和可视化功能，但作为展示增强项。
