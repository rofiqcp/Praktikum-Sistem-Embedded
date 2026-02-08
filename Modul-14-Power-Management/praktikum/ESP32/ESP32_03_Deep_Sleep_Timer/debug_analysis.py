#!/usr/bin/env python3
"""
==========================================================================
ESP32_03_Deep_Sleep_Timer - Debug & Analysis Script
==========================================================================
Modul 14 - Power Management
Program 3: Boot cycle tracker, wakeup cause histogram

Cara penggunaan:
    python3 debug_analysis.py                    # Real-time dari serial
    python3 debug_analysis.py --port /dev/ttyUSB0
    python3 debug_analysis.py --file output.log  # Dari file log
"""

import sys
import re
import time
import argparse
from collections import Counter

try:
    import serial
    HAS_SERIAL = True
except ImportError:
    HAS_SERIAL = False
    print("[WARN] pyserial not installed. Install: pip install pyserial")

try:
    import matplotlib.pyplot as plt
    import numpy as np
    HAS_MATPLOTLIB = True
except ImportError:
    HAS_MATPLOTLIB = False
    print("[WARN] matplotlib not installed. Install: pip install matplotlib")


class DeepSleepTimerAnalyzer:
    """Parser dan analyzer untuk data deep sleep timer ESP32."""

    def __init__(self):
        self.boot_events = []
        self.wakeup_causes = Counter()
        # Pattern: DATA,boot_count,sleep_sec,wakeup_cause,heap,active_ms
        self.data_pattern = re.compile(
            r'DATA,(\d+),(\d+),([A-Za-z_ ()\/]+),(\d+),(\d+)'
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
                'sleep_sec': int(match.group(2)),
                'wakeup_cause': match.group(3).strip(),
                'heap_bytes': int(match.group(4)),
                'active_ms': int(match.group(5)),
            }
            self.boot_events.append(data)
            self.wakeup_causes[data['wakeup_cause']] += 1

            print(f"  [BOOT {data['boot_count']}] Cause={data['wakeup_cause']} | "
                  f"Sleep={data['sleep_sec']}s | Active={data['active_ms']}ms | "
                  f"Heap={data['heap_bytes']}")
            return

        if any(kw in line for kw in ['DEEP', 'Boot', 'Wakeup', 'sleep', 'DATA']):
            print(f"  [LOG] {line}")

    def calculate_stats(self):
        """Hitung statistik boot cycles."""
        if not self.boot_events:
            print("\n[!] Tidak ada data boot untuk dianalisis.")
            return

        print("\n" + "=" * 60)
        print("  ANALISIS DEEP SLEEP - TIMER WAKEUP")
        print("=" * 60)

        total_boots = len(self.boot_events)
        total_sleep = sum(e['sleep_sec'] for e in self.boot_events)
        total_active = sum(e['active_ms'] for e in self.boot_events)
        avg_active = total_active / total_boots

        print(f"  Total Boot Cycles    : {total_boots}")
        print(f"  Total Sleep Time     : {total_sleep} seconds")
        print(f"  Total Active Time    : {total_active} ms ({total_active/1000:.1f} s)")
        print(f"  Avg Active per Boot  : {avg_active:.1f} ms")
        print(f"  Sleep Duty Cycle     : {total_sleep/(total_sleep + total_active/1000)*100:.1f}%")

        # Estimasi baterai
        sleep_ratio = total_sleep / (total_sleep + total_active / 1000)
        avg_current_ua = 10 * sleep_ratio + 120000 * (1 - sleep_ratio)
        avg_current_ma = avg_current_ua / 1000

        print(f"\n  ESTIMASI BATERAI (Li-Ion 3.7V, 2000mAh):")
        print(f"  Avg Current          : {avg_current_ma:.2f} mA")
        print(f"  Battery Life Est.    : {2000/avg_current_ma:.0f} hours ({2000/avg_current_ma/24:.0f} days)")

        print(f"\n  WAKEUP CAUSES:")
        for cause, count in self.wakeup_causes.most_common():
            print(f"    {cause}: {count} times ({count/total_boots*100:.0f}%)")

    def plot_analysis(self):
        """Buat chart analisis deep sleep."""
        if not HAS_MATPLOTLIB or not self.boot_events:
            return

        fig, axes = plt.subplots(2, 2, figsize=(14, 10))
        fig.suptitle('ESP32 Deep Sleep Timer Wakeup Analysis', fontsize=14, fontweight='bold')

        boots = [e['boot_count'] for e in self.boot_events]
        active_ms = [e['active_ms'] for e in self.boot_events]
        heap = [e['heap_bytes'] for e in self.boot_events]

        # Plot 1: Active time per boot
        axes[0, 0].bar(boots, active_ms, color='#e74c3c', alpha=0.8)
        axes[0, 0].set_xlabel('Boot Count')
        axes[0, 0].set_ylabel('Active Time (ms)')
        axes[0, 0].set_title('Active Time per Boot Cycle')
        axes[0, 0].grid(True, alpha=0.3)

        # Plot 2: Wakeup cause histogram
        causes = list(self.wakeup_causes.keys())
        counts = list(self.wakeup_causes.values())
        colors = ['#2ecc71', '#3498db', '#e74c3c', '#f39c12', '#9b59b6']
        axes[0, 1].bar(causes, counts, color=colors[:len(causes)], alpha=0.8)
        axes[0, 1].set_ylabel('Count')
        axes[0, 1].set_title('Wakeup Cause Histogram')
        axes[0, 1].grid(True, alpha=0.3)

        # Plot 3: Heap usage across boots
        axes[1, 0].plot(boots, heap, 'b-o', markersize=5)
        axes[1, 0].set_xlabel('Boot Count')
        axes[1, 0].set_ylabel('Free Heap (bytes)')
        axes[1, 0].set_title('Free Heap across Boot Cycles')
        axes[1, 0].grid(True, alpha=0.3)

        # Plot 4: Timeline visualization
        time_blocks = []
        colors_timeline = []
        t = 0
        for e in self.boot_events:
            # Active block
            time_blocks.append((t, e['active_ms'] / 1000))
            colors_timeline.append('#e74c3c')
            t += e['active_ms'] / 1000
            # Sleep block
            time_blocks.append((t, e['sleep_sec']))
            colors_timeline.append('#2ecc71')
            t += e['sleep_sec']

        for (start, dur), color in zip(time_blocks, colors_timeline):
            axes[1, 1].barh(0, dur, left=start, height=0.5, color=color, alpha=0.8)

        axes[1, 1].set_xlabel('Time (seconds)')
        axes[1, 1].set_title('Sleep/Wake Timeline')
        axes[1, 1].set_yticks([])
        from matplotlib.patches import Patch
        axes[1, 1].legend(handles=[
            Patch(color='#e74c3c', label='Active (~120mA)'),
            Patch(color='#2ecc71', label='Deep Sleep (~10µA)')
        ])
        axes[1, 1].grid(True, alpha=0.3)

        plt.tight_layout()
        filename = 'deep_sleep_timer_analysis.png'
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
        print(f"[OK] Menunggu data ({duration}s, deep sleep cycles ~10s each)...")
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
    parser = argparse.ArgumentParser(description='ESP32 Deep Sleep Timer Analyzer')
    parser.add_argument('--port', default='/dev/ttyUSB0')
    parser.add_argument('--baud', type=int, default=115200)
    parser.add_argument('--file', type=str, default=None)
    parser.add_argument('--duration', type=int, default=120)
    args = parser.parse_args()

    analyzer = DeepSleepTimerAnalyzer()

    print("=" * 60)
    print("  ESP32 Deep Sleep Timer - Debug Analyzer")
    print("=" * 60)

    if args.file:
        read_file(args.file, analyzer)
    else:
        read_serial(args.port, args.baud, analyzer, args.duration)

    analyzer.calculate_stats()
    analyzer.plot_analysis()


if __name__ == '__main__':
    main()
