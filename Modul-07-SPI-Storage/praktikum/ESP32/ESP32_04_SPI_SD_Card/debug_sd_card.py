#!/usr/bin/env python3
"""
SPI SD Card Debug Script
==========================
Reads serial output from ESP32 SD card program,
tracks file operations (create/read/write/list),
and reports card information.

Usage:
    python debug_sd_card.py [--port /dev/ttyUSB0] [--baud 115200]
"""

import serial
import re
import sys
import argparse
import time
from collections import OrderedDict


def main():
    parser = argparse.ArgumentParser(description='SPI SD Card Debug Monitor')
    parser.add_argument('--port', default='/dev/ttyUSB0', help='Serial port')
    parser.add_argument('--baud', type=int, default=115200, help='Baud rate')
    parser.add_argument('--timeout', type=int, default=30, help='Timeout in seconds')
    parser.add_argument('--file', type=str, help='Read from log file instead of serial')
    args = parser.parse_args()

    card_info = {}
    file_operations = []
    directory_entries = []
    mount_status = None
    unmount_status = None
    verification_result = None
    current_step = None
    file_contents_capture = False
    file_contents = []

    def process_line(line):
        nonlocal mount_status, unmount_status, verification_result
        nonlocal current_step, file_contents_capture

        line = line.strip()
        if not line:
            return

        print(f"[SERIAL] {line}")

        # Track steps
        step_match = re.search(r'Step\s+(\d+):\s*(.+?)=', line)
        if step_match:
            current_step = f"Step {step_match.group(1)}: {step_match.group(2).strip()}"
            print(f"\n>>> {current_step}")

        # Mount status
        if 'mounted successfully' in line.lower():
            mount_status = 'SUCCESS'
            print("    >>> SD Card: MOUNTED")
        elif 'Failed to initialize' in line or 'mount failed' in line.lower():
            mount_status = 'FAILED'
            print("    >>> SD Card: MOUNT FAILED")

        # Card info
        card_name_match = re.search(r'Card name:\s*(\S+)', line)
        if card_name_match:
            card_info['name'] = card_name_match.group(1)
            print(f"    >>> Card name: {card_info['name']}")

        type_match = re.search(r'Type:\s*(.+)', line)
        if type_match and 'Type' not in card_info:
            card_info['type'] = type_match.group(1).strip()

        capacity_match = re.search(r'Capacity:\s*(\d+)\s*MB', line)
        if capacity_match:
            card_info['capacity_mb'] = int(capacity_match.group(1))
            print(f"    >>> Capacity: {card_info['capacity_mb']} MB")

        sector_match = re.search(r'Sector size:\s*(\d+)', line)
        if sector_match:
            card_info['sector_size'] = int(sector_match.group(1))

        # File operations
        if 'Writing file:' in line:
            path_match = re.search(r'Writing file:\s*(.+)', line)
            if path_match:
                file_operations.append(('WRITE', path_match.group(1).strip()))
                print(f"    >>> FILE WRITE: {path_match.group(1).strip()}")

        if 'Reading file:' in line:
            path_match = re.search(r'Reading file:\s*(.+)', line)
            if path_match:
                file_operations.append(('READ', path_match.group(1).strip()))
                print(f"    >>> FILE READ: {path_match.group(1).strip()}")

        if 'Appending to file:' in line:
            path_match = re.search(r'Appending to file:\s*(.+)', line)
            if path_match:
                file_operations.append(('APPEND', path_match.group(1).strip()))
                print(f"    >>> FILE APPEND: {path_match.group(1).strip()}")

        # File size
        size_match = re.search(r'File size.*?:\s*(\d+)\s*bytes', line)
        if size_match:
            print(f"    >>> File size: {size_match.group(1)} bytes")

        # File contents capture
        if '--- BEGIN FILE ---' in line:
            file_contents_capture = True
            file_contents.clear()
            return
        if '--- END FILE ---' in line:
            file_contents_capture = False
            print(f"    >>> Captured {len(file_contents)} lines of file content")
            return
        if file_contents_capture:
            file_contents.append(line)
            return

        # Directory listing
        dir_match = re.match(r'\s+(\S+)\s+(FILE|DIR|OTHER)\s+(.+)', line)
        if dir_match and dir_match.group(1) != '----' and dir_match.group(1) != 'Name':
            entry = {
                'name': dir_match.group(1),
                'type': dir_match.group(2),
                'size': dir_match.group(3).strip(),
            }
            directory_entries.append(entry)
            print(f"    >>> DIR ENTRY: {entry['name']} ({entry['type']}, {entry['size']})")

        # Verification
        if 'verification: PASS' in line:
            verification_result = 'PASS'
            print("    >>> Content verification: PASS")
        elif 'verification: MISMATCH' in line:
            verification_result = 'MISMATCH'
            print("    >>> Content verification: MISMATCH")

        # Unmount
        if 'unmounted successfully' in line.lower():
            unmount_status = 'SUCCESS'
            print("    >>> SD Card: UNMOUNTED")

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
            print("[INFO] Listening for SD card data... (Ctrl+C to stop)\n")

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

                if unmount_status == 'SUCCESS':
                    time.sleep(1)
                    while ser.in_waiting > 0:
                        line = ser.readline().decode('utf-8', errors='replace')
                        process_line(line)
                    break

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
    print("SD CARD DEBUG SUMMARY")
    print("=" * 60)

    print(f"\n  Mount Status:   {mount_status or 'UNKNOWN'}")
    print(f"  Unmount Status: {unmount_status or 'UNKNOWN'}")

    if card_info:
        print(f"\n  Card Information:")
        for key, val in card_info.items():
            print(f"    {key}: {val}")

    if file_operations:
        print(f"\n  File Operations ({len(file_operations)} total):")
        for op_type, path in file_operations:
            print(f"    [{op_type:6s}] {path}")

    if directory_entries:
        # Deduplicate entries
        seen = set()
        unique_entries = []
        for e in directory_entries:
            if e['name'] not in seen:
                seen.add(e['name'])
                unique_entries.append(e)

        print(f"\n  Directory Contents ({len(unique_entries)} entries):")
        for entry in unique_entries:
            print(f"    {entry['name']:20s} {entry['type']:6s} {entry['size']}")

    if file_contents:
        print(f"\n  Captured File Contents ({len(file_contents)} lines):")
        for line in file_contents[:10]:
            print(f"    {line}")
        if len(file_contents) > 10:
            print(f"    ... and {len(file_contents) - 10} more lines")

    if verification_result:
        print(f"\n  Content Verification: {verification_result}")

    print("=" * 60)


if __name__ == '__main__':
    main()
