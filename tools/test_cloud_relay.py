#!/usr/bin/env python3

import json
import unittest

from n16r8_cloud_relay import (
    PROJECT,
    browser_frame_from_mqtt,
    mqtt_connect_succeeded,
    mqtt_route_for_frame,
    normalize_frame,
)


class CloudRelayContractTests(unittest.TestCase):
    def test_paho_v2_reason_code_compatibility(self):
        class ReasonCode:
            def __init__(self, is_failure):
                self.is_failure = is_failure

        self.assertTrue(mqtt_connect_succeeded(ReasonCode(False)))
        self.assertFalse(mqtt_connect_succeeded(ReasonCode(True)))
        self.assertTrue(mqtt_connect_succeeded(0))
        self.assertFalse(mqtt_connect_succeeded(5))

    def test_normalize_accepts_only_primary_protocol(self):
        self.assertEqual(normalize_frame({"type": "telemetry"})["project"], PROJECT)
        self.assertIsNone(normalize_frame({"type": "demo"}))
        self.assertIsNone(normalize_frame({"type": "telemetry", "project": "smartlife-junior"}))

    def test_board_frame_routes_to_primary_topic_and_retains(self):
        topic, message, retain = mqtt_route_for_frame({"type": "telemetry", "sensors": {"light": 42}})
        self.assertEqual(topic, "smartlife/primary/n16r8/telemetry")
        self.assertTrue(retain)
        self.assertEqual(json.loads(message)["project"], PROJECT)

    def test_command_routes_without_retain(self):
        topic, message, retain = mqtt_route_for_frame({"type": "command", "mode": "energy"})
        self.assertEqual(topic, "smartlife/primary/n16r8/command")
        self.assertFalse(retain)
        self.assertEqual(json.loads(message)["mode"], "energy")

    def test_ping_and_unknown_frames_are_not_published(self):
        self.assertIsNone(mqtt_route_for_frame({"type": "ping"}))
        self.assertIsNone(mqtt_route_for_frame({"type": "unknown"}))

    def test_mqtt_payload_returns_browser_metadata(self):
        payload = browser_frame_from_mqtt(
            "smartlife/primary/n16r8/ack",
            json.dumps({"type": "ack", "ok": True, "message": "mode=study"}),
            "smartlife/primary/n16r8",
        )
        self.assertEqual(payload["project"], PROJECT)
        self.assertEqual(payload["mqttTopic"], "smartlife/primary/n16r8/ack")
        self.assertIsInstance(payload["relayedAt"], int)

        self.assertIsNone(
            browser_frame_from_mqtt(
                "smartlife/junior/n16r8/telemetry",
                json.dumps({"type": "telemetry"}),
                "smartlife/primary/n16r8",
            )
        )


if __name__ == "__main__":
    unittest.main()
