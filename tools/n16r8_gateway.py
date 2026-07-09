#!/usr/bin/env python3
"""Serial/WebSocket/MQTT gateway for N16R8 primary SmartLife."""

from __future__ import annotations

import argparse
import asyncio
import glob
import json
import math
import time
from typing import Any, Dict, Iterable, Optional

from ws_json import JsonRelayServer, PROTOCOL_TOPIC_PREFIX, json_dumps, topic_for_frame

try:
    import serial  # type: ignore
except ImportError:  # pragma: no cover - depends on machine setup
    serial = None

try:
    import paho.mqtt.client as mqtt  # type: ignore
except ImportError:  # pragma: no cover - optional dependency
    mqtt = None


PROFILE_ID = "smartlife-primary-study-home-v1"
PROJECT = "smartlife-primary"


class MqttPublisher:
    def __init__(self, host: Optional[str], port: int, topic_prefix: str) -> None:
        self.enabled = False
        self.topic_prefix = topic_prefix
        self.client = None
        if not host:
            return
        if mqtt is None:
            print("mqtt disabled: paho-mqtt is not installed")
            return
        self.client = mqtt.Client(client_id="n16r8-primary-gateway")
        self.client.connect(host, port, keepalive=30)
        self.client.loop_start()
        self.enabled = True
        print(f"mqtt connected: {host}:{port} prefix={topic_prefix}")

    def publish(self, frame: Dict[str, Any]) -> None:
        if not self.enabled or self.client is None:
            return
        topic = topic_for_frame(frame, self.topic_prefix)
        self.client.publish(topic, json_dumps(frame), qos=0, retain=False)


class MockBoardState:
    def __init__(self) -> None:
        self.mode = "study"
        self.light_threshold = 35
        self.temperature_threshold = 29
        self.sound_threshold = 65
        self.away_delay_seconds = 10
        self.threshold_focus = "lightThreshold"
        self.last_key = "NONE"
        self.keypad_raw = 4095
        self.started_at = time.time()

    def hello(self) -> Dict[str, Any]:
        return {
            "type": "hello",
            "project": PROJECT,
            "board": "n16r8_esp32s3",
            "profileId": PROFILE_ID,
            "firmware": "mock-0.1.0",
            "deviceName": "N16R8 SmartLife Primary Mock",
            "baud": 115200,
            "capabilities": ["webSerial", "mqttBridge", "dashboard", "voiceIntent", "energyScore"],
            "pins": {
                "light": 1,
                "sound": 4,
                "dht": 14,
                "pir": 5,
                "keypad": 3,
                "lamp": 48,
                "fanPwm": 11,
                "fanDir": 12,
                "servo": 9,
                "rgb": 47,
                "buzzer": 13,
                "oledSda": 41,
                "oledScl": 42,
            },
        }

    def telemetry(self) -> Dict[str, Any]:
        elapsed = time.time() - self.started_at
        light = int(42 + 18 * math.sin(elapsed / 5))
        sound = int(24 + 16 * math.sin(elapsed / 3))
        temperature = round(27.2 + 1.8 * math.sin(elapsed / 7), 1)
        humidity = int(54 + 6 * math.sin(elapsed / 9))
        pir = int(elapsed) % 12 < 8
        lamp = self.mode == "study" and light < self.light_threshold
        fan = 60 if self.mode == "study" and temperature >= self.temperature_threshold else 0
        if self.mode == "energy" and (not pir or light > self.light_threshold):
            lamp = False
            fan = 0
        alerts = ["noise"] if self.mode == "study" and sound >= self.sound_threshold else []
        focus_short = {
            "lightThreshold": "LIGHT",
            "temperatureThreshold": "TEMP",
            "soundThreshold": "SOUND",
            "awayDelaySeconds": "AWAY",
        }.get(self.threshold_focus, "LIGHT")
        focus_value = {
            "lightThreshold": self.light_threshold,
            "temperatureThreshold": self.temperature_threshold,
            "soundThreshold": self.sound_threshold,
            "awayDelaySeconds": self.away_delay_seconds,
        }.get(self.threshold_focus, self.light_threshold)
        display_lines = [
            "N16R8 PRIMARY",
            f"MODE:{self.mode.upper()}",
            f"FOCUS:{focus_short} {focus_value}",
            f"L:{light} T:{round(temperature)} S:{sound}",
            f"KEY:{self.last_key} RAW:{self.keypad_raw}",
            f"PIR:{'ON' if pir else 'OFF'} FAN:{fan}",
        ]
        return {
            "type": "telemetry",
            "ts": int(elapsed * 1000),
            "mode": self.mode,
            "sensors": {
                "light": light,
                "sound": sound,
                "temperature": temperature,
                "humidity": humidity,
                "pir": pir,
                "keypadRaw": self.keypad_raw,
                "keypadKey": self.last_key,
            },
            "actuators": {
                "lamp": lamp,
                "fan": fan,
                "curtain": 45 if self.mode in {"study", "energy"} else 110,
                "rgb": {"study": "blue", "rest": "amber", "away": "red", "energy": "green"}.get(self.mode, "safe"),
                "buzzer": bool(alerts),
            },
            "alerts": alerts,
            "energy": {
                "score": 88 if self.mode == "energy" else 78,
                "reason": "mock-board-frame-for-dashboard-validation",
            },
            "display": {
                "lines": display_lines,
                "lastKey": self.last_key,
                "focus": self.threshold_focus,
            },
            "health": {
                "profileId": PROFILE_ID,
                "mqtt": "bridge",
                "voice": "ready",
                "thresholdFocus": self.threshold_focus,
                "keypadLastKey": self.last_key,
                "oled": "ready",
            },
        }

    def apply_command(self, frame: Dict[str, Any]) -> Dict[str, Any]:
        if frame.get("type") == "ping":
            return {"type": "ack", "ok": True, "message": "pong"}
        mode = frame.get("mode")
        if mode in {"study", "rest", "away", "energy"}:
            self.mode = mode
            self.last_key = {"study": "A", "rest": "B", "away": "C", "energy": "D"}[mode]
            self.keypad_raw = {"study": 120, "rest": 520, "away": 1020, "energy": 1520}[mode]
            return {"type": "ack", "ok": True, "message": f"mode={mode}"}
        settings = frame.get("set") or {}
        if "lightThreshold" in settings:
            self.light_threshold = int(settings["lightThreshold"])
        if "temperatureThreshold" in settings:
            self.temperature_threshold = int(settings["temperatureThreshold"])
        if "soundThreshold" in settings:
            self.sound_threshold = int(settings["soundThreshold"])
        if "awayDelaySeconds" in settings:
            self.away_delay_seconds = int(settings["awayDelaySeconds"])
        if settings.get("thresholdFocus") in {
            "lightThreshold",
            "temperatureThreshold",
            "soundThreshold",
            "awayDelaySeconds",
        }:
            self.threshold_focus = settings["thresholdFocus"]
        if settings:
            self.last_key = "UP"
            self.keypad_raw = 3050
            return {"type": "ack", "ok": True, "message": "set=thresholds"}
        if frame.get("type") == "voiceIntent":
            intent = frame.get("intent")
            if intent == "queryLight":
                return {"type": "ack", "ok": True, "message": "query=light"}
        return {"type": "ack", "ok": False, "message": "unknown-command"}


def candidate_serial_ports() -> Iterable[str]:
    patterns = [
        "/dev/cu.usbserial*",
        "/dev/tty.usbserial*",
        "/dev/cu.wchusbserial*",
        "/dev/tty.wchusbserial*",
        "/dev/cu.usbmodem*",
        "/dev/tty.usbmodem*",
    ]
    for pattern in patterns:
        for port in sorted(glob.glob(pattern)):
            yield port


def choose_serial_port(requested: Optional[str]) -> str:
    if requested:
        return requested
    for port in candidate_serial_ports():
        return port
    raise RuntimeError("no serial port found; pass --serial-port or use --mock-board")


async def emit_frame(relay: JsonRelayServer, publisher: MqttPublisher, frame: Dict[str, Any]) -> None:
    await relay.broadcast_json(frame, retain=frame.get("type") in {"hello", "telemetry", "health"})
    publisher.publish(frame)


async def serial_read_loop(relay: JsonRelayServer, publisher: MqttPublisher, serial_port: Any) -> None:
    while True:
        raw = await asyncio.to_thread(serial_port.readline)
        if not raw:
            await asyncio.sleep(0.02)
            continue
        line = raw.decode("utf-8", errors="replace").strip()
        if not line:
            continue
        if not line.startswith("{"):
            print(f"ignored non-json serial line: {line}")
            continue
        try:
            frame = json.loads(line)
        except json.JSONDecodeError as exc:
            print(f"invalid serial json: {exc}: {line}")
            continue
        await emit_frame(relay, publisher, frame)


async def serial_command_loop(relay: JsonRelayServer, serial_port: Any) -> None:
    while True:
        frame = await relay.incoming.get()
        frame_type = frame.get("type")
        if frame_type not in {"command", "voiceIntent", "ping"}:
            continue
        payload = (json_dumps(frame) + "\n").encode("utf-8")
        await asyncio.to_thread(serial_port.write, payload)
        await asyncio.to_thread(serial_port.flush)


async def mock_board_loop(relay: JsonRelayServer, publisher: MqttPublisher) -> None:
    board = MockBoardState()
    await emit_frame(relay, publisher, board.hello())

    async def consume_commands() -> None:
        while True:
            frame = await relay.incoming.get()
            if frame.get("type") not in {"command", "voiceIntent", "ping"}:
                continue
            await emit_frame(relay, publisher, board.apply_command(frame))

    asyncio.create_task(consume_commands())
    while True:
        await emit_frame(relay, publisher, board.telemetry())
        await asyncio.sleep(1)


async def run_gateway(args: argparse.Namespace) -> None:
    relay = JsonRelayServer(args.ws_host, args.ws_port, name="smartlife-primary-gateway", broadcast_incoming=True)
    await relay.start()
    publisher = MqttPublisher(args.mqtt_host, args.mqtt_port, args.topic_prefix)
    print(f"gateway websocket listening on ws://{args.ws_host}:{args.ws_port}")
    print(f"topic prefix: {args.topic_prefix}")

    if args.mock_board:
        print("mock board enabled")
        await mock_board_loop(relay, publisher)
        return

    if serial is None:
        raise RuntimeError("pyserial is not installed; use --mock-board or install pyserial")

    port_name = choose_serial_port(args.serial_port)
    board = serial.Serial(port_name, args.baud, timeout=0.1)
    print(f"serial connected: {port_name} baud={args.baud}")
    await asyncio.gather(
        serial_read_loop(relay, publisher, board),
        serial_command_loop(relay, board),
    )


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="N16R8 primary SmartLife serial/WebSocket/MQTT gateway")
    parser.add_argument("--mock-board", action="store_true", help="emit realistic mock hello/telemetry frames")
    parser.add_argument("--serial-port", help="explicit serial port, such as /dev/cu.usbserial-110")
    parser.add_argument("--baud", type=int, default=115200, help="serial baud rate")
    parser.add_argument("--ws-host", default="127.0.0.1", help="WebSocket bind host")
    parser.add_argument("--ws-port", type=int, default=19166, help="WebSocket bind port")
    parser.add_argument("--mqtt-host", help="optional MQTT broker host")
    parser.add_argument("--mqtt-port", type=int, default=1883, help="optional MQTT broker port")
    parser.add_argument("--topic-prefix", default=PROTOCOL_TOPIC_PREFIX, help="MQTT topic prefix")
    return parser


def main() -> None:
    args = build_parser().parse_args()
    try:
        asyncio.run(run_gateway(args))
    except KeyboardInterrupt:
        print("gateway stopped")


if __name__ == "__main__":
    main()
