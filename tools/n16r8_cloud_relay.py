#!/usr/bin/env python3
"""Standalone WebSocket relay for the primary N16R8 SmartLife project."""

from __future__ import annotations

import argparse
import asyncio
from typing import Any, Dict

from ws_json import JsonRelayServer, json_dumps


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="N16R8 SmartLife primary WebSocket cloud relay")
    parser.add_argument("--host", default="0.0.0.0", help="relay bind host")
    parser.add_argument("--port", type=int, default=19166, help="relay WebSocket port")
    return parser


async def run(args: argparse.Namespace) -> None:
    relay = JsonRelayServer(args.host, args.port, name="smartlife-primary-relay", broadcast_incoming=True)
    await relay.start()
    print(f"relay listening on ws://{args.host}:{args.port}")
    while True:
        frame: Dict[str, Any] = await relay.incoming.get()
        print(f"relay frame {json_dumps(frame)}")


def main() -> None:
    args = build_parser().parse_args()
    try:
        asyncio.run(run(args))
    except KeyboardInterrupt:
        print("relay stopped")


if __name__ == "__main__":
    main()
