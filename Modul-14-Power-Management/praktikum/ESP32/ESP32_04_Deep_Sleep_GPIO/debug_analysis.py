#!/usr/bin/env python3
"""
==========================================================================
ESP32_04_Deep_Sleep_GPIO - Debug & Analysis Script
==========================================================================
Modul 14 - Power Management
Program 4: GPIO wakeup event tracker, response time analysis

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
from datetime import datetime

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


class GPIOWakeupAnalyzer:
    """Parser dan analyzer untuk GPIO wakeup deep sleep ESP32."""

    def __init__(self):
        self.events = []
        self.wakeup_types = Counter()
        self.timestamps_real = []
        # Pattern: DATA,boot_count,wakeup_cause,ext0_count,ext1_count,timer_count,active_ms
        self.data_pattern = re.compile(
            r'DATA,(\d+),([^,]+),(\d+),(\d+),(\d+),(\d+)'
        )

    def parse_line(self, line):
        """Parse satu baris output serial."""
        line = line.strip()
        if not line:
            return

        match = self.data_pattern.search(line)
        if match:
            data = {
                'timestamp': datetime.now(),
                'boot_count': int(match.group(1)),
                'wakeup_cause': match.group(2).strip(),
                'ext0_total': int(match.group(3)),
                'ext1_total': int(match.group(4)),
                'timer_total': int(match.group(5)),
                'active_ms': int(match.group(6)),
            }
            self.events.append(data)
            self.timestamps_real.append(time.time())
            self.wakeup_types[data['wakeup_cause']] += 1

            print(f"  [BOOT {data['boot_count']}] {data['wakeup_cause']} | "
                  f"Active={data['active_ms']}ms | "
                  f"ext0={data['ext0_total']} ext1={data['ext1_total']} "
                  f"timer={data['timer_total']}")
            return

        if any(kw in line for kw in ['GPIO', 'WAKE', 'ext0', 'ext1', 'wakeup', 'button']):
            print(f"  [LOG] {line}")

    def calculate_stats(self):
        """Hitung statistik GPIO wakeup."""
        if not self.events:
            print("\n[!] Tidak ada data event untuk dianalisis.")
            return

        print("\n" + "=" * 60)
        print("  ANALISIS GPIO WAKEUP")
        print("=" * 60)

        total = len(self.events)
        print(f"  Total Boot Events    : {total}")

        # Response time analysis
        active_times = [e['active_ms'] for e in self.events]
        print(f"  Active Time - Min    : {min(active_times)} ms")
        print(f"  Active Time - Max    : {max(active_times)} ms")
        print(f"  Active Time - Avg    : {sum(active_times)/len(active_times):.1f} ms")

        # Waktu antar boot (dari timestamp real)
        if len(self.timestamps_real) > 1:
            intervals = []
            for i in range(1, len(self.timestamps_real)):
                intervals.append(self.timestamps_real[i] - self.timestamps_real[i-1])
            print(f"\n  Inter-boot Interval:")
            print(f"    Min: {min(intervals):.1f} s")
            print(f"    Max: {max(intervals):.1f} s")
            print(f"    Avg: {sum(intervals)/len(intervals):.1f} s")

        # Wakeup type distribution
        print(f"\n  WAKEUP TYPE DISTRIBUTION:")
        for wtype, count in self.wakeup_types.most_common():
            bar = "█" * (count * 3)
            print(f"    {wtype:30s}: {count:3d} {bar}")

        # Kumulatif dari event terakhir
        last = self.events[-1]
        print(f"\n  CUMULATIVE (from last event):")
        print(f"    ext0 wakeups : {last['ext0_total']}")
        print(f"    ext1 wakeups : {last['ext1_total']}")
        print(f"    timer wakeups: {last['timer_total']}")

    def plot_analysis(self):
        """Buat chart analisis GPIO wakeup."""
        if not HAS_MATPLOTLIB or not self.events:
            return

        fig, axes = plt.subplots(2, 2, figsize=(14, 10))
        fig.suptitle('ESP32 Deep Sleep GPIO Wakeup Analysis', fontsize=14, fontweight='bold')

        boots = [e['boot_count'] for e in self.events]
        active_ms = [e['active_ms'] for e in self.events]

        # Plot 1: Wakeup cause pie chart
        labels = list(self.wakeup_types.keys())
        sizes = list(self.wakeup_types.values())
        colors_pie = ['#2ecc71', '#3498db', '#e74c3c', '#f39c12', '#9b59b6']
        axes[0, 0].pie(sizes, labels=labels, colors=colors_pie[:len(labels)],
                       autopct='%1.0f%%', startangle=90)
        axes[0, 0].set_title('Wakeup Cause Distribution')

        # Plot 2: Active time per boot
        colors_bar = []
        for e in self.events:
            if 'EXT0' in e['wakeup_cause']:
                colors_bar.append('#2ecc71')
            elif 'EXT1' in e['wakeup_cause']:
                colors_bar.append('#3498db')
            elif 'TIMER' in e['wakeup_cause']:
                colors_bar.append('#e74c3c')
            else:
                colors_bar.append('#95a5a6')
        axes[0, 1].bar(boots, active_ms, color=colors_bar, alpha=0.8)
        axes[0, 1].set_xlabel('Boot Count')
        axes[0, 1].set_ylabel('Active Time (ms)')
        axes[0, 1].set_title('Active Time per Boot (colored by cause)')
        axes[0, 1].grid(True, alpha=0.3)

        # Plot 3: Cumulative wakeup counts
        ext0_cum = [e['ext0_total'] for e in self.events]
        ext1_cum = [e['ext1_total'] for e in self.events]
        timer_cum = [e['timer_total'] for e in self.events]
        axes[1, 0].plot(boots, ext0_cum, 'g-o', label='ext0', markersize=4)
        axes[1, 0].plot(boots, ext1_cum, 'b-s', label='ext1', markersize=4)
        axes[1, 0].plot(boots, timer_cum, 'r-^', label='timer', markersize=4)
        axes[1, 0].set_xlabel('Boot Count')
        axes[1, 0].set_ylabel('Cumulative Count')
        axes[1, 0].set_title('Cumulative Wakeup Counts')
        axes[1, 0].legend()
        axes[1, 0].grid(True, alpha=0.3)

        # Plot 4: Inter-boot intervals
        if len(self.timestamps_real) > 1:
            intervals = []
            for i in range(1, len(self.timestamps_real)):
                intervals.append(self.timestamps_real[i] - self.timestamps_real[i-1])
            axes[1, 1].plot(range(2, len(self.timestamps_real) + 1), intervals,
                           'm-o', markersize=4)
            axes[1, 1].set_xlabel('Boot Count')
            axes[1, 1].set_ylabel('Interval (seconds)')
            axes[1, 1].set_title('Inter-boot Time Intervals')
            axes[1, 1].grid(True, alpha=0.3)
        else:
            axes[1, 1].text(0.5, 0.5, 'Not enough data\nfor interval analysis',
                           ha='center', va='center', fontsize=12)
            axes[1, 1].set_title('Inter-boot Time Intervals')

        plt.tight_layout()
        filename = 'gpio_wakeup_analysis.png'
        plt.savefig(filename, dpi=150)
        print(f"\n[OK] Chart disimpan: {filename}")
        plt.show()


def read_serial(port, baudrate, analyzer, duration=180):
    """Baca data dari serial port."""
    if not HAS_SERIAL:
        print("[ERROR] pyserial diperlukan!")
        return

    print(f"[*] Membuka {port} @ {baudrate} baud...")
    print("[*] Tekan button pada ESP32 untuk trigger wakeup!")
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
    parser = argparse.ArgumentParser(description='ESP32 GPIO Wakeup Analyzer')
    parser.add_argument('--port', default='/dev/ttyUSB0')
    parser.add_argument('--baud', type=int, default=115200)
    parser.add_argument('--file', type=str, default=None)
    parser.add_argument('--duration', type=int, default=180)
    args = parser.parse_args()

    analyzer = GPIOWakeupAnalyzer()

    print("=" * 60)
    print("  ESP32 GPIO Wakeup - Debug Analyzer")
    print("=" * 60)

    if args.file:
        read_file(args.file, analyzer)
    else:
        read_serial(args.port, args.baud, analyzer, args.duration)

    analyzer.calculate_stats()
    analyzer.plot_analysis()


if __name__ == '__main__':
    main()
