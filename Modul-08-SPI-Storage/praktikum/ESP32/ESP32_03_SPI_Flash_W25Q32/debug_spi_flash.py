#!/usr/bin/env python3
"""
SPI Flash W25Q32 Debug Script
================================
Reads serial output from ESP32 SPI flash program,
parses JEDEC ID, read/write operations, and verifies
data integrity from log output.

Usage:
    python debug_spi_flash.py [--port /dev/ttyUSB0] [--baud 115200]
"""

import serial
import re
import sys
import argparse
import time


def parse_jedec_id(line):
    """Parse JEDEC ID from log line."""
    match = re.search(
        r'Manufacturer=0x([0-9A-Fa-f]{2}),\s*MemType=0x([0-9A-Fa-f]{2}),\s*Capacity=0x([0-9A-Fa-f]{2})',
        line
    )
    if match:
        return {
            'manufacturer': int(match.group(1), 16),
            'mem_type': int(match.group(2), 16),
            'capacity': int(match.group(3), 16),
        }
    return None


def parse_hex_dump_line(line):
    """Parse a hex dump line and return address and bytes."""
    match = re.search(r'([0-9A-Fa-f]{4}):\s*((?:[0-9A-Fa-f]{2}\s)+)', line)
    if match:
        addr = int(match.group(1), 16)
        hex_bytes = [int(b, 16) for b in match.group(2).strip().split()]
        return addr, hex_bytes
    return None, None


def main():
    parser = argparse.ArgumentParser(description='SPI Flash W25Q32 Debug Monitor')
    parser.add_argument('--port', default='/dev/ttyUSB0', help='Serial port')
    parser.add_argument('--baud', type=int, default=115200, help='Baud rate')
    parser.add_argument('--timeout', type=int, default=30, help='Timeout in seconds')
    parser.add_argument('--file', type=str, help='Read from log file instead of serial')
    args = parser.parse_args()

    jedec_info = None
    write_data = {}
    read_data = {}
    current_section = None
    operations = []
    verification_result = None

    def process_line(line):
        nonlocal jedec_info, current_section, verification_result

        line = line.strip()
        if not line:
            return

        print(f"[SERIAL] {line}")

        # Parse JEDEC ID
        jid = parse_jedec_id(line)
        if jid:
            jedec_info = jid
            mfr = jedec_info['manufacturer']
            mt = jedec_info['mem_type']
            cap = jedec_info['capacity']
            print(f"\n>>> JEDEC ID Detected:")
            print(f"    Manufacturer: 0x{mfr:02X} ({'Winbond' if mfr == 0xEF else 'Unknown'})")
            print(f"    Memory Type:  0x{mt:02X}")
            print(f"    Capacity:     0x{cap:02X}")

            # Decode capacity
            if cap == 0x16:
                print(f"    Device: W25Q32 (32Mbit / 4MB)")
            elif cap == 0x17:
                print(f"    Device: W25Q64 (64Mbit / 8MB)")
            elif cap == 0x18:
                print(f"    Device: W25Q128 (128Mbit / 16MB)")
            else:
                print(f"    Device: Unknown (capacity code 0x{cap:02X})")

        # Track sections
        if 'Write data' in line or 'Write Data' in line:
            current_section = 'write'
        elif 'Read data' in line or 'Read Data' in line or 'Read Back' in line:
            current_section = 'read'
        elif 'After erase' in line:
            current_section = 'erase_verify'

        # Parse hex dump lines
        addr, hex_bytes = parse_hex_dump_line(line)
        if addr is not None and hex_bytes:
            if current_section == 'write':
                for i, b in enumerate(hex_bytes):
                    write_data[addr + i] = b
            elif current_section == 'read':
                for i, b in enumerate(hex_bytes):
                    read_data[addr + i] = b

        # Track operations
        if 'Erasing sector' in line:
            addr_match = re.search(r'0x([0-9A-Fa-f]+)', line)
            if addr_match:
                operations.append(('ERASE', int(addr_match.group(1), 16)))
                print(f"    >>> Operation: SECTOR ERASE at 0x{addr_match.group(1)}")

        if 'Programming' in line:
            match = re.search(r'(\d+)\s*bytes.*0x([0-9A-Fa-f]+)', line)
            if match:
                operations.append(('WRITE', int(match.group(2), 16), int(match.group(1))))
                print(f"    >>> Operation: WRITE {match.group(1)} bytes at 0x{match.group(2)}")

        if 'Reading' in line and 'bytes from' in line:
            match = re.search(r'(\d+)\s*bytes.*0x([0-9A-Fa-f]+)', line)
            if match:
                operations.append(('READ', int(match.group(2), 16), int(match.group(1))))
                print(f"    >>> Operation: READ {match.group(1)} bytes from 0x{match.group(2)}")

        # Detect verification result
        if 'VERIFICATION: PASS' in line:
            verification_result = 'PASS'
            print(f"\n>>> DATA VERIFICATION: PASS")
        elif 'VERIFICATION: FAIL' in line:
            verification_result = 'FAIL'
            print(f"\n>>> DATA VERIFICATION: FAIL")

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
            print("[INFO] Listening for W25Q32 flash data... (Ctrl+C to stop)\n")

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

                if verification_result is not None:
                    time.sleep(1)  # Wait for remaining output
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
    print("W25Q32 FLASH DEBUG SUMMARY")
    print("=" * 60)

    if jedec_info:
        print(f"  JEDEC ID: 0x{jedec_info['manufacturer']:02X} "
              f"0x{jedec_info['mem_type']:02X} "
              f"0x{jedec_info['capacity']:02X}")
        expected = (jedec_info['manufacturer'] == 0xEF and
                    jedec_info['mem_type'] == 0x40 and
                    jedec_info['capacity'] == 0x16)
        print(f"  ID Match (W25Q32): {'YES' if expected else 'NO'}")
    else:
        print("  JEDEC ID: NOT DETECTED")

    print(f"\n  Operations performed:")
    for op in operations:
        if op[0] == 'ERASE':
            print(f"    - SECTOR ERASE at 0x{op[1]:06X}")
        elif op[0] == 'WRITE':
            print(f"    - WRITE {op[2]} bytes at 0x{op[1]:06X}")
        elif op[0] == 'READ':
            print(f"    - READ {op[2]} bytes from 0x{op[1]:06X}")

    # Verify data integrity from captured hex dumps
    if write_data and read_data:
        print(f"\n  Data integrity check (from hex dumps):")
        common_addrs = set(write_data.keys()) & set(read_data.keys())
        mismatches = sum(1 for a in common_addrs if write_data[a] != read_data[a])
        print(f"    Compared bytes: {len(common_addrs)}")
        print(f"    Mismatches: {mismatches}")
        print(f"    Result: {'PASS' if mismatches == 0 else 'FAIL'}")

    if verification_result:
        print(f"\n  ESP32 Verification: {verification_result}")

    print("=" * 60)


if __name__ == '__main__':
    main()
