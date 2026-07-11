# Dashboard iOS 模式图标与报警提醒设计

日期：2026-07-12
参考页面：`https://zhinengshenghuo.ilelezhan.cn/`

## 1. 目标

在现有小学组双栏 Dashboard 中增加两条评委可直接理解的视觉证据：

1. 当前模式实际使用哪些传感器，对应 iOS 风格线性图标先闪蓝约 `2200ms`，随后保持柔和蓝色并显示“本模式工作中”。
2. 固件上报提醒或报警时，页面说明“为什么触发、来自哪个 GPIO、当前值和阈值（可用时）、真实采取了什么动作”。

模式状态来自新鲜 `telemetry.mode`，提醒和报警只来自新鲜 `telemetry.alerts`。网页不根据原始传感器值自行制造报警结论。

## 2. 当前小学硬件边界

本功能只为当前实物核心传感器绘制卡片和图标：

| 卡片 | 模块 | GPIO | iOS 线性图标 |
| --- | --- | --- | --- |
| 光照 | 光敏传感器 | `GPIO1` | 太阳/光线 |
| 温度 | DHT11 | `GPIO14` | 温度计 |
| 湿度 | DHT11 | `GPIO14` | 水滴 |
| 噪声 | 声音传感器 | `GPIO4` | 声波/扬声器 |
| 人体 | PIR | `GPIO5` | 人体/活动 |

图标使用页面内本地 SVG 线条，不使用 emoji、外部 CDN 或网络字体。图标底座和卡片圆角不超过 `8px`。

不增加 8键AD、RGB、风扇和舵机。可选 MQ-2、水滴、火焰只有在固件 `alerts` 上报对应代码时进入原因通知，不预先增加不存在或未安装的传感器卡片。

## 3. 模式与传感器映射

映射必须与当前 `firmware/src/main.cpp::applyAutomation()` 一致：

| 模式 | 高亮卡片 | 原因 |
| --- | --- | --- |
| `study` 学习 | 光照、温度、湿度、噪声 | 光敏控制 D12；DHT11 温度和声音参与舒适提醒。温湿度来自同一个 DHT11，因此两张读数卡共同显示模块工作态。 |
| `rest` 休息 | 无 | 当前固件直接关闭学习灯和蜂鸣器，没有传感器参与自动联动，不做虚假高亮。 |
| `away` 离家 | 人体 | PIR 决定离家入侵报警。 |
| `energy` 节能 | 光照、人体 | 光照和 PIR 共同决定学习灯是否允许开启。 |

该映射只定义一次，放在纯函数模块中，由图标高亮和测试共同使用，避免在 CSS、HTML 和 `app.js` 中重复维护多份规则。

## 4. 三级视觉状态

优先级从高到低为：`红色报警 > 橙色提醒 > 蓝色模式工作 > 普通监测`。

### 4.1 蓝色模式工作

- 只在收到真实模式变化时启动约 `2200ms` 的蓝色图标闪动；连续 telemetry 不重复启动动画。
- 闪动结束后，参与当前模式的卡片保持柔和蓝色边框和图标底座。
- 状态标签显示“本模式工作中”。
- 首次连接收到模式时直接显示蓝色工作态，不制造“用户刚切换模式”的动画。

### 4.2 橙色普通提醒

| 告警码 | 卡片 | 标题 | 原因 |
| --- | --- | --- | --- |
| `noise` | 噪声 | 学习噪声提醒 | 学习模式声音超过页面当前噪声阈值；包含当前百分比和页面阈值。 |
| `temperature` | 温度 | 学习温度提醒 | 学习模式温度达到或超过页面当前温度阈值；包含当前温度和页面阈值。 |

橙色状态标签显示“正在提醒”。当前固件在这两种条件下会令蜂鸣器为开启状态，因此动作摘要按 telemetry 写成“蜂鸣器提醒，OLED/网页同步显示”；若 telemetry 中蜂鸣器实际为关闭，则只写网页提示，不虚构响铃。

### 4.3 红色安全报警

| 告警码 | 卡片/来源 | 标题 | 原因 |
| --- | --- | --- | --- |
| `intrusion` | PIR `GPIO5` | 离家人体报警 | 离家模式检测到人体活动。 |
| `mq2` / `gas` | 可选 MQ-2 `GPIO2` | 烟雾/燃气报警 | MQ-2 上报安全异常；只有 telemetry 提供当前值时才显示数值。 |
| `water` | 可选水滴 `GPIO8` | 漏水报警 | 水滴传感器上报漏水。 |
| `flame` | 可选火焰 `GPIO6` | 火源报警 | 火焰传感器上报火源信号。 |

`intrusion` 将 PIR 卡片标红并显示“正在报警”。可选安全告警没有对应核心卡片时，只在原因通知中标红，不凭空增加卡片。

动作摘要必须按当前小学固件和 telemetry 写：

- `intrusion`：学习灯关闭；蜂鸣器为 `true` 时写“蜂鸣器报警”；OLED/网页同步显示现状。
- MQ-2、水滴、火焰：当前固件没有风扇/RGB，也没有独立安全联动合同；只写页面正在显示安全报警，并依据 `actuators.buzzer` 的真实值决定是否写蜂鸣器。
- 禁止出现“风扇 100%”“RGB 红”或舵机动作。

## 5. 原因通知

在“学习小屋总览”标题和房间状态之间增加一个固定通知区域，使用：

```html
role="status" aria-live="polite" aria-atomic="true"
```

状态文案：

- 离线或无新鲜 telemetry：`等待实时数据`。
- 在线且无提醒/报警：`当前无报警或提醒`，说明传感器持续监测。
- 只有普通提醒：橙色标题 `检测到 N 项提醒`，逐项显示标题、原因、GPIO 和真实动作。
- 存在安全报警：红色标题 `检测到 N 项报警`；同一帧同时有提醒时仍逐项保留，红色安全报警决定通知主色。
- 未知代码：保留为 `设备上报异常：<code>`，来源写 `N16R8 实时数据`，不得静默丢弃。

多个告警去重但保持固件原始顺序。

## 6. 模块与数据流

新增 `dashboard/alert-core.js` 纯函数模块，职责如下：

- 唯一维护模式到传感器卡片的映射。
- 规范化、去重 `telemetry.alerts`。
- 把已知代码转成小学项目的中文标题、原因、GPIO、级别和传感器卡片键。
- 根据实际 `telemetry.actuators` 生成动作摘要。
- 未知代码安全回退。

浏览器数据流：

```text
新鲜 telemetry
  -> alert-core 生成模式键、提醒/报警说明和动作
  -> app.js 设置卡片 class、状态标签和原因通知
  -> CSS 渲染蓝/橙/红三级状态
```

## 7. 文件范围

| 文件 | 改动 |
| --- | --- |
| `dashboard/alert-core.js` | 新增模式映射、告警说明、级别和动作纯函数。 |
| `dashboard/tests/alert-core.test.js` | 测试四种模式、已知提醒/报警、多个代码、未知代码、值/阈值和真实动作。 |
| `dashboard/index.html` | 增加原因通知、五张传感器卡片的本地 iOS SVG 图标和状态标签，并加载新模块。 |
| `dashboard/app.js` | 跟踪真实模式变化时限，渲染卡片优先级、原因通知和离线状态。 |
| `dashboard/style.css` | 增加 iOS 图标底座、蓝色闪动/工作态、橙色提醒、红色报警和 reduced-motion。 |
| `dashboard/tests/layout-contract.test.js` | 增加新模块、通知 DOM、传感器键和静态版本检查。 |
| `开发文档.md` | 增加模式高亮和原因通知验收条目。 |

修改静态资源后，将 `index.html` 的 CSS、`alert-core.js` 和 `app.js` 查询版本更新为新的 `20260712-ios-mode-alerts`。当前仓库没有 `dashboard/sw.js`，本步骤不新增 Service Worker。

## 8. 自动与视觉验证

自动检查：

```bash
node --check dashboard/alert-core.js
node --check dashboard/app.js
node dashboard/tests/alert-core.test.js
node dashboard/tests/energy-core.test.js
node dashboard/tests/voice-intent.test.js
node dashboard/tests/layout-contract.test.js
python3 -m py_compile tools/n16r8_gateway.py tools/n16r8_cloud_relay.py tools/ws_json.py
PYTHONPATH=tools python3 -m unittest tools/test_gateway.py
/Users/yukii/.platformio/penv/bin/pio run -d firmware
git diff --check
```

隔离浏览器帧验证：

1. `study + alerts=[]`：光照、温度、湿度、噪声保持蓝色工作态。
2. 从 `rest` 切换到 `study`：上述图标闪蓝约 `2200ms`，连续帧不重启动画。
3. `study + noise`：噪声卡橙色，原因包含声音值和页面阈值。
4. `study + temperature`：温度卡橙色，原因包含温度值和页面阈值。
5. `away + intrusion`：PIR 卡红色，原因说明离家模式人体活动，不出现风扇/RGB 文案。
6. `energy + alerts=[]`：光照和 PIR 蓝色，节能分与中文原因继续正常显示。
7. 无新鲜 telemetry：图标和通知回到等待状态，不保留旧报警。
8. `1440 x 1000` 与 `390 x 844`：通知和长原因不溢出，手机无横向滚动，控制台零错误。

实板复验仍以真实 Web Serial 帧为准，隔离帧只用于 UI 状态测试，不作为硬件通过证据。

## 9. 范围边界

- 不修改固件模式逻辑、GPIO 或告警产生条件。
- 不用网页根据传感器值推断 `alerts`。
- 不加入参考站的初中硬件或动作文案。
- 不使用 emoji、外部图标 CDN、外部字体或网络依赖。
- 不把 mock/隔离帧写成实板验收记录。
