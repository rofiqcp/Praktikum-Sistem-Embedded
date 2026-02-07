#!/usr/bin/env python3
"""
Debug & Analysis Script - ADC Moving Average
Modul 04 - ADC | Praktikum Sistem Embedded

Script ini memvisualisasikan efek filter moving average pada data ADC.
Menampilkan perbandingan raw vs filtered dan analisis noise reduction.
"""

import serial
import time
import csv
import argparse
import re
from collections import deque

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
        description='ADC Moving Average - Debug & Analysis Tool'
    )
    parser.add_argument('-p', '--port', default='/dev/ttyUSB0',
                        help='Serial port (default: /dev/ttyUSB0)')
    parser.add_argument('-b', '--baud', type=int, default=115200,
                        help='Baud rate (default: 115200)')
    parser.add_argument('-d', '--duration', type=int, default=30,
                        help='Durasi pengambilan data dalam detik (default: 30)')
    parser.add_argument('-o', '--output', default='adc_moving_avg.csv',
                        help='Nama file output CSV')
    return parser.parse_args()


def parse_line(line):
    """Parse: [0001] Raw: 2048 | MA-16: 2050 | MA-32: 2049"""
    match = re.search(
        r'\[(\d+)\]\s+Raw:\s*(\d+)\s*\|\s*MA-16:\s*(\d+)\s*\|\s*MA-32:\s*(\d+)',
        line
    )
    if match:
        return {
            'counter': int(match.group(1)),
            'raw': int(match.group(2)),
            'ma16': int(match.group(3)),
            'ma32': int(match.group(4))
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
        writer.writerow(['timestamp', 'counter', 'raw', 'ma16', 'ma32'])

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
                                parsed['raw'], parsed['ma16'], parsed['ma32']
                            ])
            except Exception as e:
                print(f"[ERROR] {e}")

    print("-" * 70)
    print(f"[INFO] Total data: {len(data)} sampel")
    return timestamps, data


def plot_data(timestamps, data, output_file):
    """Buat grafik perbandingan raw vs filtered."""
    if not HAS_MATPLOTLIB or not data:
        return

    raw = [d['raw'] for d in data]
    ma16 = [d['ma16'] for d in data]
    ma32 = [d['ma32'] for d in data]

    fig, axes = plt.subplots(3, 1, figsize=(14, 10))
    fig.suptitle('ADC Moving Average - Analisis Filter', fontsize=14, fontweight='bold')

    # Plot 1: Semua sinyal
    axes[0].plot(timestamps, raw, 'b-', alpha=0.4, label='Raw', linewidth=0.8)
    axes[0].plot(timestamps, ma16, 'r-', label='MA-16', linewidth=1.5)
    axes[0].plot(timestamps, ma32, 'g-', label='MA-32', linewidth=1.5)
    axes[0].set_xlabel('Waktu (detik)')
    axes[0].set_ylabel('Nilai ADC')
    axes[0].set_title('Perbandingan Raw vs Moving Average')
    axes[0].legend()
    axes[0].grid(True, alpha=0.3)

    # Plot 2: Noise (deviasi dari rata-rata)
    raw_arr = np.array(raw)
    ma16_arr = np.array(ma16)
    ma32_arr = np.array(ma32)
    noise_raw = raw_arr - np.convolve(raw_arr, np.ones(32)/32, mode='same')
    noise_16 = raw_arr - ma16_arr
    noise_32 = raw_arr - ma32_arr

    axes[1].plot(timestamps, noise_raw, 'b-', alpha=0.5, label='Noise Raw')
    axes[1].plot(timestamps, noise_16, 'r-', alpha=0.7, label='Residual MA-16')
    axes[1].set_xlabel('Waktu (detik)')
    axes[1].set_ylabel('Noise')
    axes[1].set_title('Analisis Noise')
    axes[1].legend()
    axes[1].grid(True, alpha=0.3)

    # Plot 3: Noise reduction statistik
    labels = ['Raw', 'MA-16', 'MA-32']
    stds = [np.std(raw), np.std(ma16), np.std(ma32)]
    colors = ['steelblue', 'salmon', 'lightgreen']
    bars = axes[2].bar(labels, stds, color=colors, edgecolor='black')
    axes[2].set_ylabel('Standar Deviasi')
    axes[2].set_title('Perbandingan Noise (Standar Deviasi)')
    for bar, val in zip(bars, stds):
        axes[2].text(bar.get_x() + bar.get_width()/2., bar.get_height() + 0.5,
                     f'{val:.2f}', ha='center', va='bottom', fontweight='bold')
    axes[2].grid(True, alpha=0.3, axis='y')

    plt.tight_layout()
    plot_file = output_file.replace('.csv', '_plot.png')
    plt.savefig(plot_file, dpi=150)
    print(f"[INFO] Grafik disimpan ke: {plot_file}")
    plt.show()


def main():
    args = parse_args()

    print("=" * 60)
    print("  ADC Moving Average - Debug & Analysis Tool")
    print("=" * 60)

    try:
        ser = serial.Serial(args.port, args.baud, timeout=1)
        time.sleep(2)
        ser.flushInput()

        timestamps, data = collect_data(ser, args.duration, args.output)
        ser.close()

        if data and HAS_MATPLOTLIB:
            raw = [d['raw'] for d in data]
            ma16 = [d['ma16'] for d in data]
            ma32 = [d['ma32'] for d in data]
            red16 = (1 - np.std(ma16)/np.std(raw)) * 100 if np.std(raw) > 0 else 0
            red32 = (1 - np.std(ma32)/np.std(raw)) * 100 if np.std(raw) > 0 else 0
            print(f"\n[NOISE REDUCTION]")
            print(f"  Raw StdDev : {np.std(raw):.2f}")
            print(f"  MA-16 StdDev: {np.std(ma16):.2f} (Reduksi: {red16:.1f}%)")
            print(f"  MA-32 StdDev: {np.std(ma32):.2f} (Reduksi: {red32:.1f}%)")
            plot_data(timestamps, data, args.output)

    except serial.SerialException as e:
        print(f"[ERROR] Gagal membuka port serial: {e}")
    except KeyboardInterrupt:
        print("\n[INFO] Dihentikan oleh pengguna.")


if __name__ == '__main__':
    main()
