# 小学智慧生活公网部署与跨平台 USB 网关设计

日期：2026-07-12

状态：已部署，macOS 实板与远端双向链路通过；Windows 实板待现场确认

## 1. 目标

把当前 `N16R8 智慧低碳学习小屋` 部署到现有 HTTPS 域名的独立路径，使 Windows 和 macOS 的 Chrome/Edge 都能完成“打开网页 -> 插入 N16R8 -> 授权串口 -> 显示真实数据与控制实板”，并让第二台电脑或手机通过 WSS/MQTT 查看同一块实板的实时状态。

公网入口：

```text
https://hongkongxiao.ilelezhan.cn/
```

该域名的 A 记录已解析到 `82.156.146.105`。使用独立 Nginx server block 和独立 HTTPS 证书，不修改初中站点域名、首页和路由，不复用初中服务端口，不停止服务器上的其他系统。

## 2. 服务器审计结论

当前服务器已有初中 SmartLife 服务：

| 现有端口 | 服务 | 处理原则 |
| --- | --- | --- |
| `19166` | 初中 WSS Relay | 不修改、不重启。 |
| `19167` | 初中 Dashboard | 不修改、不重启。 |
| `19168` | 初中语音服务 | 不修改、不重启。 |
| `19183` | 初中 MQTT Broker | 不修改、不重启。 |

小学项目使用的新端口 `19266`、`19267`、`19283` 已确认未被占用。服务器现有 Nginx、HTTPS 证书、Docker、Mixly 和其他 systemd 服务保持原状。

## 3. 最终拓扑

```text
N16R8 实板
  -> Windows/macOS Chrome/Edge Web Serial
  -> 小学 Dashboard（USB 网关角色）
  -> WSS /smartlife-primary-ws
  -> 127.0.0.1:19266 小学 Cloud Relay
  -> 127.0.0.1:19283 小学 MQTT Broker
  -> WSS Relay
  -> 第二台电脑/手机 Dashboard
```

反向命令路径：

```text
远端 Dashboard 按钮
  -> WSS Relay -> MQTT command
  -> USB 电脑 Dashboard
  -> Web Serial command
  -> N16R8 -> ack/telemetry
  -> WSS/MQTT -> 所有 Dashboard
```

插 USB 的浏览器标签页是网关，必须保持打开。手机和 Safari 可观看云端状态，但不作为 USB 直连网关。

## 4. 隔离端口与目录

| 资源 | 配置 |
| --- | --- |
| 项目目录 | `/home/ubuntu/smartlife-primary` |
| Dashboard 静态服务 | `127.0.0.1:19267` |
| WSS Relay 后端 | `127.0.0.1:19266` |
| MQTT Broker | `127.0.0.1:19283` |
| 公网页面 | `https://hongkongxiao.ilelezhan.cn/` |
| 公网 WSS | `/smartlife-primary-ws` |
| MQTT topic | `smartlife/primary/n16r8/#` |

三个后端端口只绑定回环地址，公网只开放 Nginx 的现有 `443`。即使云防火墙已开放新端口，也不直接暴露 Python 静态服务和 MQTT Broker。

## 5. Dashboard 云端桥接

### 5.1 状态区分

页面新增“云端已连接/云端离线”状态，不把 WSS 在线等同于开发板在线：

- `串口已连接`：本浏览器持有 USB。
- `云端已连接`：本浏览器连接到小学 WSS Relay。
- `开发板在线`：最近约 `3.5s` 收到真实 `hello/telemetry`，来源可以是本地 USB 或云端。
- 没有新鲜实板帧时必须显示离线/等待，不能用 MQTT retained 数据长期伪装在线。

### 5.2 USB 主机行为

- Web Serial 收到 `hello/telemetry/ack` 后先在本地渲染，再附加 `origin=web-serial-gateway` 与随机 `originClientId` 发往 WSS。
- 本浏览器收到带有同一 `originClientId` 的回传帧时忽略，避免重复计数和重复动画。
- 本地按钮优先直接写入 USB，同时把命令状态发往云端；真实 `ack/telemetry` 再同步给远端。

### 5.3 远端行为

- 未连接 USB 的页面从 WSS 接收真实 `hello/telemetry/ack` 并正常渲染传感器、模式、报警原因、OLED 和执行器状态。
- 远端按钮通过 WSS 发送白名单 `command/voiceIntent`；USB 主机页面收到后写入串口。
- 页面不能把远端命令点击本身当成动作成功，必须等待实板 `ack/telemetry`。

## 6. WSS 与 MQTT Relay

`tools/n16r8_cloud_relay.py` 扩展为小学项目专用 MQTT/WSS Relay：

- 只使用 `smartlife/primary/n16r8` topic 前缀。
- 板端帧发布到 `hello/telemetry/health/event`；命令发布到 `command`。
- `hello/telemetry/health` 使用 retained，供新打开的远端页面获得最近状态；浏览器仍用本地接收时间做新鲜度超时。
- MQTT 暂时不可用时，WSS 客户端保持连接并显示云端异常，不影响 USB 本地控制。
- 过滤非 JSON、未知项目和不允许的命令类型。

MQTT 用户名和随机密码只保存在服务器 `/home/ubuntu/smartlife-primary/deploy/.env` 与 Mosquitto 密码文件中，权限为 `600`；不写入 Git、Dashboard 或开发文档。

## 7. Nginx 与 systemd

新增服务：

```text
smartlife-primary-web.service
smartlife-primary-relay.service
smartlife-primary-mqtt.service
```

Nginx 新增独立的 `hongkongxiao.ilelezhan.cn` server block，现有域名配置不做修改：

```text
/                         -> http://127.0.0.1:19267/
/smartlife-primary-ws  -> http://127.0.0.1:19266
```

实施顺序：

1. 备份现有 Nginx 配置和远端目标目录。
2. 上传到新的 `/home/ubuntu/smartlife-primary`，不覆盖初中目录。
3. 写入独立 MQTT 配置、环境文件和 systemd unit。
4. 启动三个小学服务，先用 localhost 探测。
5. 新增新域名的 HTTP/ACME server block，执行 `nginx -t` 后 reload。
6. 为 `hongkongxiao.ilelezhan.cn` 签发独立 Let's Encrypt 证书，再启用 HTTPS/WSS。
7. 再次执行 `nginx -t`，只执行 `systemctl reload nginx`，不重启 Nginx。
8. 任一步失败时停止小学新服务并恢复新增配置，不操作现有初中服务。

## 8. 跨平台验收

### 8.1 macOS 实板

- Chrome/Edge 的页面满足 `window.isSecureContext=true`、`navigator.serial` 可用。
- 授权 CH340 后收到固件 `0.1.2` 的真实 `hello/telemetry/ack`。
- 温湿度、光敏、噪声、PIR、D12、蜂鸣器和 OLED 与当前实板一致。
- USB 页面刷新后可重新授权；拔板后不能保持假在线。

### 8.2 Windows 实板

- Windows 10/11 Chrome 或 Edge 打开同一 HTTPS URL。
- 串口选择器能看到 `USB-SERIAL CH340 (COMx)`；如系统未识别，先安装对应 CH340 驱动。
- 页面不依赖 macOS `/dev/cu.*` 路径，只使用标准 `navigator.serial.requestPort()`。
- 完成连接、实时帧、模式命令和 `ack` 冒烟测试。

Safari、Firefox、iPhone 和普通手机浏览器不承诺 USB Web Serial，但可作为远端 WSS 观看端。Windows 的最终实板结论必须由真实 Windows + CH340 完成，不能只用代码审查代替。

### 8.3 远端同步

- USB 电脑连接实板后，第二个无 USB 浏览器看到连续变化的真实帧。
- USB 电脑切换模式或触发提醒时，远端页面同步更新。
- 远端下发一个模式命令，USB 页面写入串口，实板返回 `ack`，两端同步最终状态。
- 浏览器控制台无错误，390px 手机宽度无横向溢出。

## 9. 自动与上线检查

本地：

```bash
node --check dashboard/app.js
node dashboard/tests/cloud-bridge.test.js
node dashboard/tests/layout-contract.test.js
python3 -m py_compile tools/n16r8_cloud_relay.py tools/ws_json.py
PYTHONPATH=tools python3 -m unittest tools/test_cloud_relay.py tools/test_gateway.py
/Users/yukii/.platformio/penv/bin/pio run -d firmware
git diff --check
```

服务器：

```text
三个小学 systemd 服务均 active
19266/19267/19283 仅监听 127.0.0.1
现有 19166/19167/19168/19183 PID 与状态不变
nginx -t 通过
公网 HTML/JS/CSS 返回 200，静态文件 SHA-256 与本地一致
WSS 握手成功，真实 USB 帧可以到达第二客户端
```

## 10. 安全边界

- 不记录或提交 SSH 密码、MQTT 密码、Wi-Fi 密码和 API key。
- 不使用 `git add -A`，不上传本机隐藏配置。
- 不运行 `docker system prune`、全局端口清理或批量停止服务。
- 不修改初中站点文件、初中 MQTT topic 或初中 systemd unit。
- 不开放 `19266/19267/19283` 的公网直连；所有浏览器访问走现有 HTTPS/WSS。
- 部署完成后建议更换本次在对话中提供过的服务器登录密码。

## 11. 部署结果

- 公网地址 `https://hongkongxiao.ilelezhan.cn/` 已上线，HTTPS 证书有效并自动续期。
- `smartlife-primary-web`、`smartlife-primary-relay`、`smartlife-primary-mqtt` 均为 active，端口仅监听 `127.0.0.1`。
- 初中 `smartlife-junior-*` 四个服务在部署前后均为 active，未被重启。
- 公网静态文件 SHA-256 与本地一致，WSS `101`、MQTT retained、超时离线和 390px 布局验证通过。
- macOS Chrome 已用真实 N16R8 `0.1.2` 验证连续遥测；第二个无 USB 浏览器完成 `rest -> ack -> study -> ack` 远端命令闭环。
- Windows 页面代码只使用标准 Web Serial API，不依赖 `/dev/cu.*`；真实 Windows + CH340 冒烟测试仍是最终待办，当前不得写成实板通过。
