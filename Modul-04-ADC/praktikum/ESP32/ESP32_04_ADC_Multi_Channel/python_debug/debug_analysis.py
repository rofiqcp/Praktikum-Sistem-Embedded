#!/usr/bin/env python3
"""
Debug & Analysis Script - ADC Multi Channel
Modul 04 - ADC | Praktikum Sistem Embedded

Script ini memvisualisasikan pembacaan dua channel ADC secara bersamaan.
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
        description='ADC Multi Channel - Debug & Analysis Tool'
    )
    parser.add_argument('-p', '--port', default='/dev/ttyUSB0',
                        help='Serial port (default: /dev/ttyUSB0)')
    parser.add_argument('-b', '--baud', type=int, default=115200,
                        help='Baud rate (default: 115200)')
    parser.add_argument('-d', '--duration', type=int, default=30,
                        help='Durasi pengambilan data (detik)')
    parser.add_argument('-o', '--output', default='adc_multi_channel.csv',
                        help='Nama file output CSV')
    return parser.parse_args()


def parse_line(line):
    """Parse: [0001] | 2048 / 1650 mV      | 1024 /  825 mV      |  +825 mV"""
    match = re.search(
        r'\[(\d+)\]\s*\|\s*(\d+)\s*/\s*(\d+)\s*mV\s*\|\s*(\d+)\s*/\s*(\d+)\s*mV\s*\|\s*([+-]?\d+)\s*mV',
        line
    )
    if match:
        return {
            'counter': int(match.group(1)),
            'raw1': int(match.group(2)),
            'mv1': int(match.group(3)),
            'raw2': int(match.group(4)),
            'mv2': int(match.group(5)),
            'diff': int(match.group(6))
        }
    return None


def collect_data(ser, duration, output_file):
    """Kumpulkan data dari serial port."""
    data = []
    timestamps = []
    start_time = time.time()

    print(f"\n[INFO] Mengumpulkan data selama {duration} detik...")
    print("-" * 70)

    with open(output_file, 'w', newline='') as csvfile:
        writer = csv.writer(csvfile)
        writer.writerow(['timestamp', 'counter', 'raw_ch1', 'mv_ch1',
                         'raw_ch2', 'mv_ch2', 'diff_mV'])

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
                                parsed['raw1'], parsed['mv1'],
                                parsed['raw2'], parsed['mv2'],
                                parsed['diff']
                            ])
            except Exception as e:
                print(f"[ERROR] {e}")

    print("-" * 70)
    print(f"[INFO] Total data: {len(data)} sampel")
    return timestamps, data


def plot_data(timestamps, data, output_file):
    """Buat grafik multi-channel."""
    if not HAS_MATPLOTLIB or not data:
        return

    mv1 = [d['mv1'] for d in data]
    mv2 = [d['mv2'] for d in data]
    diff = [d['diff'] for d in data]

    fig, axes = plt.subplots(2, 2, figsize=(14, 10))
    fig.suptitle('ADC Multi Channel - Analisis 2 Potensiometer', fontsize=14, fontweight='bold')

    # Plot 1: Kedua channel vs waktu
    axes[0][0].plot(timestamps, mv1, 'b-', label='POT-1', linewidth=1.5)
    axes[0][0].plot(timestamps, mv2, 'r-', label='POT-2', linewidth=1.5)
    axes[0][0].set_xlabel('Waktu (detik)')
    axes[0][0].set_ylabel('Tegangan (mV)')
    axes[0][0].set_title('Tegangan Kedua Channel')
    axes[0][0].legend()
    axes[0][0].grid(True, alpha=0.3)

    # Plot 2: Selisih
    axes[0][1].plot(timestamps, diff, 'g-', linewidth=1)
    axes[0][1].axhline(y=0, color='black', linestyle='--', alpha=0.5)
    axes[0][1].fill_between(timestamps, diff, alpha=0.2, color='green')
    axes[0][1].set_xlabel('Waktu (detik)')
    axes[0][1].set_ylabel('Selisih (mV)')
    axes[0][1].set_title('Selisih CH1 - CH2')
    axes[0][1].grid(True, alpha=0.3)

    # Plot 3: Histogram kedua channel
    axes[1][0].hist(mv1, bins=25, alpha=0.6, label='POT-1', color='blue')
    axes[1][0].hist(mv2, bins=25, alpha=0.6, label='POT-2', color='red')
    axes[1][0].set_xlabel('Tegangan (mV)')
    axes[1][0].set_ylabel('Frekuensi')
    axes[1][0].set_title('Distribusi Tegangan')
    axes[1][0].legend()
    axes[1][0].grid(True, alpha=0.3)

    # Plot 4: Scatter CH1 vs CH2
    axes[1][1].scatter(mv1, mv2, alpha=0.5, s=15)
    axes[1][1].set_xlabel('POT-1 (mV)')
    axes[1][1].set_ylabel('POT-2 (mV)')
    axes[1][1].set_title('Korelasi CH1 vs CH2')
    axes[1][1].grid(True, alpha=0.3)

    plt.tight_layout()
    plot_file = output_file.replace('.csv', '_plot.png')
    plt.savefig(plot_file, dpi=150)
    print(f"[INFO] Grafik disimpan ke: {plot_file}")
    plt.show()


def main():
    args = parse_args()

    print("=" * 60)
    print("  ADC Multi Channel - Debug & Analysis Tool")
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
