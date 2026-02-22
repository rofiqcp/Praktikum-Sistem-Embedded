#!/usr/bin/env python3
"""
STM32_09_UART_Bridge_ESP32 - Protocol Analyzer & Bridge Simulator
=================================================================
Tools:
1. Protocol frame encoder/decoder
2. Bridge simulator (simulates ESP32 responses)
3. Checksum calculator/verifier
4. Serial protocol analyzer with logging

Usage:
  python debug_analysis.py --analyze          # Analyze protocol frames
  python debug_analysis.py --simulate PORT    # Simulate ESP32 bridge on serial port
  python debug_analysis.py --decode HEX       # Decode a hex frame
"""

import struct
import time
import sys
import argparse
from datetime import datetime

# Protocol constants
PROTO_STX = 0x02
PROTO_ETX = 0x03
CMD_WIFI_CONNECT = 0x01
CMD_SEND_DATA    = 0x02
CMD_GET_STATUS   = 0x03
CMD_GET_IP       = 0x04
CMD_ACK          = 0x10
CMD_NACK         = 0x11

CMD_NAMES = {
    0x01: "WIFI_CONNECT",
    0x02: "SEND_DATA",
    0x03: "GET_STATUS",
    0x04: "GET_IP",
    0x10: "ACK",
    0x11: "NACK",
}

def calculate_checksum(cmd, data):
    """Calculate XOR checksum of CMD + LEN_L + LEN_H + DATA."""
    data_len = len(data)
    cs = cmd
    cs ^= (data_len & 0xFF)
    cs ^= ((data_len >> 8) & 0xFF)
    for b in data:
        cs ^= b
    return cs & 0xFF

def build_frame(cmd, data=b""):
    """Build a protocol frame: STX + CMD + LEN(2) + DATA + CS + ETX."""
    data_len = len(data)
    cs = calculate_checksum(cmd, data)
    frame = bytearray()
    frame.append(PROTO_STX)
    frame.append(cmd)
    frame.append(data_len & 0xFF)
    frame.append((data_len >> 8) & 0xFF)
    frame.extend(data)
    frame.append(cs)
    frame.append(PROTO_ETX)
    return bytes(frame)

def decode_frame(frame_bytes):
    """Decode a protocol frame and return parsed fields."""
    if len(frame_bytes) < 6:
        return {"error": "Frame too short"}
    if frame_bytes[0] != PROTO_STX:
        return {"error": f"Invalid STX: 0x{frame_bytes[0]:02X}"}
    if frame_bytes[-1] != PROTO_ETX:
        return {"error": f"Invalid ETX: 0x{frame_bytes[-1]:02X}"}

    cmd = frame_bytes[1]
    data_len = frame_bytes[2] | (frame_bytes[3] << 8)
    
    if len(frame_bytes) < 6 + data_len:
        return {"error": f"Frame length mismatch: expected {6 + data_len}, got {len(frame_bytes)}"}

    data = frame_bytes[4:4 + data_len]
    rx_cs = frame_bytes[4 + data_len]
    calc_cs = calculate_checksum(cmd, data)

    return {
        "cmd": cmd,
        "cmd_name": CMD_NAMES.get(cmd, f"UNKNOWN(0x{cmd:02X})"),
        "data_len": data_len,
        "data": data,
        "data_str": data.decode("utf-8", errors="replace") if data else "",
        "checksum_rx": rx_cs,
        "checksum_calc": calc_cs,
        "checksum_ok": rx_cs == calc_cs,
        "frame_hex": " ".join(f"{b:02X}" for b in frame_bytes),
    }

def analyze_protocol():
    """Demonstrate protocol analysis with sample frames."""
    print("=" * 60)
    print("  UART Bridge Protocol Analyzer")
    print("=" * 60)

    test_cases = [
        ("WiFi Connect", CMD_WIFI_CONNECT, b"MySSID:MyPassword"),
        ("Send Data",    CMD_SEND_DATA,    b'{"temp":25.5,"hum":60}'),
        ("Get Status",   CMD_GET_STATUS,   b""),
        ("Get IP",       CMD_GET_IP,       b""),
        ("ACK Response", CMD_ACK,          b"OK"),
    ]

    for name, cmd, data in test_cases:
        print(f"\n--- {name} ---")
        frame = build_frame(cmd, data)
        print(f"  Frame ({len(frame)} bytes): {' '.join(f'{b:02X}' for b in frame)}")

        decoded = decode_frame(frame)
        print(f"  CMD      : {decoded['cmd_name']} (0x{decoded['cmd']:02X})")
        print(f"  Data Len : {decoded['data_len']}")
        print(f"  Data     : {decoded['data_str']}")
        print(f"  Checksum : 0x{decoded['checksum_rx']:02X} {'OK' if decoded['checksum_ok'] else 'FAIL'}")

    # Test corrupted frame
    print(f"\n--- Corrupted Frame Test ---")
    frame = build_frame(CMD_SEND_DATA, b"test_data")
    corrupted = bytearray(frame)
    corrupted[5] ^= 0xFF  # Corrupt data byte
    decoded = decode_frame(bytes(corrupted))
    print(f"  Original CS : 0x{frame[-2]:02X}")
    print(f"  Corrupted CS: 0x{decoded['checksum_rx']:02X} vs calc 0x{decoded['checksum_calc']:02X}")
    print(f"  Verification: {'PASS' if not decoded['checksum_ok'] else 'FAIL - should detect!'}")

def simulate_bridge(port_name="/dev/ttyUSB0", baudrate=115200):
    """Simulate ESP32 bridge responder on a serial port."""
    try:
        import serial
    except ImportError:
        print("ERROR: pyserial not installed. Run: pip install pyserial")
        print("\nRunning in demo mode instead...\n")
        simulate_bridge_demo()
        return

    print(f"Opening {port_name} at {baudrate} baud...")
    try:
        ser = serial.Serial(port_name, baudrate, timeout=0.1)
    except serial.SerialException as e:
        print(f"ERROR: {e}")
        print("\nRunning in demo mode instead...\n")
        simulate_bridge_demo()
        return

    print(f"Bridge simulator active on {port_name}")
    print("Waiting for STM32 commands...\n")

    rx_buf = bytearray()
    stats = {"rx": 0, "tx": 0, "errors": 0}

    try:
        while True:
            data = ser.read(64)
            if data:
                rx_buf.extend(data)

                # Look for complete frames
                while PROTO_STX in rx_buf:
                    stx_idx = rx_buf.index(PROTO_STX)
                    if stx_idx > 0:
                        rx_buf = rx_buf[stx_idx:]

                    if len(rx_buf) < 6:
                        break

                    data_len = rx_buf[2] | (rx_buf[3] << 8)
                    frame_len = 6 + data_len

                    if len(rx_buf) < frame_len:
                        break

                    frame = bytes(rx_buf[:frame_len])
                    rx_buf = rx_buf[frame_len:]

                    decoded = decode_frame(frame)
                    ts = datetime.now().strftime("%H:%M:%S.%f")[:-3]

                    if "error" in decoded:
                        print(f"[{ts}] RX ERROR: {decoded['error']}")
                        stats["errors"] += 1
                        continue

                    stats["rx"] += 1
                    print(f"[{ts}] RX #{stats['rx']}: {decoded['cmd_name']} "
                          f"len={decoded['data_len']} data='{decoded['data_str']}'")

                    # Generate response
                    response_data = generate_response(decoded)
                    if response_data is not None:
                        resp_frame = build_frame(CMD_ACK, response_data)
                        ser.write(resp_frame)
                        stats["tx"] += 1
                        print(f"[{ts}] TX #{stats['tx']}: ACK data='{response_data.decode()}'")

            time.sleep(0.01)

    except KeyboardInterrupt:
        print(f"\n\nStats: RX={stats['rx']} TX={stats['tx']} Errors={stats['errors']}")
        ser.close()

def generate_response(decoded):
    """Generate simulated response for a command."""
    cmd = decoded["cmd"]
    if cmd == CMD_WIFI_CONNECT:
        return b"WIFI_OK:192.168.1.100"
    elif cmd == CMD_GET_STATUS:
        return b"STATUS:CONNECTED"
    elif cmd == CMD_GET_IP:
        return b"IP:192.168.1.100"
    elif cmd == CMD_SEND_DATA:
        return b"DATA_OK"
    return None

def simulate_bridge_demo():
    """Demo mode without real serial port."""
    print("=== Bridge Simulator Demo Mode ===\n")

    commands = [
        (CMD_WIFI_CONNECT, b"TestSSID:TestPass"),
        (CMD_GET_STATUS,   b""),
        (CMD_SEND_DATA,    b'{"temp":25.0}'),
        (CMD_GET_IP,       b""),
    ]

    for cmd, data in commands:
        frame = build_frame(cmd, data)
        decoded = decode_frame(frame)
        print(f"STM32 -> ESP32: {decoded['cmd_name']} data='{decoded['data_str']}'")

        resp_data = generate_response(decoded)
        if resp_data:
            resp_frame = build_frame(CMD_ACK, resp_data)
            resp_decoded = decode_frame(resp_frame)
            print(f"ESP32 -> STM32: ACK data='{resp_decoded['data_str']}'")
        print()

def decode_hex_frame(hex_str):
    """Decode a hex string frame."""
    hex_str = hex_str.replace(" ", "").replace("0x", "").replace(",", "")
    try:
        frame_bytes = bytes.fromhex(hex_str)
    except ValueError as e:
        print(f"ERROR: Invalid hex string: {e}")
        return

    decoded = decode_frame(frame_bytes)
    if "error" in decoded:
        print(f"Decode error: {decoded['error']}")
    else:
        print(f"Command  : {decoded['cmd_name']} (0x{decoded['cmd']:02X})")
        print(f"Data Len : {decoded['data_len']}")
        print(f"Data     : {decoded['data_str']}")
        print(f"Data Hex : {' '.join(f'{b:02X}' for b in decoded['data'])}")
        print(f"Checksum : 0x{decoded['checksum_rx']:02X} "
              f"({'VALID' if decoded['checksum_ok'] else 'INVALID'})")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="UART Bridge Protocol Analyzer")
    parser.add_argument("--analyze", action="store_true", help="Run protocol analysis demo")
    parser.add_argument("--simulate", type=str, metavar="PORT", nargs="?", const="/dev/ttyUSB0",
                        help="Simulate ESP32 bridge on serial port")
    parser.add_argument("--decode", type=str, metavar="HEX", help="Decode hex frame")
    args = parser.parse_args()

    if args.decode:
        decode_hex_frame(args.decode)
    elif args.simulate:
        simulate_bridge(args.simulate)
    elif args.analyze:
        analyze_protocol()
    else:
        analyze_protocol()
