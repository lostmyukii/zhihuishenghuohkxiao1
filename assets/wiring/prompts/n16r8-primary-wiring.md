Create a clean Chinese educational wiring diagram as a raster image.

Topic: N16R8 智慧低碳学习小屋 · 小学组硬件连接示意图.

Aspect ratio: 16:9 landscape.

Style: hand-drawn technical education diagram, warm cream paper background, neat colored wires, clear module icons, large readable Chinese labels, no photorealism. Use a semi-realistic simplified ESP32-S3 N16R8 expansion board in the center-right, and a small house model on the left. The diagram must feel like a judge-facing teaching wiring guide.

Main composition:

1. Center-right: "N16R8 ESP32-S3 + S3扩展板" board.
   - Show rows of white G-V-S sockets.
   - Add a label: "G=黑线 GND / V=红线 低压电源 / S=黄线 信号".
   - USB-C cable exits to the right and points to "Chrome/Edge HTTPS Dashboard".

2. Left: small simplified house model with three zones:
   - "学习房": desk lamp, light sensor, sound sensor.
   - "客厅中控": OLED, N16R8 service area, USB cable to dashboard.
   - "玄关": PIR sensor and buzzer.

3. Blue sensing wires from house modules to board:
   - "光敏 ADC1 / GPIO1"
   - "DHT11 D14 / GPIO14"
   - "声音 ADC4 / GPIO4"
   - "PIR D5 / GPIO5"

4. Orange actuator wires from board to house modules:
   - "学习灯 GPIO48"
   - "蜂鸣器 D13 / GPIO13"

5. Purple display/communication wires:
   - "OLED SDA=GPIO41 / SCL=GPIO42"
   - "USB 115200 JSON"
   - "Web Serial -> WSS -> MQTT -> Dashboard"

6. Optional safety extension in a dashed red box, marked "可选扩展":
   - "MQ-2 ADC2 / GPIO2：AO需0~3.3V"
   - "水滴 D8 / GPIO8"
   - "火焰 D6 / GPIO6：不用明火演示"

7. Bottom warning strip:
   - "只接低压负载，不接220V"
   - "所有模块共地"
   - "网页在线必须来自 hello / telemetry / ack"

Visual requirements:

- Make wiring routes clear and color-coded: blue=sensing, orange=actuator, purple=display/communication, red=danger/safety extension.
- Use large Chinese text labels; avoid tiny text.
- Keep the exact GPIO labels listed above as much as possible.
- Do not draw 8键AD, RGB灯带, fan, or servo; this hardware version does not include them.
- The diagram should not imply that relay or high-power devices are inside the room; keep heavy control in service area.
- Do not invent unrelated sensors.
- Do not show 220V wiring.
