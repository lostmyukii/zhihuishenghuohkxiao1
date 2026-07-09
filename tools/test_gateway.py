#!/usr/bin/env python3
import unittest

from n16r8_gateway import MockBoardState, PROFILE_ID
from ws_json import is_protocol_frame, topic_for_frame


class GatewayContractTest(unittest.TestCase):
    def test_topic_mapping_uses_primary_prefix(self):
        self.assertEqual(
            topic_for_frame({"type": "telemetry"}, "smartlife/primary/n16r8"),
            "smartlife/primary/n16r8/telemetry",
        )
        self.assertEqual(
            topic_for_frame({"type": "ack"}, "smartlife/primary/n16r8"),
            "smartlife/primary/n16r8/event",
        )

    def test_protocol_frame_filter(self):
        self.assertTrue(is_protocol_frame({"type": "hello"}))
        self.assertTrue(is_protocol_frame({"type": "voiceIntent"}))
        self.assertFalse(is_protocol_frame({"type": "relayStatus"}))

    def test_mock_board_hello_and_mode_ack(self):
        board = MockBoardState()
        hello = board.hello()
        self.assertEqual(hello["profileId"], PROFILE_ID)
        self.assertEqual(hello["pins"]["light"], 1)
        self.assertEqual(hello["pins"]["rgb"], 47)

        ack = board.apply_command({"type": "command", "mode": "energy"})
        self.assertEqual(ack, {"type": "ack", "ok": True, "message": "mode=energy"})
        self.assertEqual(board.telemetry()["mode"], "energy")

    def test_mock_board_threshold_ack(self):
        board = MockBoardState()
        ack = board.apply_command(
            {
                "type": "command",
                "set": {
                    "lightThreshold": 25,
                    "soundThreshold": 70,
                    "thresholdFocus": "soundThreshold",
                },
            }
        )
        self.assertTrue(ack["ok"])
        self.assertEqual(board.light_threshold, 25)
        self.assertEqual(board.sound_threshold, 70)
        self.assertEqual(board.threshold_focus, "soundThreshold")

    def test_mock_board_keypad_and_oled_contract(self):
        board = MockBoardState()
        board.apply_command({"type": "command", "mode": "energy"})
        telemetry = board.telemetry()

        self.assertEqual(telemetry["sensors"]["keypadKey"], "D")
        self.assertEqual(telemetry["display"]["lastKey"], "D")
        self.assertEqual(telemetry["display"]["focus"], "lightThreshold")
        self.assertEqual(telemetry["health"]["oled"], "ready")
        self.assertEqual(telemetry["health"]["keypadLastKey"], "D")
        self.assertEqual(len(telemetry["display"]["lines"]), 6)
        self.assertIn("MODE:ENERGY", telemetry["display"]["lines"])


if __name__ == "__main__":
    unittest.main()
