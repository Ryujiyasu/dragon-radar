#!/usr/bin/env python3
"""
2BP UART sniffer.

MCU-Link Pro の Bridge UART (Virtual COM) 経由で 2BP のシリアル出力を覗き、
hex / ascii 双方で表示しつつ raw を bin ファイルに保存する。

Phase 1a (PC 先行解析) で使う。プロトコルが分かったらパーサに育てる。

Usage:
    python tools/uart_sniffer.py --port /dev/ttyACM0 --baud 115200
    python tools/uart_sniffer.py --scan
"""
import argparse
import datetime
import sys
import time
from pathlib import Path

try:
    import serial
    from serial.tools import list_ports
except ImportError:
    sys.stderr.write("pip install pyserial\n")
    sys.exit(1)


CAPTURE_DIR = Path(__file__).resolve().parent / "captures"


def list_serial_ports():
    ports = list(list_ports.comports())
    if not ports:
        print("(no serial ports detected)")
        return
    print("Available serial ports:")
    for p in ports:
        print(f"  {p.device:20s}  {p.description}  [{p.hwid}]")


def hexdump_line(offset, chunk):
    hexs = " ".join(f"{b:02x}" for b in chunk).ljust(48)
    asci = "".join(chr(b) if 32 <= b < 127 else "." for b in chunk)
    return f"{offset:08x}  {hexs}  |{asci}|"


def sniff(port, baud, duration=None):
    CAPTURE_DIR.mkdir(parents=True, exist_ok=True)
    ts = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
    bin_path = CAPTURE_DIR / f"capture_{ts}_{baud}.bin"
    print(f"Sniffing {port} @ {baud} 8N1 -> {bin_path}")
    print("Ctrl+C to stop.\n")

    offset = 0
    start = time.time()
    with serial.Serial(port, baud, timeout=0.1) as ser, open(bin_path, "wb") as fout:
        while True:
            if duration and (time.time() - start) > duration:
                break
            data = ser.read(64)
            if not data:
                continue
            fout.write(data)
            fout.flush()
            for i in range(0, len(data), 16):
                chunk = data[i:i + 16]
                print(hexdump_line(offset + i, chunk))
            offset += len(data)


def baud_scan(port, candidates=(9600, 38400, 115200, 230400, 460800, 921600), per=2.0):
    print(f"Probing {port} for plausible baud rate ({per:.1f}s each)...")
    best = None
    for b in candidates:
        try:
            with serial.Serial(port, b, timeout=0.1) as ser:
                start = time.time()
                bytes_seen = 0
                printable = 0
                while time.time() - start < per:
                    chunk = ser.read(256)
                    bytes_seen += len(chunk)
                    printable += sum(1 for x in chunk if 32 <= x < 127 or x in (9, 10, 13))
                ratio = (printable / bytes_seen) if bytes_seen else 0
                print(f"  {b:>7}: {bytes_seen:>5} B, printable ratio {ratio:.2f}")
                score = bytes_seen * (0.3 + ratio)
                if best is None or score > best[0]:
                    best = (score, b)
        except serial.SerialException as e:
            print(f"  {b:>7}: error {e}")
    if best:
        print(f"\nBest guess: {best[1]} baud (heuristic — may need confirmation)")
    return best[1] if best else None


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--port", help="serial device (e.g. /dev/ttyACM0)")
    ap.add_argument("--baud", type=int, default=115200)
    ap.add_argument("--list", action="store_true", help="list serial ports and exit")
    ap.add_argument("--scan", action="store_true", help="probe baud rates and exit")
    ap.add_argument("--duration", type=float, default=None, help="seconds to capture")
    args = ap.parse_args()

    if args.list:
        list_serial_ports()
        return
    if not args.port:
        list_serial_ports()
        ap.error("--port is required (use --list to see candidates)")
    if args.scan:
        baud_scan(args.port)
        return

    try:
        sniff(args.port, args.baud, args.duration)
    except KeyboardInterrupt:
        print("\n(stopped)")


if __name__ == "__main__":
    main()
