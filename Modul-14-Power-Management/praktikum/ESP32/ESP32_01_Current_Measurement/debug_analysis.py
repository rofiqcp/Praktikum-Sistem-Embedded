#!/usr/bin/env python3
"""
==========================================================================
ESP32_01_Current_Measurement - Debug & Analysis Script
==========================================================================
Modul 14 - Power Management
Program 1: Parse serial data, calculate average power, create power profile

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
from collections import defaultdict

try:
    import serial
    HAS_SERIAL = True
except ImportError:
    HAS_SERIAL = False
    print("[WARN] pyserial not installed. Install: pip install pyserial")

try:
    import matplotlib.pyplot as plt
    import matplotlib.dates as mdates
    HAS_MATPLOTLIB = True
except ImportError:
    HAS_MATPLOTLIB = False
    print("[WARN] matplotlib not installed. Install: pip install matplotlib")


class CurrentMeasurementAnalyzer:
    """Parser dan analyzer untuk data pengukuran arus ESP32."""

    def __init__(self):
        self.data_points = []
        self.cpu_freq_history = []
        self.heap_history = []
        self.toggle_history = []
        self.timestamps = []
        # Pattern: DATA,freq,uptime,heap,toggles,current
        self.data_pattern = re.compile(
            r'DATA,(\d+),([\d.]+),(\d+),(\d+),([\d.]+)'
        )
        self.start_time = time.time()

    def parse_line(self, line):
        """Parse satu baris output serial."""
        line = line.strip()
        if not line:
            return

        # Cari data terformat
        match = self.data_pattern.search(line)
        if match:
            cpu_freq = int(match.group(1))
            uptime = float(match.group(2))
            heap = int(match.group(3))
            toggles = int(match.group(4))
            est_current = float(match.group(5))

            data = {
                'timestamp': datetime.now(),
                'cpu_freq_mhz': cpu_freq,
                'uptime_s': uptime,
                'heap_bytes': heap,
                'led_toggles': toggles,
                'est_current_ma': est_current,
                'est_power_mw': est_current * 3.3,
            }
            self.data_points.append(data)
            self.timestamps.append(uptime)
            self.cpu_freq_history.append(cpu_freq)
            self.heap_history.append(heap)
            self.toggle_history.append(toggles)

            print(f"  [DATA] t={uptime:.1f}s | CPU={cpu_freq}MHz | "
                  f"Heap={heap} | Toggles={toggles} | "
                  f"~{est_current:.0f}mA ({est_current*3.3:.0f}mW)")
            return

        # Print baris lain yang mengandung info berguna
        if any(kw in line for kw in ['POWER', 'CPU', 'Heap', 'PENGUKURAN']):
            print(f"  [LOG] {line}")

    def calculate_stats(self):
        """Hitung statistik rata-rata dari data yang terkumpul."""
        if not self.data_points:
            print("\n[!] Tidak ada data untuk dianalisis.")
            return

        print("\n" + "=" * 60)
        print("  ANALISIS PENGUKURAN ARUS - RINGKASAN")
        print("=" * 60)

        # Rata-rata estimasi arus
        avg_current = sum(d['est_current_ma'] for d in self.data_points) / len(self.data_points)
        avg_power = avg_current * 3.3
        total_time = self.data_points[-1]['uptime_s'] - self.data_points[0]['uptime_s']

        print(f"  Total Data Points  : {len(self.data_points)}")
        print(f"  Durasi Pengukuran  : {total_time:.1f} detik")
        print(f"  CPU Frequency      : {self.cpu_freq_history[-1]} MHz")
        print(f"  Avg Estimated Arus : {avg_current:.1f} mA")
        print(f"  Avg Estimated Daya : {avg_power:.1f} mW")
        print(f"  Total LED Toggles  : {self.toggle_history[-1]}")
        print(f"  Final Heap Free    : {self.heap_history[-1]} bytes")
        print()

        # Tabel perbandingan mode daya ESP32
        print("  PERBANDINGAN MODE DAYA ESP32 (dari datasheet):")
        print("  ┌──────────────────┬──────────────┬─────────────┐")
        print("  │ Mode             │ Arus (typ)   │ Daya @3.3V  │")
        print("  ├──────────────────┼──────────────┼─────────────┤")
        print("  │ Active 240MHz    │ ~120 mA      │ ~396 mW     │")
        print("  │ Active 80MHz     │ ~30 mA       │ ~99 mW      │")
        print("  │ Modem Sleep      │ ~20 mA       │ ~66 mW      │")
        print("  │ Light Sleep      │ ~0.8 mA      │ ~2.6 mW     │")
        print("  │ Deep Sleep       │ ~10 µA       │ ~0.033 mW   │")
        print("  │ Hibernation      │ ~5 µA        │ ~0.017 mW   │")
        print("  └──────────────────┴──────────────┴─────────────┘")

    def plot_power_profile(self):
        """Buat chart profil daya dari data yang terkumpul."""
        if not HAS_MATPLOTLIB or not self.data_points:
            if not HAS_MATPLOTLIB:
                print("[!] matplotlib tidak tersedia, skip plotting")
            return

        fig, axes = plt.subplots(3, 1, figsize=(12, 10), sharex=True)
        fig.suptitle('ESP32 Active Mode Power Profile', fontsize=14, fontweight='bold')

        times = self.timestamps

        # Plot 1: Estimated current
        currents = [d['est_current_ma'] for d in self.data_points]
        axes[0].plot(times, currents, 'r-o', markersize=4, label='Est. Current')
        axes[0].set_ylabel('Current (mA)')
        axes[0].set_title('Estimated Current Consumption')
        axes[0].legend()
        axes[0].grid(True, alpha=0.3)
        axes[0].axhline(y=120, color='gray', linestyle='--', alpha=0.5, label='Typical 240MHz')

        # Plot 2: Free heap
        axes[1].plot(times, self.heap_history, 'b-s', markersize=4, label='Free Heap')
        axes[1].set_ylabel('Bytes')
        axes[1].set_title('Free Heap Memory')
        axes[1].legend()
        axes[1].grid(True, alpha=0.3)

        # Plot 3: LED toggle count
        axes[2].plot(times, self.toggle_history, 'g-^', markersize=4, label='LED Toggles')
        axes[2].set_ylabel('Count')
        axes[2].set_xlabel('Uptime (seconds)')
        axes[2].set_title('LED Toggle Count (Activity Indicator)')
        axes[2].legend()
        axes[2].grid(True, alpha=0.3)

        plt.tight_layout()
        filename = 'power_profile_active_mode.png'
        plt.savefig(filename, dpi=150)
        print(f"\n[OK] Chart disimpan: {filename}")
        plt.show()


def read_serial(port, baudrate, analyzer, duration=35):
    """Baca data dari serial port."""
    if not HAS_SERIAL:
        print("[ERROR] pyserial diperlukan untuk membaca serial!")
        return

    print(f"[*] Membuka {port} @ {baudrate} baud...")
    try:
        ser = serial.Serial(port, baudrate, timeout=1)
        print(f"[OK] Serial port terbuka. Menunggu data ({duration}s)...")

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
    parser = argparse.ArgumentParser(
        description='ESP32 Current Measurement Analyzer')
    parser.add_argument('--port', default='/dev/ttyUSB0',
                        help='Serial port (default: /dev/ttyUSB0)')
    parser.add_argument('--baud', type=int, default=115200,
                        help='Baud rate (default: 115200)')
    parser.add_argument('--file', type=str, default=None,
                        help='Read from log file instead of serial')
    parser.add_argument('--duration', type=int, default=35,
                        help='Duration in seconds (default: 35)')
    args = parser.parse_args()

    analyzer = CurrentMeasurementAnalyzer()

    print("=" * 60)
    print("  ESP32 Current Measurement - Debug Analyzer")
    print("=" * 60)

    if args.file:
        read_file(args.file, analyzer)
    else:
        read_serial(args.port, args.baud, analyzer, args.duration)

    analyzer.calculate_stats()
    analyzer.plot_power_profile()


if __name__ == '__main__':
    main()
