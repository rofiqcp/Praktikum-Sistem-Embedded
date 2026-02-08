#!/usr/bin/env python3
"""
==========================================================================
ESP32_05_Deep_Sleep_RTC - Debug & Analysis Script
==========================================================================
Modul 14 - Power Management
Program 5: Scheduled wakeup analysis, duty cycle calculator

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
    import numpy as np
    HAS_MATPLOTLIB = True
except ImportError:
    HAS_MATPLOTLIB = False
    print("[WARN] matplotlib not installed. Install: pip install matplotlib")


class ScheduledWakeupAnalyzer:
    """Parser dan analyzer untuk scheduled deep sleep wakeup ESP32."""

    def __init__(self):
        self.events = []
        # Pattern: DATA,boot,adc,active_ms,voltage,total_active,total_sleep,readings,duty,current
        self.data_pattern = re.compile(
            r'DATA,(\d+),(\d+),(\d+),([\d.]+),(\d+),(\d+),(\d+),([\d.]+),([\d.]+)'
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
                'adc_raw': int(match.group(2)),
                'active_ms': int(match.group(3)),
                'voltage': float(match.group(4)),
                'total_active_ms': int(match.group(5)),
                'total_sleep_ms': int(match.group(6)),
                'total_readings': int(match.group(7)),
                'duty_cycle': float(match.group(8)),
                'avg_current_ua': float(match.group(9)),
            }
            self.events.append(data)

            print(f"  [BOOT {data['boot_count']}] ADC={data['adc_raw']} ({data['voltage']:.2f}V) | "
                  f"Active={data['active_ms']}ms | "
                  f"Duty={data['duty_cycle']:.2f}% | "
                  f"I_avg={data['avg_current_ua']:.1f}µA")
            return

        if any(kw in line for kw in ['SCHED', 'Sensor', 'duty', 'battery', 'sleep']):
            print(f"  [LOG] {line}")

    def calculate_stats(self):
        """Hitung statistik scheduled wakeup."""
        if not self.events:
            print("\n[!] Tidak ada data untuk dianalisis.")
            return

        print("\n" + "=" * 60)
        print("  ANALISIS SCHEDULED DATA LOGGER")
        print("=" * 60)

        last = self.events[-1]
        adc_values = [e['adc_raw'] for e in self.events]
        voltages = [e['voltage'] for e in self.events]
        active_times = [e['active_ms'] for e in self.events]

        print(f"  Total Boot Cycles    : {last['boot_count']}")
        print(f"  Total Readings       : {last['total_readings']}")
        print(f"  Total Active Time    : {last['total_active_ms']} ms ({last['total_active_ms']/1000:.1f} s)")
        print(f"  Total Sleep Time     : {last['total_sleep_ms']} ms ({last['total_sleep_ms']/1000:.1f} s)")
        print(f"  Effective Duty Cycle : {last['duty_cycle']:.2f}%")

        print(f"\n  SENSOR READINGS STATISTICS:")
        print(f"    ADC Min    : {min(adc_values)}")
        print(f"    ADC Max    : {max(adc_values)}")
        print(f"    ADC Avg    : {sum(adc_values)/len(adc_values):.1f}")
        print(f"    Voltage Min: {min(voltages):.3f} V")
        print(f"    Voltage Max: {max(voltages):.3f} V")
        print(f"    Voltage Avg: {sum(voltages)/len(voltages):.3f} V")

        print(f"\n  ACTIVE TIME STATISTICS:")
        print(f"    Min : {min(active_times)} ms")
        print(f"    Max : {max(active_times)} ms")
        print(f"    Avg : {sum(active_times)/len(active_times):.1f} ms")

        # Battery life estimation
        avg_current_ua = last['avg_current_ua']
        battery_capacities = [1000, 2000, 3000, 5000]  # mAh
        print(f"\n  ESTIMASI BATTERY LIFE (avg current: {avg_current_ua:.1f} µA):")
        print(f"  ┌──────────────┬──────────────┬──────────────┐")
        print(f"  │ Battery (mAh)│ Hours        │ Days         │")
        print(f"  ├──────────────┼──────────────┼──────────────┤")
        for cap in battery_capacities:
            hours = (cap * 1000) / avg_current_ua
            days = hours / 24
            print(f"  │ {cap:12d} │ {hours:12.0f} │ {days:12.0f} │")
        print(f"  └──────────────┴──────────────┴──────────────┘")

    def plot_analysis(self):
        """Buat chart analisis scheduled wakeup."""
        if not HAS_MATPLOTLIB or not self.events:
            return

        fig, axes = plt.subplots(2, 2, figsize=(14, 10))
        fig.suptitle('ESP32 Scheduled Data Logger Analysis', fontsize=14, fontweight='bold')

        boots = [e['boot_count'] for e in self.events]
        voltages = [e['voltage'] for e in self.events]
        active_ms = [e['active_ms'] for e in self.events]
        duty = [e['duty_cycle'] for e in self.events]
        current = [e['avg_current_ua'] for e in self.events]

        # Plot 1: Sensor voltage over time
        axes[0, 0].plot(boots, voltages, 'b-o', markersize=5, label='Voltage')
        axes[0, 0].set_xlabel('Boot Count')
        axes[0, 0].set_ylabel('Voltage (V)')
        axes[0, 0].set_title('Sensor Voltage Readings')
        axes[0, 0].set_ylim(0, 3.5)
        axes[0, 0].legend()
        axes[0, 0].grid(True, alpha=0.3)

        # Plot 2: Active time per boot
        axes[0, 1].bar(boots, active_ms, color='#e74c3c', alpha=0.8)
        axes[0, 1].set_xlabel('Boot Count')
        axes[0, 1].set_ylabel('Active Time (ms)')
        axes[0, 1].set_title('Active Time per Wakeup')
        axes[0, 1].grid(True, alpha=0.3)

        # Plot 3: Duty cycle evolution
        axes[1, 0].plot(boots, duty, 'g-o', markersize=5)
        axes[1, 0].set_xlabel('Boot Count')
        axes[1, 0].set_ylabel('Duty Cycle (%)')
        axes[1, 0].set_title('Effective Duty Cycle (lower = more efficient)')
        axes[1, 0].grid(True, alpha=0.3)

        # Plot 4: Average current estimation
        axes[1, 1].plot(boots, current, 'm-o', markersize=5)
        axes[1, 1].set_xlabel('Boot Count')
        axes[1, 1].set_ylabel('Avg Current (µA)')
        axes[1, 1].set_title('Estimated Average Current')
        axes[1, 1].grid(True, alpha=0.3)
        axes[1, 1].axhline(y=10, color='green', linestyle='--', alpha=0.5, label='Deep Sleep only')
        axes[1, 1].legend()

        plt.tight_layout()
        filename = 'scheduled_wakeup_analysis.png'
        plt.savefig(filename, dpi=150)
        print(f"\n[OK] Chart disimpan: {filename}")
        plt.show()


def read_serial(port, baudrate, analyzer, duration=300):
    """Baca data dari serial port."""
    if not HAS_SERIAL:
        print("[ERROR] pyserial diperlukan!")
        return

    print(f"[*] Membuka {port} @ {baudrate} baud...")
    print("[*] Interval sleep: 30 detik. Sabar menunggu beberapa siklus...")
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
    parser = argparse.ArgumentParser(description='ESP32 Scheduled Wakeup Analyzer')
    parser.add_argument('--port', default='/dev/ttyUSB0')
    parser.add_argument('--baud', type=int, default=115200)
    parser.add_argument('--file', type=str, default=None)
    parser.add_argument('--duration', type=int, default=300)
    args = parser.parse_args()

    analyzer = ScheduledWakeupAnalyzer()

    print("=" * 60)
    print("  ESP32 Scheduled Wakeup - Debug Analyzer")
    print("=" * 60)

    if args.file:
        read_file(args.file, analyzer)
    else:
        read_serial(args.port, args.baud, analyzer, args.duration)

    analyzer.calculate_stats()
    analyzer.plot_analysis()


if __name__ == '__main__':
    main()
