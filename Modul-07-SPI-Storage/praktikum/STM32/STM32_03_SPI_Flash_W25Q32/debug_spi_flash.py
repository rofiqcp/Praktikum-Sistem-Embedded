"""
============================================================
 Debug Script - STM32 SPI Flash W25Q32
 Modul 07 - SPI & Storage
============================================================
 Parse JEDEC ID, track operations, verify data
 Usage: python debug_spi_flash.py [PORT] [BAUD]
============================================================
"""

import serial
import sys
import re
import time
from collections import defaultdict

try:
    import matplotlib
    matplotlib.use('Agg')
    import matplotlib.pyplot as plt
    HAS_MATPLOTLIB = True
except ImportError:
    HAS_MATPLOTLIB = False

# ----- Configuration -----
PORT = sys.argv[1] if len(sys.argv) > 1 else '/dev/ttyUSB0'
BAUD = int(sys.argv[2]) if len(sys.argv) > 2 else 115200

# ----- Known Flash IDs -----
KNOWN_MANUFACTURERS = {
    0xEF: "Winbond",
    0xC8: "GigaDevice",
    0x20: "Micron/Numonyx",
    0x01: "Spansion/Cypress",
    0xBF: "Microchip/SST",
    0x1F: "Adesto/Atmel",
}

KNOWN_CAPACITIES = {
    0x14: "8Mbit (1MB)",
    0x15: "16Mbit (2MB)",
    0x16: "32Mbit (4MB)",
    0x17: "64Mbit (8MB)",
    0x18: "128Mbit (16MB)",
}


def parse_jedec_id(line):
    """Parse JEDEC ID from serial output"""
    match = re.search(r'JEDEC ID:\s*0x([0-9A-Fa-f]{2})\s+0x([0-9A-Fa-f]{2})\s+0x([0-9A-Fa-f]{2})', line)
    if match:
        mfg = int(match.group(1), 16)
        mem_type = int(match.group(2), 16)
        capacity = int(match.group(3), 16)
        return mfg, mem_type, capacity
    return None


def analyze_jedec(mfg, mem_type, capacity):
    """Analyze JEDEC ID"""
    print("\n" + "=" * 50)
    print("  JEDEC ID Analysis")
    print("=" * 50)

    mfg_name = KNOWN_MANUFACTURERS.get(mfg, "Unknown")
    cap_name = KNOWN_CAPACITIES.get(capacity, "Unknown")

    print(f"  Manufacturer : 0x{mfg:02X} - {mfg_name}")
    print(f"  Memory Type  : 0x{mem_type:02X}")
    print(f"  Capacity     : 0x{capacity:02X} - {cap_name}")

    if mfg == 0xEF and capacity == 0x16:
        print(f"\n  [OK] Confirmed W25Q32 (Winbond 32Mbit)")
    elif mfg == 0x00 or mfg == 0xFF:
        print(f"\n  [ERROR] Invalid JEDEC ID - Check wiring!")
    else:
        print(f"\n  [INFO] Non-standard but valid flash detected")

    print("=" * 50)


def main():
    print(f"[INFO] Connecting to {PORT} at {BAUD} baud...")

    try:
        ser = serial.Serial(PORT, BAUD, timeout=1.0)
    except serial.SerialException as e:
        print(f"[ERROR] Cannot open {PORT}: {e}")
        print("[INFO] Running in demo mode...")
        analyze_jedec(0xEF, 0x40, 0x16)
        return

    print("[INFO] Listening for flash data...")
    print("[INFO] Press Ctrl+C to stop\n")

    operations = defaultdict(int)
    timing_data = {}
    results = {}

    try:
        while True:
            line = ser.readline().decode('utf-8', errors='replace').strip()
            if not line:
                continue

            print(f"  >> {line}")

            # Parse JEDEC ID
            jedec = parse_jedec_id(line)
            if jedec:
                analyze_jedec(*jedec)

            # Track operations
            if 'Erasing sector' in line:
                operations['erase'] += 1
            elif 'Programming' in line:
                operations['write'] += 1
            elif 'Reading back' in line:
                operations['read'] += 1

            # Parse timing
            erase_match = re.search(r'Erase time:\s*(\d+)\s*ms', line)
            if erase_match:
                timing_data['erase'] = int(erase_match.group(1))

            write_match = re.search(r'Write time:\s*(\d+)\s*ms', line)
            if write_match:
                timing_data['write'] = int(write_match.group(1))

            # Parse results
            for test in ['JEDEC ID', 'Sector Erase', 'Page Write', 'Read Verify', 'Power Cycle']:
                if test in line:
                    if 'PASS' in line:
                        results[test] = 'PASS'
                    elif 'FAIL' in line:
                        results[test] = 'FAIL'

    except KeyboardInterrupt:
        print("\n\n[INFO] Stopped by user.")
    finally:
        ser.close()

        print("\n" + "=" * 50)
        print("  FLASH OPERATIONS SUMMARY")
        print("=" * 50)
        for op, count in operations.items():
            t = timing_data.get(op, '?')
            print(f"  {op:>10s}: {count} operations ({t} ms avg)")

        if results:
            print(f"\n  TEST RESULTS:")
            for test, result in results.items():
                icon = "PASS" if result == 'PASS' else "FAIL"
                print(f"    {test:<20s}: {icon}")
        print("=" * 50)


if __name__ == '__main__':
    main()
