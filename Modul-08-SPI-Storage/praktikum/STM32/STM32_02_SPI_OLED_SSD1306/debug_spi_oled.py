"""
============================================================
 Debug Script - STM32 SPI OLED SSD1306
 Modul 07 - SPI & Storage
============================================================
 Parse SSD1306 init commands from serial, verify sequence
 Usage: python debug_spi_oled.py [PORT] [BAUD]
============================================================
"""

import serial
import sys
import re
import time

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

# ----- SSD1306 Expected Init Sequence -----
EXPECTED_INIT_CMDS = {
    0xAE: "Display OFF",
    0xD5: "Set Clock Divide",
    0xA8: "Set MUX Ratio",
    0xD3: "Set Display Offset",
    0x40: "Set Start Line",
    0x8D: "Charge Pump Setting",
    0x20: "Set Memory Mode",
    0xA1: "Segment Remap",
    0xC8: "COM Scan Decrement",
    0xDA: "Set COM Pins Config",
    0x81: "Set Contrast",
    0xD9: "Set Precharge Period",
    0xDB: "Set VCOMH Deselect",
    0xA4: "Entire Display ON (Resume)",
    0xA6: "Normal Display",
    0xAF: "Display ON",
}


def parse_init_commands(line):
    """Parse init command hex string from serial"""
    match = re.search(r'Init commands sent:\s*([0-9A-Fa-f,]+)', line)
    if match:
        hex_str = match.group(1)
        cmds = [int(h, 16) for h in hex_str.split(',')]
        return cmds
    return None


def verify_init_sequence(cmds):
    """Verify SSD1306 init command sequence"""
    print("\n" + "=" * 60)
    print("  SSD1306 Init Command Verification")
    print("=" * 60)

    found = set()
    for cmd in cmds:
        if cmd in EXPECTED_INIT_CMDS:
            found.add(cmd)
            print(f"  [OK]   0x{cmd:02X} - {EXPECTED_INIT_CMDS[cmd]}")
        else:
            # Could be a data byte (parameter), skip
            pass

    missing = set(EXPECTED_INIT_CMDS.keys()) - found
    if missing:
        print(f"\n  [WARN] Missing commands:")
        for cmd in sorted(missing):
            print(f"         0x{cmd:02X} - {EXPECTED_INIT_CMDS[cmd]}")
    else:
        print(f"\n  [OK] All {len(EXPECTED_INIT_CMDS)} essential commands found!")

    print("=" * 60)
    return len(missing) == 0


def plot_counter_data(counter_data, time_data):
    """Plot counter vs time"""
    if not HAS_MATPLOTLIB or len(counter_data) < 2:
        return

    fig, ax = plt.subplots(figsize=(10, 5))
    ax.plot(time_data, counter_data, 'b-o', markersize=3, label='Counter')
    ax.set_xlabel('Uptime (seconds)')
    ax.set_ylabel('Counter Value')
    ax.set_title('SSD1306 OLED Counter Progress')
    ax.legend()
    ax.grid(True, alpha=0.3)

    plt.tight_layout()
    filename = 'ssd1306_counter_plot.png'
    plt.savefig(filename, dpi=150)
    print(f"\n[INFO] Chart saved to {filename}")
    plt.close()


def main():
    print(f"[INFO] Connecting to {PORT} at {BAUD} baud...")

    try:
        ser = serial.Serial(PORT, BAUD, timeout=1.0)
    except serial.SerialException as e:
        print(f"[ERROR] Cannot open {PORT}: {e}")
        print("[INFO] Running in demo mode...")

        # Demo verification
        demo_cmds = [0xAE, 0xD5, 0x80, 0xA8, 0x3F, 0xD3, 0x00, 0x40,
                     0x8D, 0x14, 0x20, 0x00, 0xA1, 0xC8, 0xDA, 0x12,
                     0x81, 0xCF, 0xD9, 0xF1, 0xDB, 0x40, 0xA4, 0xA6, 0xAF]
        verify_init_sequence(demo_cmds)
        return

    print("[INFO] Listening for OLED debug data...")
    print("[INFO] Press Ctrl+C to stop\n")

    counter_data = []
    time_data = []
    init_verified = False

    try:
        while True:
            line = ser.readline().decode('utf-8', errors='replace').strip()
            if not line:
                continue

            print(f"  >> {line}")

            # Parse init commands
            cmds = parse_init_commands(line)
            if cmds and not init_verified:
                init_verified = verify_init_sequence(cmds)

            # Parse counter data
            counter_match = re.search(r'Counter=(\d+),\s*Uptime=(\d+)', line)
            if counter_match:
                counter_data.append(int(counter_match.group(1)))
                time_data.append(int(counter_match.group(2)))

    except KeyboardInterrupt:
        print("\n\n[INFO] Stopped by user.")
    finally:
        ser.close()
        if counter_data:
            plot_counter_data(counter_data, time_data)
            print(f"\n[INFO] Captured {len(counter_data)} counter readings")
            print(f"[INFO] Counter range: {min(counter_data)} - {max(counter_data)}")


if __name__ == '__main__':
    main()
