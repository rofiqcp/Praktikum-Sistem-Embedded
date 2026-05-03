#!/usr/bin/env python3
"""
SPI OLED SSD1306 Debug Script
===============================
Reads serial output from ESP32 SPI OLED program,
verifies initialization sequence, and logs command bytes sent.

Usage:
    python debug_spi_oled.py [--port /dev/ttyUSB0] [--baud 115200]
"""

import serial
import re
import sys
import argparse
import time
from collections import OrderedDict


# Expected SSD1306 initialization commands
EXPECTED_INIT_CMDS = OrderedDict([
    (0xAE, "Display OFF"),
    (0xD5, "Set Clock Divide"),
    (0xA8, "Set Mux Ratio"),
    (0xD3, "Set Display Offset"),
    (0x40, "Set Start Line"),
    (0x8D, "Charge Pump Setting"),
    (0x20, "Set Memory Addressing Mode"),
    (0xA1, "Segment Remap"),
    (0xC8, "COM Scan Direction"),
    (0xDA, "COM Pins Config"),
    (0x81, "Set Contrast"),
    (0xD9, "Set Pre-charge Period"),
    (0xDB, "Set VCOMH Level"),
    (0xA4, "Display from RAM"),
    (0xA6, "Normal Display"),
    (0xAF, "Display ON"),
])


def parse_cmd_line(line):
    """Parse a CMD log line to extract command byte(s)."""
    match = re.search(r'CMD:\s*(0x[0-9A-Fa-f]{2}(?:,0x[0-9A-Fa-f]{2})*)', line)
    if match:
        cmd_str = match.group(1)
        cmds = [int(c, 16) for c in cmd_str.split(',')]
        return cmds
    return None


def main():
    parser = argparse.ArgumentParser(description='SPI OLED SSD1306 Debug Monitor')
    parser.add_argument('--port', default='/dev/ttyUSB0', help='Serial port')
    parser.add_argument('--baud', type=int, default=115200, help='Baud rate')
    parser.add_argument('--timeout', type=int, default=30, help='Timeout in seconds')
    parser.add_argument('--file', type=str, help='Read from log file instead of serial')
    args = parser.parse_args()

    received_cmds = []
    init_complete = False
    display_updates = 0
    counter_values = []

    def process_line(line):
        nonlocal init_complete, display_updates

        line = line.strip()
        if not line:
            return

        print(f"[SERIAL] {line}")

        # Parse command bytes
        cmds = parse_cmd_line(line)
        if cmds:
            for cmd in cmds:
                received_cmds.append(cmd)
                cmd_name = EXPECTED_INIT_CMDS.get(cmd, "Unknown")
                print(f"    [CMD] 0x{cmd:02X} - {cmd_name}")

        # Check for initialization complete
        if 'initialization complete' in line.lower():
            init_complete = True
            print("\n>>> OLED Initialization Sequence Complete")
            print(f"    Total commands received: {len(received_cmds)}")

            # Verify initialization sequence
            print("\n    Verification of init commands:")
            for expected_cmd, desc in EXPECTED_INIT_CMDS.items():
                found = expected_cmd in received_cmds
                status = "OK" if found else "MISSING"
                print(f"      0x{expected_cmd:02X} ({desc}): {status}")

        # Track display updates
        if 'Display updated' in line:
            display_updates += 1
            counter_match = re.search(r'Counter:\s*(\d+)', line)
            if counter_match:
                counter_val = int(counter_match.group(1))
                counter_values.append(counter_val)
                print(f"    [UPDATE #{display_updates}] Counter = {counter_val}")

    # Read from file or serial
    if args.file:
        print(f"[INFO] Reading from file: {args.file}")
        with open(args.file, 'r') as f:
            for line in f:
                process_line(line)
    else:
        print(f"[INFO] Opening serial port {args.port} at {args.baud} baud")
        try:
            ser = serial.Serial(args.port, args.baud, timeout=1)
            print("[INFO] Listening for OLED debug data... (Ctrl+C to stop)\n")

            start_time = time.time()
            while True:
                if args.timeout and (time.time() - start_time) > args.timeout:
                    print(f"\n[INFO] Timeout ({args.timeout}s) reached")
                    break

                if ser.in_waiting > 0:
                    try:
                        line = ser.readline().decode('utf-8', errors='replace')
                        process_line(line)
                    except UnicodeDecodeError:
                        pass

        except serial.SerialException as e:
            print(f"[ERROR] Serial error: {e}")
            sys.exit(1)
        except KeyboardInterrupt:
            print("\n[INFO] Interrupted by user")
        finally:
            if 'ser' in locals():
                ser.close()

    # Summary
    print("\n" + "=" * 60)
    print("OLED DEBUG SUMMARY")
    print("=" * 60)
    print(f"  Initialization: {'COMPLETE' if init_complete else 'INCOMPLETE'}")
    print(f"  Commands sent: {len(received_cmds)}")
    print(f"  Display updates: {display_updates}")
    if counter_values:
        print(f"  Counter range: {min(counter_values)} - {max(counter_values)}")
        # Check if counter is incrementing correctly
        is_sequential = all(
            counter_values[i] == counter_values[i - 1] + 1
            for i in range(1, len(counter_values))
        )
        print(f"  Counter sequential: {'YES' if is_sequential else 'NO'}")
    print("=" * 60)


if __name__ == '__main__':
    main()
