#!/usr/bin/env python3
"""Tiny JSON WebSocket helpers for the N16R8 SmartLife tools.

This module intentionally avoids third-party runtime dependencies. It supports
the subset needed by the local gateway and cloud relay: text frames, close
frames, ping/pong, and broadcast to connected browser clients.
"""

from __future__ import annotations

import asyncio
import base64
import hashlib
import json
import struct
from typing import Any, Dict, Optional, Set


WEBSOCKET_GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
PROTOCOL_TOPIC_PREFIX = "smartlife/primary/n16r8"


def json_dumps(payload: Dict[str, Any]) -> str:
    return json.dumps(payload, ensure_ascii=False, separators=(",", ":"))


def topic_for_frame(frame: Dict[str, Any], prefix: str = PROTOCOL_TOPIC_PREFIX) -> str:
    frame_type = str(frame.get("type") or "event")
    if frame_type in {"hello", "telemetry", "health", "command", "config", "voiceIntent"}:
        suffix = frame_type
    elif frame_type == "ack":
        suffix = "event"
    else:
        suffix = "event"
    return f"{prefix.rstrip('/')}/{suffix}"


def is_protocol_frame(frame: Dict[str, Any]) -> bool:
    return frame.get("type") in {
        "hello",
        "telemetry",
        "ack",
        "command",
        "config",
        "voiceIntent",
        "ping",
    }


class WebSocketPeer:
    def __init__(self, reader: asyncio.StreamReader, writer: asyncio.StreamWriter) -> None:
        self.reader = reader
        self.writer = writer

    @classmethod
    async def accept(cls, reader: asyncio.StreamReader, writer: asyncio.StreamWriter) -> "WebSocketPeer":
        request = await reader.readuntil(b"\r\n\r\n")
        headers = _parse_headers(request.decode("utf-8", errors="replace"))
        key = headers.get("sec-websocket-key")
        if not key:
            writer.write(b"HTTP/1.1 400 Bad Request\r\nConnection: close\r\n\r\n")
            await writer.drain()
            raise ConnectionError("missing Sec-WebSocket-Key")

        accept_key = base64.b64encode(hashlib.sha1((key + WEBSOCKET_GUID).encode()).digest()).decode()
        response = (
            "HTTP/1.1 101 Switching Protocols\r\n"
            "Upgrade: websocket\r\n"
            "Connection: Upgrade\r\n"
            f"Sec-WebSocket-Accept: {accept_key}\r\n"
            "\r\n"
        )
        writer.write(response.encode("ascii"))
        await writer.drain()
        return cls(reader, writer)

    async def recv_text(self) -> Optional[str]:
        header = await self.reader.readexactly(2)
        byte_one, byte_two = header[0], header[1]
        opcode = byte_one & 0x0F
        masked = (byte_two & 0x80) != 0
        length = byte_two & 0x7F

        if length == 126:
            length = struct.unpack("!H", await self.reader.readexactly(2))[0]
        elif length == 127:
            length = struct.unpack("!Q", await self.reader.readexactly(8))[0]

        mask = await self.reader.readexactly(4) if masked else b""
        payload = await self.reader.readexactly(length) if length else b""
        if masked:
            payload = bytes(payload[i] ^ mask[i % 4] for i in range(len(payload)))

        if opcode == 0x8:
            return None
        if opcode == 0x9:
            await self.send_frame(payload, opcode=0xA)
            return ""
        if opcode != 0x1:
            return ""
        return payload.decode("utf-8", errors="replace")

    async def send_text(self, text: str) -> None:
        await self.send_frame(text.encode("utf-8"), opcode=0x1)

    async def close(self) -> None:
        try:
            await self.send_frame(b"", opcode=0x8)
        except Exception:
            pass
        self.writer.close()
        try:
            await self.writer.wait_closed()
        except Exception:
            pass

    async def send_frame(self, payload: bytes, opcode: int) -> None:
        length = len(payload)
        header = bytearray([0x80 | opcode])
        if length < 126:
            header.append(length)
        elif length <= 0xFFFF:
            header.append(126)
            header.extend(struct.pack("!H", length))
        else:
            header.append(127)
            header.extend(struct.pack("!Q", length))
        self.writer.write(bytes(header) + payload)
        await self.writer.drain()


class JsonRelayServer:
    def __init__(self, host: str, port: int, *, name: str, broadcast_incoming: bool = True) -> None:
        self.host = host
        self.port = port
        self.name = name
        self.broadcast_incoming = broadcast_incoming
        self.incoming: asyncio.Queue[Dict[str, Any]] = asyncio.Queue()
        self.peers: Set[WebSocketPeer] = set()
        self.retained: Dict[str, str] = {}
        self.server: Optional[asyncio.AbstractServer] = None

    async def start(self) -> None:
        self.server = await asyncio.start_server(self._handle_client, self.host, self.port)

    async def serve_forever(self) -> None:
        if self.server is None:
            await self.start()
        async with self.server:
            await self.server.serve_forever()

    async def broadcast_json(self, payload: Dict[str, Any], *, retain: bool = False) -> None:
        if retain:
            retained_key = str(payload.get("type") or "event")
            self.retained[retained_key] = json_dumps(payload)
        await self.broadcast_text(json_dumps(payload))

    async def broadcast_text(self, text: str) -> None:
        stale = []
        for peer in list(self.peers):
            try:
                await peer.send_text(text)
            except Exception:
                stale.append(peer)
        for peer in stale:
            self.peers.discard(peer)

    async def _handle_client(self, reader: asyncio.StreamReader, writer: asyncio.StreamWriter) -> None:
        peer: Optional[WebSocketPeer] = None
        try:
            peer = await WebSocketPeer.accept(reader, writer)
            self.peers.add(peer)
            await self.broadcast_json({"type": "relayStatus", "name": self.name, "clients": len(self.peers)})
            for text in self.retained.values():
                await peer.send_text(text)
            while True:
                text = await peer.recv_text()
                if text is None:
                    break
                if not text:
                    continue
                try:
                    payload = json.loads(text)
                except json.JSONDecodeError:
                    payload = {"type": "raw", "text": text}
                await self.incoming.put(payload)
                if self.broadcast_incoming:
                    retain = payload.get("type") in {"hello", "telemetry", "health"}
                    await self.broadcast_json(payload, retain=retain)
        except (asyncio.IncompleteReadError, ConnectionError):
            pass
        finally:
            if peer is not None:
                self.peers.discard(peer)
            try:
                writer.close()
                await writer.wait_closed()
            except Exception:
                pass
            await self.broadcast_json({"type": "relayStatus", "name": self.name, "clients": len(self.peers)})


def _parse_headers(request: str) -> Dict[str, str]:
    headers: Dict[str, str] = {}
    for line in request.split("\r\n")[1:]:
        if ":" not in line:
            continue
        key, value = line.split(":", 1)
        headers[key.strip().lower()] = value.strip()
    return headers
