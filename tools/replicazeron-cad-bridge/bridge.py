#!/usr/bin/env python3
"""Read Replicazeron's CAD stick report and forward it to local CAD add-ons."""

from __future__ import annotations

import argparse
import json
import math
import socket
import sys
import time
from dataclasses import asdict, dataclass
from typing import Any

VID = 0x4142
PID = 0x2305
RAW_USAGE_PAGE = 0xFF60
RAW_USAGE = 0x61
PACKET_SIZE = 32
COMMAND = 0x70
CAD_STICK_GET = 0x1C
DEFAULT_PORTS = (28461, 28462)


@dataclass(frozen=True)
class CadSample:
    version: int
    sequence: int
    active: bool
    x: float
    y: float
    strength: float
    timestamp: float


def decode_report(report: bytes | bytearray | list[int], sequence: int, timestamp: float) -> CadSample:
    data = bytes(report)
    if len(data) == PACKET_SIZE + 1 and data[0] == 0:
        data = data[1:]
    if len(data) != PACKET_SIZE or data[0] != COMMAND or data[1] != CAD_STICK_GET:
        raise ValueError("not a Replicazeron CAD-stick reply")

    version = data[3]
    active = data[4] == 1
    angle = (data[5] << 8) | data[6]
    distance = (data[7] << 8) | data[8]
    deadzone = (data[9] << 8) | data[10]
    if version != 1 or angle >= 360 or deadzone >= 512:
        raise ValueError("unsupported or malformed CAD-stick reply")

    strength = 0.0
    if active and distance > deadzone:
        strength = min(1.0, (distance - deadzone) / max(1, 512 - deadzone))
    radians = math.radians(angle)
    # Firmware angle 0 is up, 90 is left, 180 is down, and 270 is right.
    x = -math.sin(radians) * strength
    y = math.cos(radians) * strength
    return CadSample(version, sequence, active, x, y, strength, timestamp)


class ReplicazeronHid:
    def __init__(self) -> None:
        self._hid: Any = None
        self._device: Any = None

    def open(self) -> None:
        try:
            import hid  # type: ignore
        except ImportError as error:
            raise RuntimeError("Install the bridge dependency with: pip install -r requirements.txt") from error

        matches = hid.enumerate(VID, PID)
        raw_matches = [
            item
            for item in matches
            if item.get("usage_page") == RAW_USAGE_PAGE and item.get("usage") == RAW_USAGE
        ]
        candidates = raw_matches or matches
        if not candidates:
            raise RuntimeError("Replicazeron is not connected")

        device = hid.device()
        device.open_path(candidates[0]["path"])
        device.set_nonblocking(False)
        self._hid = hid
        self._device = device

    def close(self) -> None:
        if self._device is not None:
            try:
                self._device.close()
            finally:
                self._device = None

    def poll(self, sequence: int, timeout_ms: int) -> CadSample:
        if self._device is None:
            raise RuntimeError("Replicazeron HID is not open")
        request = bytearray(PACKET_SIZE)
        request[0] = COMMAND
        request[1] = CAD_STICK_GET
        written = self._device.write(bytes([0]) + request)
        if written <= 0:
            raise OSError("failed to write the Replicazeron HID request")

        deadline = time.monotonic() + timeout_ms / 1000.0
        while True:
            remaining = max(1, int((deadline - time.monotonic()) * 1000))
            reply = self._device.read(PACKET_SIZE, remaining)
            if not reply:
                raise TimeoutError("Replicazeron did not answer the CAD-stick request")
            try:
                return decode_report(reply, sequence, time.monotonic())
            except ValueError:
                if time.monotonic() >= deadline:
                    raise TimeoutError("no matching Replicazeron CAD-stick reply")


def demo_sample(sequence: int, started: float) -> CadSample:
    elapsed = time.monotonic() - started
    angle = elapsed * 1.2
    strength = 0.65
    return CadSample(1, sequence, True, math.sin(angle) * strength, math.cos(angle) * strength, strength, time.monotonic())


def send_sample(sock: socket.socket, ports: tuple[int, ...], sample: CadSample) -> None:
    payload = json.dumps(asdict(sample), separators=(",", ":")).encode("utf-8")
    for port in ports:
        sock.sendto(payload, ("127.0.0.1", port))


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Replicazeron Fusion/FreeCAD analog navigation bridge")
    parser.add_argument("--rate", type=float, default=60.0, help="stick reports per second (default: 60)")
    parser.add_argument("--ports", type=int, nargs="+", default=list(DEFAULT_PORTS), help="localhost UDP destination ports")
    parser.add_argument("--demo", action="store_true", help="send a circular test signal without a controller")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    if not 10 <= args.rate <= 240:
        print("--rate must be between 10 and 240", file=sys.stderr)
        return 2
    ports = tuple(dict.fromkeys(args.ports))
    if any(port < 1024 or port > 65535 for port in ports):
        print("ports must be between 1024 and 65535", file=sys.stderr)
        return 2

    interval = 1.0 / args.rate
    timeout_ms = max(20, int(interval * 1000 * 2))
    sender = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    device = ReplicazeronHid()
    sequence = 0
    started = time.monotonic()
    connected = False
    last_error = ""
    next_tick = started

    print(f"Replicazeron CAD bridge -> localhost ports {', '.join(map(str, ports))}")
    try:
        while True:
            next_tick += interval
            try:
                if args.demo:
                    sample = demo_sample(sequence, started)
                else:
                    if not connected:
                        device.open()
                        connected = True
                        print("Replicazeron connected")
                        last_error = ""
                    sample = device.poll(sequence, timeout_ms)
                send_sample(sender, ports, sample)
                sequence = (sequence + 1) & 0xFFFFFFFF
            except (OSError, RuntimeError, TimeoutError) as error:
                message = str(error)
                if connected or message != last_error:
                    print(f"Replicazeron unavailable: {message}", file=sys.stderr)
                    last_error = message
                if connected:
                    inactive = CadSample(1, sequence, False, 0.0, 0.0, 0.0, time.monotonic())
                    send_sample(sender, ports, inactive)
                connected = False
                device.close()
                time.sleep(1.0)
                next_tick = time.monotonic()

            delay = next_tick - time.monotonic()
            if delay > 0:
                time.sleep(delay)
            elif delay < -interval:
                next_tick = time.monotonic()
    except KeyboardInterrupt:
        print("Stopping CAD bridge")
    finally:
        device.close()
        sender.close()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
