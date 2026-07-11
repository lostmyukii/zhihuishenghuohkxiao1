#!/usr/bin/env python3

import re
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SOURCE = (ROOT / "firmware" / "src" / "main.cpp").read_text(encoding="utf-8")
PLATFORMIO = (ROOT / "firmware" / "platformio.ini").read_text(encoding="utf-8")


def function_body(name: str, next_name: str) -> str:
    pattern = rf"void {name}\([^)]*\) \{{(?P<body>.*?)\n\}}\n\nvoid {next_name}\("
    match = re.search(pattern, SOURCE, re.DOTALL)
    if not match:
        raise AssertionError(f"cannot locate {name} before {next_name}")
    return match.group("body")


class FirmwareSamplingContractTests(unittest.TestCase):
    def test_dht_pin_and_firmware_version(self):
        self.assertIn("constexpr uint8_t DHT = 14;", SOURCE)
        self.assertIn('constexpr const char *FIRMWARE_VERSION = "0.1.2";', SOURCE)

    def test_uses_pinned_dht_driver(self):
        self.assertIn("#include <DHT.h>", SOURCE)
        self.assertIn("DHT dht(Pins::DHT, DHT11);", SOURCE)
        self.assertIn("dht.begin();", SOURCE)
        self.assertIn("adafruit/DHT sensor library@1.4.6", PLATFORMIO)

    def test_fast_and_dht_intervals_are_independent(self):
        self.assertIn("constexpr unsigned long SENSOR_INTERVAL_MS = 200;", SOURCE)
        self.assertIn("constexpr unsigned long DHT_INTERVAL_MS = 2000;", SOURCE)
        self.assertIn("constexpr unsigned long DHT_STALE_MS = 6000;", SOURCE)

        fast_body = function_body("readFastSensors", "readDhtSensor")
        self.assertNotIn("dht.read", fast_body)
        self.assertIn("sensors.lightRaw", fast_body)
        self.assertIn("sensors.soundRaw", fast_body)
        self.assertIn("sensors.pir", fast_body)

    def test_transient_dht_failure_keeps_recent_value(self):
        dht_body = function_body("readDhtSensor", "setMode")
        self.assertIn("dht.readHumidity()", dht_body)
        self.assertIn("dht.readTemperature()", dht_body)
        self.assertIn("lastDhtSuccessAt = now;", dht_body)
        self.assertIn(
            "lastDhtSuccessAt == 0 || now - lastDhtSuccessAt >= DHT_STALE_MS",
            dht_body,
        )
        self.assertNotRegex(dht_body, r"sensors\.dhtValid\s*=\s*readDht11")

    def test_main_loop_runs_both_sampling_paths(self):
        self.assertRegex(
            SOURCE,
            r"(?s)now - lastSensorAt >= SENSOR_INTERVAL_MS.*?readFastSensors\(\)",
        )
        self.assertRegex(
            SOURCE,
            r"(?s)lastDhtReadAt == 0 \|\| now - lastDhtReadAt >= DHT_INTERVAL_MS.*?readDhtSensor\(now\)",
        )


if __name__ == "__main__":
    unittest.main()
