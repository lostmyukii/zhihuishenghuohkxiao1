# Dashboard 节能状态中文展示设计

日期：2026-07-12

## 1. 目标

在现有 Dashboard 的“执行器”区域显示开发板真实上报的 `telemetry.energy.score` 和 `telemetry.energy.reason`，让小学组“节能响应”可以在 PIR 实板测试中被直接看见和讲清。

本功能不在网页端重新判断是否节能。节能结论仍由 N16R8 固件产生，Dashboard 只负责把新鲜遥测翻译成适合小学组评委阅读的中文。

## 2. 界面位置

在现有“执行器”面板内，学习灯和蜂鸣器状态下方增加一个节能状态区域：

- `节能分`：显示 `telemetry.energy.score`，格式为 `90 分`。
- `节能原因`：只显示中文解释，不显示固件英文原因码。

该位置与学习灯状态相邻，现场可以按“人体状态 -> 学习灯动作 -> 节能原因”的顺序讲解，不占用小屋总览或事件记录空间。

## 3. 数据与中文映射

新增纯函数模块 `dashboard/energy-core.js`，负责把固件原因码转换成中文。首版映射如下：

| 固件原因码 | Dashboard 中文解释 |
| --- | --- |
| `empty-room-light-off` | 无人，已自动关闭学习灯 |
| `energy-mode-balances-light-and-comfort` | 节能模式正在根据光照和人体状态控制学习灯 |
| `study-mode-keeps-comfort-when-needed` | 学习模式按需保持照明与舒适提醒 |
| `scene-rules-active` | 当前模式的智能规则正在运行 |
| `mock-board-frame-for-dashboard-validation` | 模拟开发板正在验证节能状态显示 |

如果固件直接上报中文原因，则原样显示。未知英文原因码不暴露给评委，统一显示“设备正在执行节能规则”。

## 4. 离线与异常状态

- 只有收到新鲜 `telemetry` 时才显示节能分和节能原因。
- 串口未连接、开发板离线、遥测超时或 `energy` 字段缺失时，两项都显示“等待实时数据”。
- `score` 不是有效数字时显示“等待实时数据”，不自动补零。
- 不根据 PIR、光照或学习灯状态在网页端推断节能原因，避免网页结论与固件实际规则漂移。

## 5. 文件改动

| 文件 | 改动 |
| --- | --- |
| `dashboard/energy-core.js` | 新增原因码中文映射和安全回退纯函数。 |
| `dashboard/tests/energy-core.test.js` | 测试已知原因、中文直通、未知原因、缺失值和分数格式。 |
| `dashboard/index.html` | 在执行器面板增加节能分/原因元素，并先加载 `energy-core.js`。 |
| `dashboard/app.js` | 从新鲜 `telemetry.energy` 渲染节能状态。 |
| `dashboard/style.css` | 增加紧凑、可换行的节能原因样式。 |

修改静态 JavaScript 时同步更新 `dashboard/index.html` 的查询版本。如果仓库后续新增 Service Worker，还必须同步更新缓存名和资源清单；当前仓库没有 `dashboard/sw.js`，本步骤不创建 PWA 缓存。

## 6. 验证

自动检查：

```bash
node --check dashboard/energy-core.js
node --check dashboard/app.js
node dashboard/tests/energy-core.test.js
node dashboard/tests/voice-intent.test.js
git diff --check
```

实板验收：

1. Dashboard 收到实时 `telemetry` 后显示数字节能分和中文原因。
2. 切换到 `energy` 模式并遮挡光敏，PIR 有人时允许 D12 学习灯打开。
3. 人离开 PIR 感应范围后，人体状态变为“无人”，D12 学习灯关闭。
4. 节能原因显示“无人，已自动关闭学习灯”。
5. 断开 USB 或遥测超时后，节能分和原因显示“等待实时数据”，不保留旧结论冒充在线状态。

## 7. 范围边界

- 不修改固件自动化条件和 GPIO 映射。
- 不增加风扇、RGB、舵机或八键 AD。
- 不新增网页端节能算法。
- 不把 mock 数据当作实板验收证据。
