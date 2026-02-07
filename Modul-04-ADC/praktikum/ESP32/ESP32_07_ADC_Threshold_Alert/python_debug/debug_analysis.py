#!/usr/bin/env python3
"""
Debug & Analysis Script - ADC Threshold Alert
Modul 04 - ADC | Praktikum Sistem Embedded

Script ini memvisualisasikan monitoring threshold ADC,
menampilkan zona alert dan transisi level.
"""

import serial
import time
import csv
import argparse
import re

try:
    import matplotlib.pyplot as plt
    import numpy as np
    HAS_MATPLOTLIB = True
except ImportError:
    HAS_MATPLOTLIB = False
    print("[WARN] matplotlib/numpy not installed. Plotting disabled.")
    print("Install with: pip install matplotlib numpy")


def parse_args():
    parser = argparse.ArgumentParser(
        description='ADC Threshold Alert - Debug & Analysis Tool'
    )
    parser.add_argument('-p', '--port', default='/dev/ttyUSB0',
                        help='Serial port (default: /dev/ttyUSB0)')
    parser.add_argument('-b', '--baud', type=int, default=115200,
                        help='Baud rate (default: 115200)')
    parser.add_argument('-d', '--duration', type=int, default=30,
                        help='Durasi pengambilan data (detik)')
    parser.add_argument('-o', '--output', default='adc_threshold.csv',
                        help='Nama file output CSV')
    return parser.parse_args()


def parse_line(line):
    """Parse: [0001] Raw: 2048 | 1650 mV | NORMAL ..."""
    match = re.search(
        r'\[(\d+)\]\s+Raw:\s*(\d+)\s*\|\s*(\d+)\s*mV\s*\|\s*(\w+)',
        line
    )
    if match:
        return {
            'counter': int(match.group(1)),
            'raw': int(match.group(2)),
            'voltage': int(match.group(3)),
            'level': match.group(4).strip()
        }
    return None


def collect_data(ser, duration, output_file):
    """Kumpulkan data dari serial port."""
    data = []
    timestamps = []
    start_time = time.time()

    print(f"\n[INFO] Mengumpulkan data selama {duration} detik...")
    print("-" * 80)

    with open(output_file, 'w', newline='') as csvfile:
        writer = csv.writer(csvfile)
        writer.writerow(['timestamp', 'counter', 'raw', 'voltage_mV', 'level'])

        while (time.time() - start_time) < duration:
            try:
                if ser.in_waiting:
                    line = ser.readline().decode('utf-8', errors='ignore').strip()
                    if line:
                        print(f"  {line}")
                        parsed = parse_line(line)
                        if parsed:
                            elapsed = time.time() - start_time
                            timestamps.append(elapsed)
                            data.append(parsed)
                            writer.writerow([
                                f"{elapsed:.3f}", parsed['counter'],
                                parsed['raw'], parsed['voltage'],
                                parsed['level']
                            ])
            except Exception as e:
                print(f"[ERROR] {e}")

    print("-" * 80)
    print(f"[INFO] Total data: {len(data)} sampel")
    return timestamps, data


def plot_data(timestamps, data, output_file):
    """Buat grafik threshold monitoring."""
    if not HAS_MATPLOTLIB or not data:
        return

    raw_vals = [d['raw'] for d in data]
    levels = [d['level'] for d in data]

    LOW_TH = 1000
    HIGH_TH = 3000

    fig, axes = plt.subplots(2, 1, figsize=(14, 8))
    fig.suptitle('ADC Threshold Alert - Monitoring', fontsize=14, fontweight='bold')

    # Plot 1: Nilai ADC dengan zona threshold
    axes[0].fill_between(timestamps, 0, LOW_TH, alpha=0.1, color='blue', label='Zona LOW')
    axes[0].fill_between(timestamps, LOW_TH, HIGH_TH, alpha=0.1, color='green', label='Zona NORMAL')
    axes[0].fill_between(timestamps, HIGH_TH, 4095, alpha=0.1, color='red', label='Zona HIGH')
    axes[0].axhline(y=LOW_TH, color='blue', linestyle='--', alpha=0.7)
    axes[0].axhline(y=HIGH_TH, color='red', linestyle='--', alpha=0.7)
    axes[0].plot(timestamps, raw_vals, 'k-', linewidth=1, label='ADC Value')

    # Tandai titik alert
    alert_t = [t for t, d in zip(timestamps, data) if d['level'] == 'HIGH']
    alert_v = [d['raw'] for d in data if d['level'] == 'HIGH']
    if alert_t:
        axes[0].scatter(alert_t, alert_v, color='red', s=20, zorder=5, label='ALERT')

    axes[0].set_xlabel('Waktu (detik)')
    axes[0].set_ylabel('Nilai ADC')
    axes[0].set_title('Monitoring ADC dengan Threshold')
    axes[0].legend(loc='upper right')
    axes[0].grid(True, alpha=0.3)
    axes[0].set_ylim(-100, 4200)

    # Plot 2: Distribusi level
    level_counts = {'LOW': 0, 'NORMAL': 0, 'HIGH': 0}
    for l in levels:
        if l in level_counts:
            level_counts[l] += 1

    labels = list(level_counts.keys())
    values = list(level_counts.values())
    colors = ['steelblue', 'lightgreen', 'salmon']
    bars = axes[1].bar(labels, values, color=colors, edgecolor='black')
    axes[1].set_ylabel('Jumlah Sampel')
    axes[1].set_title('Distribusi Level')
    for bar, val in zip(bars, values):
        pct = val / len(data) * 100 if data else 0
        axes[1].text(bar.get_x() + bar.get_width()/2., bar.get_height() + 0.5,
                     f'{val} ({pct:.1f}%)', ha='center', va='bottom', fontweight='bold')
    axes[1].grid(True, alpha=0.3, axis='y')

    plt.tight_layout()
    plot_file = output_file.replace('.csv', '_plot.png')
    plt.savefig(plot_file, dpi=150)
    print(f"[INFO] Grafik disimpan ke: {plot_file}")
    plt.show()


def main():
    args = parse_args()

    print("=" * 60)
    print("  ADC Threshold Alert - Debug & Analysis Tool")
    print("=" * 60)

    try:
        ser = serial.Serial(args.port, args.baud, timeout=1)
        time.sleep(2)
        ser.flushInput()

        timestamps, data = collect_data(ser, args.duration, args.output)
        ser.close()

        if data:
            plot_data(timestamps, data, args.output)

    except serial.SerialException as e:
        print(f"[ERROR] Gagal membuka port serial: {e}")
    except KeyboardInterrupt:
        print("\n[INFO] Dihentikan oleh pengguna.")


if __name__ == '__main__':
    main()
