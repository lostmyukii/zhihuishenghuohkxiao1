---
title: "N16R8 智慧低碳学习小屋"
topic: "educational technical infographic"
data_type: "system overview with task mapping"
complexity: "complex"
point_count: 10
source_language: "zh"
user_language: "zh"
---

## Main Topic

本信息图把 `N16R8 小学组智慧生活作品设计方案` 转化为一张面向评委、老师和学生的视觉说明图。核心是展示小学组四项任务如何映射到真实 N16R8 硬件、生活场景、MQTT 可视化和演示闭环。

## Learning Objectives

After viewing this infographic, the viewer should understand:

1. 小学组智慧生活的四项任务是“数据采集、智能控制、语音交互、节能响应”。
2. `N16R8 智慧低碳学习小屋` 如何用真实传感器、执行器、OLED 和网页看板完成任务闭环。
3. 初中项目的 MQTT 与可视化能力如何迁移为小学组作品的展示增强项。

## Target Audience

- **Knowledge Level**: Beginner to Intermediate
- **Context**: 比赛方案展示、答辩说明、作品设计沟通
- **Expectations**: 快速看懂小学组任务映射、真实硬件模块、演示流程和 MQTT/可视化链路

## Content Type Analysis

- **Data Structure**: 中心作品概念 + 四个小学任务映射 + 硬件合同 + MQTT 可视化链路 + 演示步骤。
- **Key Relationships**: 小学任务 -> 传感器变化 -> 判断逻辑 -> 执行器动作 -> OLED/网页同步。
- **Visual Opportunities**: 中心小屋模型、四个任务卡片、真实 GPIO 标签、Web Serial/WSS/MQTT 箭头链路、5 分钟演示时间线。

## Key Data Points (Verbatim)

- "作品名称：**N16R8 智慧低碳学习小屋**"
- "数据采集 | 采得到、读得稳、表达清楚。"
- "智能控制 | 根据预设光照或温度阈值自动触发生活设备。"
- "语音交互 | 指令能精准触发功能，并能回读状态。"
- "节能响应 | 在合适时机关掉或切换到节能状态，解释为什么省电。"
- "N16R8 USB -> HTTPS Dashboard Web Serial -> WSS Cloud Relay -> MQTT Broker -> 其他电脑/手机网页看板"
- "感知 | 光敏传感器 | `ADC1 / GPIO1`"
- "感知 | DHT11 温湿度 | `D14 / GPIO14`"
- "感知 | 声音传感器 | `ADC4 / GPIO4`"
- "感知 | PIR 人体红外 | `D5 / GPIO5`"
- "执行 | 继电器/学习灯 | `D12 / GPIO12`"
- "执行 | 无源蜂鸣器 | `D13 / GPIO13`"
- "显示 | OLED | `SDA=GPIO41, SCL=GPIO42`"
- "小学规则主线：数据采集 -> 智能控制 -> 语音交互 -> 节能响应"
- "真实硬件主线：N16R8 ESP32-S3 -> 光/DHT/声/PIR -> 学习灯/蜂鸣器/OLED"
- "初中功能迁移：Web Serial -> WSS Relay -> MQTT -> 可视化 Dashboard"

## Layout × Style Signals

- Content type: system overview with task mapping -> suggests `hub-spoke`, `bento-grid`, `dense-modules`
- Tone: 小学组、教学展示、真实硬件 -> suggests `hand-drawn-edu`, `morandi-journal`, `technical-schematic`
- Audience: 评委、老师、学生 -> suggests clear labels, friendly diagrams, task-first structure
- Complexity: complex -> requires one strong center visual and grouped modules

## Design Instructions (from user input)

- 使用 image 模型生成。
- 表示上面的设计。
- 注意结合小学组任务映射展开。
- 保留 MQTT 和可视化亮点，但不要盖过小学组任务。

## Recommended Combinations

1. **hub-spoke + hand-drawn-edu** (Recommended): 中心是“智慧低碳学习小屋”，四周展开“数据采集、智能控制、语音交互、节能响应”，非常适合小学组任务映射。
2. **dense-modules + morandi-journal**: 适合做高密度大图，把硬件 GPIO、MQTT topic、演示流程和验收清单都放进去，信息量更足。
3. **linear-progression + hand-drawn-edu**: 适合按“传感器变化 -> 判断 -> 执行器动作 -> OLED/网页同步”讲演示流程，动线清楚。
4. **structural-breakdown + technical-schematic**: 适合偏工程答辩，突出 N16R8、传感器、执行器、Web Serial、MQTT 的结构关系。
