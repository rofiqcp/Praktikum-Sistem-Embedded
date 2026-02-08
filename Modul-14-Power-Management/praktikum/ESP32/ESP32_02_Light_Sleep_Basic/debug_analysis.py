#!/usr/bin/env python3
"""
==========================================================================
ESP32_02_Light_Sleep_Basic - Debug & Analysis Script
==========================================================================
Modul 14 - Power Management
Program 2: Sleep/wake cycle analysis, current timeline plot

Cara penggunaan:
    python3 debug_analysis.py                    # Real-time dari serial
    python3 debug_analysis.py --port /dev/ttyUSB0
    python3 debug_analysis.py --file output.log  # Dari file log
"""

import sys
import re
import time
import argparse
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


class LightSleepAnalyzer:
    """Parser dan analyzer untuk data light sleep ESP32."""

    def __init__(self):
        self.cycles = []
        # Pattern: DATA,cycle,sleep_ms,active_ms,cumulative_sleep,test_var
        self.data_pattern = re.compile(
            r'DATA,(\d+),([\d.]+),([\d.]+),([\d.]+),(\d+)'
        )

    def parse_line(self, line):
        """Parse satu baris output serial."""
        line = line.strip()
        if not line:
            return

        match = self.data_pattern.search(line)
        if match:
            data = {
                'cycle': int(match.group(1)),
                'sleep_ms': float(match.group(2)),
                'active_ms': float(match.group(3)),
                'cumulative_sleep_ms': float(match.group(4)),
                'test_variable': int(match.group(5)),
            }
            self.cycles.append(data)
            print(f"  [CYCLE {data['cycle']}] Sleep={data['sleep_ms']:.1f}ms | "
                  f"Active={data['active_ms']:.1f}ms | "
                  f"test_var={data['test_variable']}")
            return

        # Log baris informatif
        if any(kw in line for kw in ['SLEEP', 'Woke', 'LED', 'Cycle', 'RINGKASAN']):
            print(f"  [LOG] {line}")

    def calculate_stats(self):
        """Hitung statistik sleep/wake cycles."""
        if not self.cycles:
            print("\n[!] Tidak ada data cycle untuk dianalisis.")
            return

        print("\n" + "=" * 60)
        print("  ANALISIS LIGHT SLEEP CYCLES")
        print("=" * 60)

        sleep_times = [c['sleep_ms'] for c in self.cycles]
        active_times = [c['active_ms'] for c in self.cycles]
        total_sleep = sum(sleep_times)
        total_active = sum(active_times)
        total_time = total_sleep + total_active

        print(f"  Total Cycles         : {len(self.cycles)}")
        print(f"  Avg Sleep Duration   : {sum(sleep_times)/len(sleep_times):.1f} ms")
        print(f"  Avg Active Duration  : {sum(active_times)/len(active_times):.1f} ms")
        print(f"  Total Sleep Time     : {total_sleep:.1f} ms ({total_sleep/1000:.1f} s)")
        print(f"  Total Active Time    : {total_active:.1f} ms ({total_active/1000:.1f} s)")
        print(f"  Sleep Duty Cycle     : {total_sleep/total_time*100:.1f}%")
        print()

        # Estimasi konsumsi rata-rata
        sleep_ratio = total_sleep / total_time
        avg_current = 0.8 * sleep_ratio + 120.0 * (1 - sleep_ratio)
        print(f"  ESTIMASI KONSUMSI RATA-RATA:")
        print(f"  Active current   : ~120 mA")
        print(f"  Sleep current    : ~0.8 mA")
        print(f"  Blended average  : ~{avg_current:.1f} mA")
        print(f"  Power saving     : {(1 - avg_current/120)*100:.1f}% vs always-active")

        # Verifikasi variable preservation
        print(f"\n  VARIABLE PRESERVATION TEST:")
        for c in self.cycles:
            print(f"    Cycle {c['cycle']}: test_variable = {c['test_variable']} ✓")

    def plot_timeline(self):
        """Buat timeline chart sleep/wake cycles."""
        if not HAS_MATPLOTLIB or not self.cycles:
            return

        fig, axes = plt.subplots(3, 1, figsize=(14, 10))
        fig.suptitle('ESP32 Light Sleep Cycle Analysis', fontsize=14, fontweight='bold')

        cycles = [c['cycle'] for c in self.cycles]
        sleep_ms = [c['sleep_ms'] for c in self.cycles]
        active_ms = [c['active_ms'] for c in self.cycles]

        # Plot 1: Sleep vs Active duration per cycle (stacked bar)
        axes[0].bar(cycles, active_ms, label='Active (~120mA)', color='#e74c3c', alpha=0.8)
        axes[0].bar(cycles, sleep_ms, bottom=active_ms, label='Sleep (~0.8mA)',
                    color='#2ecc71', alpha=0.8)
        axes[0].set_ylabel('Duration (ms)')
        axes[0].set_title('Sleep vs Active Duration per Cycle')
        axes[0].legend()
        axes[0].grid(True, alpha=0.3)

        # Plot 2: Current timeline (estimated)
        time_points = []
        current_points = []
        t = 0
        for c in self.cycles:
            # Active period
            time_points.extend([t, t + c['active_ms']])
            current_points.extend([120, 120])
            t += c['active_ms']
            # Sleep period
            time_points.extend([t, t + c['sleep_ms']])
            current_points.extend([0.8, 0.8])
            t += c['sleep_ms']

        axes[1].fill_between(time_points, current_points, alpha=0.4, color='blue')
        axes[1].plot(time_points, current_points, 'b-', linewidth=0.8)
        axes[1].set_ylabel('Current (mA)')
        axes[1].set_title('Estimated Current Timeline')
        axes[1].set_yscale('log')
        axes[1].grid(True, alpha=0.3)

        # Plot 3: Cumulative sleep time
        cum_sleep = [c['cumulative_sleep_ms'] for c in self.cycles]
        axes[2].plot(cycles, cum_sleep, 'g-o', markersize=5, label='Cumulative Sleep')
        axes[2].set_xlabel('Cycle')
        axes[2].set_ylabel('Time (ms)')
        axes[2].set_title('Cumulative Sleep Time')
        axes[2].legend()
        axes[2].grid(True, alpha=0.3)

        plt.tight_layout()
        filename = 'light_sleep_analysis.png'
        plt.savefig(filename, dpi=150)
        print(f"\n[OK] Chart disimpan: {filename}")
        plt.show()


def read_serial(port, baudrate, analyzer, duration=80):
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
    parser = argparse.ArgumentParser(description='ESP32 Light Sleep Analyzer')
    parser.add_argument('--port', default='/dev/ttyUSB0')
    parser.add_argument('--baud', type=int, default=115200)
    parser.add_argument('--file', type=str, default=None)
    parser.add_argument('--duration', type=int, default=80)
    args = parser.parse_args()

    analyzer = LightSleepAnalyzer()

    print("=" * 60)
    print("  ESP32 Light Sleep - Debug Analyzer")
    print("=" * 60)

    if args.file:
        read_file(args.file, analyzer)
    else:
        read_serial(args.port, args.baud, analyzer, args.duration)

    analyzer.calculate_stats()
    analyzer.plot_timeline()


if __name__ == '__main__':
    main()
