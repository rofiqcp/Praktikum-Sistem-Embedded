"""
============================================================
 Debug Script - STM32 SPI SD Card
 Modul 07 - SPI & Storage
============================================================
 Parse init sequence, track block operations
 Usage: python debug_sd_card.py [PORT] [BAUD]
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

# ----- SD Card Command Names -----
SD_COMMANDS = {
    0:  "CMD0  - GO_IDLE_STATE",
    1:  "CMD1  - SEND_OP_COND (MMC)",
    8:  "CMD8  - SEND_IF_COND",
    9:  "CMD9  - SEND_CSD",
    10: "CMD10 - SEND_CID",
    16: "CMD16 - SET_BLOCKLEN",
    17: "CMD17 - READ_SINGLE_BLOCK",
    24: "CMD24 - WRITE_BLOCK",
    55: "CMD55 - APP_CMD",
    58: "CMD58 - READ_OCR",
}

SD_CARD_TYPES = {
    0: "Unknown",
    1: "MMC",
    2: "SDSC (Standard Capacity)",
    3: "SDHC (High Capacity)",
}


def main():
    print(f"[INFO] Connecting to {PORT} at {BAUD} baud...")

    try:
        ser = serial.Serial(PORT, BAUD, timeout=1.0)
    except serial.SerialException as e:
        print(f"[ERROR] Cannot open {PORT}: {e}")
        print("[INFO] Running in demo mode...")

        # Demo analysis
        print("\n" + "=" * 55)
        print("  SD Card Init Sequence (Expected)")
        print("=" * 55)
        sequence = [
            ("Power Up", "80 dummy clocks, CS HIGH"),
            ("CMD0", "GO_IDLE_STATE -> R1=0x01"),
            ("CMD8", "SEND_IF_COND(0x1AA) -> R7 response"),
            ("ACMD41", "SD_SEND_OP_COND(HCS=1) -> R1=0x00"),
            ("CMD58", "READ_OCR -> Check CCS bit"),
        ]
        for step, desc in sequence:
            print(f"  {step:<10s}: {desc}")
        print("=" * 55)
        return

    print("[INFO] Listening for SD card data...")
    print("[INFO] Press Ctrl+C to stop\n")

    init_steps = []
    block_ops = []
    card_type = "Unknown"
    timing_data = {}

    try:
        while True:
            line = ser.readline().decode('utf-8', errors='replace').strip()
            if not line:
                continue

            print(f"  >> {line}")

            # Track init steps
            for step in ['CMD0', 'CMD8', 'ACMD41', 'CMD58', 'CMD16', 'CMD1']:
                if step in line and 'Step' in line:
                    init_steps.append(step)

            # Parse card type
            type_match = re.search(r'Card [Tt]ype:\s*(SDHC|SDSC|MMC)', line)
            if type_match:
                card_type = type_match.group(1)

            # Parse timing
            read_match = re.search(r'Read OK \((\d+) ms\)', line)
            if read_match:
                timing_data.setdefault('read_ms', []).append(int(read_match.group(1)))

            write_match = re.search(r'Write OK \((\d+) ms\)', line)
            if write_match:
                timing_data.setdefault('write_ms', []).append(int(write_match.group(1)))

            # Parse block operations
            block_match = re.search(r'(Reading|Writing)\s+.*block\s+(\d+)', line)
            if block_match:
                block_ops.append({
                    'op': block_match.group(1),
                    'block': int(block_match.group(2))
                })

            # Parse verification
            if 'ALL TESTS PASSED' in line:
                print("\n  [OK] SD Card test PASSED!")
            elif 'SOME TESTS FAILED' in line:
                print("\n  [FAIL] SD Card test had failures!")

    except KeyboardInterrupt:
        print("\n\n[INFO] Stopped by user.")
    finally:
        ser.close()

        # Print analysis
        print("\n" + "=" * 55)
        print("  SD CARD DEBUG ANALYSIS")
        print("=" * 55)
        print(f"  Card Type    : {card_type}")
        print(f"  Init Steps   : {' -> '.join(init_steps) if init_steps else 'N/A'}")
        print(f"  Block Ops    : {len(block_ops)}")

        if timing_data.get('read_ms'):
            avg_read = sum(timing_data['read_ms']) / len(timing_data['read_ms'])
            print(f"  Avg Read Time: {avg_read:.1f} ms")

        if timing_data.get('write_ms'):
            avg_write = sum(timing_data['write_ms']) / len(timing_data['write_ms'])
            print(f"  Avg Write Time: {avg_write:.1f} ms")

        print("=" * 55)

        # Plot timing if available
        if HAS_MATPLOTLIB and (timing_data.get('read_ms') or timing_data.get('write_ms')):
            fig, ax = plt.subplots(figsize=(10, 5))

            if timing_data.get('read_ms'):
                ax.bar(['Read'] * len(timing_data['read_ms']),
                       timing_data['read_ms'], color='#3498db', alpha=0.7, label='Read')
            if timing_data.get('write_ms'):
                ax.bar(['Write'] * len(timing_data['write_ms']),
                       timing_data['write_ms'], color='#e74c3c', alpha=0.7, label='Write')

            categories = []
            values = []
            if timing_data.get('read_ms'):
                categories.append('Read')
                values.append(sum(timing_data['read_ms']) / len(timing_data['read_ms']))
            if timing_data.get('write_ms'):
                categories.append('Write')
                values.append(sum(timing_data['write_ms']) / len(timing_data['write_ms']))

            ax.bar(categories, values, color=['#3498db', '#e74c3c'][:len(categories)])
            ax.set_ylabel('Time (ms)')
            ax.set_title('SD Card Block Operation Timing')

            plt.tight_layout()
            filename = 'sd_card_timing.png'
            plt.savefig(filename, dpi=150)
            print(f"\n[INFO] Chart saved to {filename}")
            plt.close()


if __name__ == '__main__':
    main()
