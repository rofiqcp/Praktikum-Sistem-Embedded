#!/usr/bin/env python3
"""
==========================================================================
ESP32_06_Sleep_Data_Retention - Debug & Analysis Script
==========================================================================
Modul 14 - Power Management
Program 6: Data persistence analyzer, memory map visualization

Cara penggunaan:
    python3 debug_analysis.py                    # Real-time dari serial
    python3 debug_analysis.py --port /dev/ttyUSB0
    python3 debug_analysis.py --file output.log  # Dari file log
"""

import sys
import re
import time
import argparse
from collections import defaultdict
from datetime import datetime

try:
    import serial
    HAS_SERIAL = True
except ImportError:
    HAS_SERIAL = False
    print("[WARN] pyserial not installed. Install: pip install pyserial")

try:
    import matplotlib.pyplot as plt
    import matplotlib.patches as mpatches
    import numpy as np
    HAS_MATPLOTLIB = True
except ImportError:
    HAS_MATPLOTLIB = False
    print("[WARN] matplotlib not installed. Install: pip install matplotlib")


class DataRetentionAnalyzer:
    """Parser dan analyzer untuk RTC memory data persistence ESP32."""

    def __init__(self):
        self.events = []
        self.memory_states = []
        # Pattern: DATA,boot,sensor,reading_count,cause,regular_var,checksum,noinit,rtc_used
        self.data_pattern = re.compile(
            r'DATA,(\d+),(\d+),(\d+),(\w+),(\d+),(0x[0-9a-fA-F]+),(\d+),(\d+)'
        )

    def parse_line(self, line):
        """Parse satu baris output serial."""
        line = line.strip()
        if not line:
            return

        match = self.data_pattern.search(line)
        if match:
            data = {
                'boot_count': int(match.group(1)),
                'sensor_value': int(match.group(2)),
                'reading_count': int(match.group(3)),
                'wakeup_cause': match.group(4),
                'regular_var': int(match.group(5)),
                'checksum': match.group(6),
                'noinit_counter': int(match.group(7)),
                'rtc_used_bytes': int(match.group(8)),
            }
            self.events.append(data)

            # Verify data persistence
            rtc_ok = "✓" if data['boot_count'] == data['reading_count'] else "?"
            reg_ok = "✓" if data['regular_var'] == 0 else "✗"

            print(f"  [BOOT {data['boot_count']}] "
                  f"Sensor={data['sensor_value']} | "
                  f"RTC_readings={data['reading_count']} {rtc_ok} | "
                  f"Regular={data['regular_var']} {reg_ok} | "
                  f"Noinit={data['noinit_counter']} | "
                  f"RTC used={data['rtc_used_bytes']}B")
            return

        if any(kw in line for kw in ['RTC_MEM', 'RTC_DATA', 'NOINIT', 'Memory', 'PRESERVED', 'LOST']):
            print(f"  [LOG] {line}")

    def calculate_stats(self):
        """Hitung statistik data persistence."""
        if not self.events:
            print("\n[!] Tidak ada data untuk dianalisis.")
            return

        print("\n" + "=" * 60)
        print("  ANALISIS DATA PERSISTENCE")
        print("=" * 60)

        # Verify RTC_DATA_ATTR persistence
        print(f"\n  1. RTC_DATA_ATTR PERSISTENCE TEST:")
        rtc_pass = 0
        rtc_fail = 0
        for e in self.events:
            # boot_count should match reading_count (both in RTC)
            if e['boot_count'] == e['reading_count']:
                rtc_pass += 1
            else:
                rtc_fail += 1
        print(f"     Pass: {rtc_pass} / {len(self.events)}")
        if rtc_fail > 0:
            print(f"     Fail: {rtc_fail} (possible power cycle detected)")
        print(f"     Result: {'PASS ✓' if rtc_fail == 0 else 'MIXED (power cycles detected)'}")

        # Verify regular variable reset
        print(f"\n  2. REGULAR VARIABLE RESET TEST:")
        reg_pass = sum(1 for e in self.events if e['regular_var'] == 0)
        print(f"     Always 0 after deep sleep: {reg_pass} / {len(self.events)}")
        print(f"     Result: {'PASS ✓' if reg_pass == len(self.events) else 'FAIL ✗'}")

        # RTC_NOINIT_ATTR analysis
        print(f"\n  3. RTC_NOINIT_ATTR ANALYSIS:")
        noinit_values = [e['noinit_counter'] for e in self.events]
        print(f"     Noinit counter progression: {noinit_values}")
        if len(noinit_values) > 1:
            monotonic = all(noinit_values[i] <= noinit_values[i+1]
                           for i in range(len(noinit_values)-1))
            print(f"     Monotonically increasing: {'Yes ✓' if monotonic else 'No (power cycle detected)'}")

        # Memory usage
        print(f"\n  4. RTC MEMORY USAGE:")
        last = self.events[-1]
        used = last['rtc_used_bytes']
        total = 8192
        print(f"     Used: {used} bytes")
        print(f"     Available: {total} bytes")
        print(f"     Usage: {used/total*100:.1f}%")
        print(f"     Remaining: {total - used} bytes")

        # Checksums
        print(f"\n  5. DATA INTEGRITY (Checksums):")
        for e in self.events:
            print(f"     Boot {e['boot_count']}: {e['checksum']}")

        # Sensor data
        sensors = [e['sensor_value'] for e in self.events]
        print(f"\n  6. SENSOR DATA COLLECTED:")
        print(f"     Total readings: {len(sensors)}")
        print(f"     Min: {min(sensors)}")
        print(f"     Max: {max(sensors)}")
        print(f"     Avg: {sum(sensors)/len(sensors):.1f}")

    def plot_analysis(self):
        """Buat chart analisis data persistence."""
        if not HAS_MATPLOTLIB or not self.events:
            return

        fig, axes = plt.subplots(2, 2, figsize=(14, 10))
        fig.suptitle('ESP32 RTC Memory Data Retention Analysis', fontsize=14, fontweight='bold')

        boots = [e['boot_count'] for e in self.events]
        sensors = [e['sensor_value'] for e in self.events]
        readings = [e['reading_count'] for e in self.events]
        regulars = [e['regular_var'] for e in self.events]
        noinits = [e['noinit_counter'] for e in self.events]
        rtc_used = [e['rtc_used_bytes'] for e in self.events]

        # Plot 1: Memory type comparison
        x = np.arange(len(boots))
        width = 0.25
        axes[0, 0].bar(x - width, boots, width, label='RTC boot_count', color='#2ecc71', alpha=0.8)
        axes[0, 0].bar(x, regulars, width, label='Regular var', color='#e74c3c', alpha=0.8)
        axes[0, 0].bar(x + width, noinits, width, label='Noinit counter', color='#3498db', alpha=0.8)
        axes[0, 0].set_xlabel('Event Index')
        axes[0, 0].set_ylabel('Value')
        axes[0, 0].set_title('Memory Type Persistence Comparison')
        axes[0, 0].legend()
        axes[0, 0].grid(True, alpha=0.3)

        # Plot 2: Sensor readings collected via RTC
        voltages = [s * 3.3 / 4095 for s in sensors]
        axes[0, 1].plot(boots, voltages, 'b-o', markersize=5, label='Voltage')
        axes[0, 1].fill_between(boots, voltages, alpha=0.2)
        axes[0, 1].set_xlabel('Boot Count')
        axes[0, 1].set_ylabel('Voltage (V)')
        axes[0, 1].set_title('Sensor Readings Stored in RTC Memory')
        axes[0, 1].set_ylim(0, 3.5)
        axes[0, 1].legend()
        axes[0, 1].grid(True, alpha=0.3)

        # Plot 3: RTC memory usage over boots
        axes[1, 0].bar(boots, rtc_used, color='#9b59b6', alpha=0.8)
        axes[1, 0].axhline(y=8192, color='red', linestyle='--', label='Max (8KB)')
        axes[1, 0].set_xlabel('Boot Count')
        axes[1, 0].set_ylabel('Bytes Used')
        axes[1, 0].set_title('RTC Memory Usage')
        axes[1, 0].legend()
        axes[1, 0].grid(True, alpha=0.3)

        # Plot 4: Memory map visualization
        ax = axes[1, 1]
        mem_regions = [
            ('Main RAM\n(520KB)\nLOST on sleep', 520*1024, '#e74c3c'),
            ('RTC FAST\n(8KB)\nPRESERVED', 8*1024, '#2ecc71'),
            ('RTC SLOW\n(8KB)\nULP data', 8*1024, '#3498db'),
        ]

        total_mem = sum(m[1] for m in mem_regions)
        y_pos = 0
        for name, size, color in mem_regions:
            height = size / total_mem * 10
            rect = plt.Rectangle((0.5, y_pos), 3, height, color=color, alpha=0.7)
            ax.add_patch(rect)
            ax.text(2, y_pos + height/2, name, ha='center', va='center',
                   fontsize=8, fontweight='bold')
            ax.text(4, y_pos + height/2, f'{size//1024}KB',
                   ha='left', va='center', fontsize=8)
            y_pos += height

        # Show used RTC
        if self.events:
            last_used = self.events[-1]['rtc_used_bytes']
            ax.text(2, -0.5, f'RTC Used: {last_used}B / 8192B ({last_used/8192*100:.1f}%)',
                   ha='center', fontsize=9, style='italic')

        ax.set_xlim(0, 6)
        ax.set_ylim(-1, y_pos + 0.5)
        ax.set_title('ESP32 Memory Map')
        ax.axis('off')

        plt.tight_layout()
        filename = 'data_retention_analysis.png'
        plt.savefig(filename, dpi=150)
        print(f"\n[OK] Chart disimpan: {filename}")
        plt.show()


def read_serial(port, baudrate, analyzer, duration=120):
    """Baca data dari serial port."""
    if not HAS_SERIAL:
        print("[ERROR] pyserial diperlukan!")
        return

    print(f"[*] Membuka {port} @ {baudrate} baud...")
    try:
        ser = serial.Serial(port, baudrate, timeout=1)
        print(f"[OK] Menunggu data ({duration}s)...")
        end_time = time.time() + duration
        while time.time() < end_time:
            if ser.in_waiting:
                line = ser.readline().decode('utf-8', errors='replace')
                analyzer.parse_line(line)
        ser.close()
    except serial.SerialException as e:
        print(f"[ERROR] Serial error: {e}")


def read_file(filepath, analyzer):
    """Baca data dari file log."""
    print(f"[*] Membaca file: {filepath}")
    try:
        with open(filepath, 'r') as f:
            for line in f:
                analyzer.parse_line(line)
    except FileNotFoundError:
        print(f"[ERROR] File tidak ditemukan: {filepath}")


def main():
    parser = argparse.ArgumentParser(description='ESP32 Data Retention Analyzer')
    parser.add_argument('--port', default='/dev/ttyUSB0')
    parser.add_argument('--baud', type=int, default=115200)
    parser.add_argument('--file', type=str, default=None)
    parser.add_argument('--duration', type=int, default=120)
    args = parser.parse_args()

    analyzer = DataRetentionAnalyzer()

    print("=" * 60)
    print("  ESP32 Data Retention - Debug Analyzer")
    print("=" * 60)

    if args.file:
        read_file(args.file, analyzer)
    else:
        read_serial(args.port, args.baud, analyzer, args.duration)

    analyzer.calculate_stats()
    analyzer.plot_analysis()


if __name__ == '__main__':
    main()
