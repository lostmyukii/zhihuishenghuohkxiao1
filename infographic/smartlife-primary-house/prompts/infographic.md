Create a professional Chinese infographic following these specifications:

## Image Specifications

- Type: Infographic
- Layout: hub-spoke
- Style: hand-drawn-edu
- Aspect Ratio: 16:9 landscape
- Language: zh
- Output target: `infographic/smartlife-primary-house/infographic.png`

## Core Principles

- This image is for a primary-school SmartLife competition design.
- The main goal is to show how the primary-school tasks map to the N16R8 project.
- Keep the visual story clear: task -> sensor -> judgment -> actuator -> OLED/dashboard.
- Use short, large, legible Chinese labels. Avoid tiny paragraphs.
- Use real hardware concepts from the design: N16R8 ESP32-S3, light sensor, DHT11, sound sensor, PIR, relay lamp, buzzer, OLED, Web Serial, MQTT dashboard.
- Do not depict fake or impossible wiring. Keep the board and sensors as a small educational model.

## Layout Guidelines: hub-spoke

- Put a prominent central hub in the middle: a cute hand-drawn model house named "N16R8 智慧低碳学习小屋".
- Four large spoke nodes radiate around the house:
  1. 数据采集
  2. 智能控制
  3. 语音交互
  4. 节能响应
- Each node should include 2-4 short labels and simple doodle icons.
- Add a compact hardware legend on the right edge.
- Add a connection ribbon along the bottom for Web Serial -> WSS -> MQTT -> Dashboard.
- Add a tiny 4-step demo timeline along the lower area.

## Style Guidelines: hand-drawn-edu

- Warm cream paper background (#F5F0E8), subtle paper grain.
- Hand-drawn educational style with slight wobbly outlines.
- Macaron pastel rounded cards:
  - blue for sensing / 数据采集
  - peach or orange for automatic control / 智能控制
  - lavender for voice interaction / 语音交互
  - mint green for energy saving / 节能响应
- Deep charcoal text (#2D2D2D), clear handwritten print style.
- Add small doodles: lightbulb, thermometer, microphone, human PIR icon, buzzer, OLED screen, phone dashboard, cloud, MQTT message bubbles.
- Include one small student character pointing at the model house.
- Keep generous whitespace and strong hierarchy.

## Infographic Content

### Main Title

"N16R8 智慧低碳学习小屋"

Subtitle:

"小学组智慧生活任务映射"

### Central Hub

Draw a small learning-house model:

- learning room with desk lamp, light sensor, sound sensor
- central control area with OLED and N16R8 board
- entrance with PIR sensor
- neat colored wires

Central labels:

- "N16R8 中控"
- "学习房"
- "客厅中控"
- "玄关"
- "OLED 看板"

### Spoke 1: 数据采集

Card title:

"1 数据采集"

Short subhead:

"采得到 · 读得稳 · 表达清楚"

Labels:

- "光照"
- "温湿度"
- "噪声"
- "人体"
- "OLED/网页同步"

Visual:

light sensor, DHT11 thermometer, sound wave, PIR human icon, arrows to OLED and a small dashboard.

### Spoke 2: 智能控制

Card title:

"2 智能控制"

Short subhead:

"阈值判断 -> 自动执行"

Labels:

- "光暗开灯"
- "温度高提醒"
- "噪声提醒"

Visual:

if/then arrows: dark light -> lamp on, hot temperature -> OLED/dashboard reminder, noisy sound -> small reminder bell.

### Spoke 3: 语音交互

Card title:

"3 语音交互"

Short subhead:

"说指令，也要回读状态"

Labels:

- "开始学习"
- "开启节能"
- "现在光线怎么样"
- "安全白名单"

Visual:

microphone speech bubbles flowing to mode buttons and status reply.

### Spoke 4: 节能响应

Card title:

"4 节能响应"

Short subhead:

"合适时机关掉，讲清原因"

Labels:

- "无人"
- "光照足"
- "关闭学习灯"
- "节能分"

Visual:

PIR no-person icon, bright sun, lamp off, green energy score badge.

### Right Hardware Legend

Small heading:

"真实硬件合同"

Compact labels:

- "光敏 GPIO1"
- "DHT11 GPIO14"
- "声音 GPIO4"
- "PIR GPIO5"
- "灯 D12/GPIO12"
- "OLED 41/42"

Use small colored tag stickers; keep text large enough to read.

### Bottom Connection Ribbon

Heading:

"沿用初中亮点：MQTT + 可视化"

Draw this as a simple arrow ribbon:

"N16R8 USB" -> "Web Serial" -> "WSS Relay" -> "MQTT" -> "Dashboard"

Small labels:

- "hello"
- "telemetry"
- "ack"

### Bottom Mini Timeline

Heading:

"5分钟演示闭环"

Four mini steps:

1. "采集：遮光 / 拍手 / PIR"
2. "控制：灯亮 / 蜂鸣提醒"
3. "语音+节能：开启节能"
4. "同步：网页看板变化"

## Text Requirements

- Use only the Chinese text specified above.
- Keep labels short and large.
- Do not invent extra claims.
- Avoid long paragraphs and tiny captions.
- The infographic should be readable at slide scale.
