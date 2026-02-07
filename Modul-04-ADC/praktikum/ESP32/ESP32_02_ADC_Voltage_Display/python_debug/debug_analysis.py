#!/usr/bin/env python3
"""
Debug & Analysis Script - ADC Voltage Display
Modul 04 - ADC | Praktikum Sistem Embedded

Script ini membaca data tegangan ADC dari ESP32, membandingkan
nilai manual vs terkalibrasi, dan membuat visualisasi.
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
        description='ADC Voltage Display - Debug & Analysis Tool'
    )
    parser.add_argument('-p', '--port', default='/dev/ttyUSB0',
                        help='Serial port (default: /dev/ttyUSB0)')
    parser.add_argument('-b', '--baud', type=int, default=115200,
                        help='Baud rate (default: 115200)')
    parser.add_argument('-d', '--duration', type=int, default=30,
                        help='Durasi pengambilan data dalam detik (default: 30)')
    parser.add_argument('-o', '--output', default='adc_voltage.csv',
                        help='Nama file output CSV (default: adc_voltage.csv)')
    return parser.parse_args()


def parse_line(line):
    """Parse output: [0001] Raw: 2048 | Manual: 1650 mV (1.650 V) | Kalibrasi: 1660 mV (1.660 V) | Selisih: +10 mV"""
    match = re.search(
        r'\[(\d+)\]\s+Raw:\s*(\d+)\s*\|\s*Manual:\s*(\d+)\s*mV.*?'
        r'Kalibrasi:\s*(\d+)\s*mV.*?Selisih:\s*([+-]?\d+)\s*mV',
        line
    )
    if match:
        return {
            'counter': int(match.group(1)),
            'raw': int(match.group(2)),
            'voltage_manual': int(match.group(3)),
            'voltage_cal': int(match.group(4)),
            'difference': int(match.group(5))
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
        writer.writerow(['timestamp', 'counter', 'raw', 'voltage_manual_mV',
                         'voltage_calibrated_mV', 'difference_mV'])

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
                                f"{elapsed:.3f}",
                                parsed['counter'],
                                parsed['raw'],
                                parsed['voltage_manual'],
                                parsed['voltage_cal'],
                                parsed['difference']
                            ])
            except Exception as e:
                print(f"[ERROR] {e}")

    print("-" * 80)
    print(f"[INFO] Selesai. Total data: {len(data)} sampel")
    return timestamps, data


def plot_data(timestamps, data, output_file):
    """Buat grafik perbandingan tegangan manual vs terkalibrasi."""
    if not HAS_MATPLOTLIB or not data:
        return

    v_manual = [d['voltage_manual'] for d in data]
    v_cal = [d['voltage_cal'] for d in data]
    diff = [d['difference'] for d in data]

    fig, axes = plt.subplots(3, 1, figsize=(12, 10))
    fig.suptitle('ADC Voltage Display - Analisis Perbandingan', fontsize=14, fontweight='bold')

    # Plot 1: Tegangan manual vs kalibrasi
    axes[0].plot(timestamps, v_manual, 'b-', label='Manual', alpha=0.7)
    axes[0].plot(timestamps, v_cal, 'r-', label='Kalibrasi', alpha=0.7)
    axes[0].set_xlabel('Waktu (detik)')
    axes[0].set_ylabel('Tegangan (mV)')
    axes[0].set_title('Perbandingan Tegangan Manual vs Terkalibrasi')
    axes[0].legend()
    axes[0].grid(True, alpha=0.3)

    # Plot 2: Selisih (error)
    axes[1].plot(timestamps, diff, 'g-o', markersize=2)
    axes[1].axhline(y=0, color='black', linestyle='--', alpha=0.5)
    axes[1].fill_between(timestamps, diff, alpha=0.2, color='green')
    axes[1].set_xlabel('Waktu (detik)')
    axes[1].set_ylabel('Selisih (mV)')
    axes[1].set_title('Selisih Kalibrasi - Manual (mV)')
    axes[1].grid(True, alpha=0.3)

    # Plot 3: Scatter plot korelasi
    axes[2].scatter(v_manual, v_cal, alpha=0.5, s=10)
    min_v = min(min(v_manual), min(v_cal))
    max_v = max(max(v_manual), max(v_cal))
    axes[2].plot([min_v, max_v], [min_v, max_v], 'r--', label='Garis ideal')
    axes[2].set_xlabel('Tegangan Manual (mV)')
    axes[2].set_ylabel('Tegangan Kalibrasi (mV)')
    axes[2].set_title('Korelasi Manual vs Kalibrasi')
    axes[2].legend()
    axes[2].grid(True, alpha=0.3)

    plt.tight_layout()
    plot_file = output_file.replace('.csv', '_plot.png')
    plt.savefig(plot_file, dpi=150)
    print(f"[INFO] Grafik disimpan ke: {plot_file}")
    plt.show()


def main():
    args = parse_args()

    print("=" * 60)
    print("  ADC Voltage Display - Debug & Analysis Tool")
    print("  Modul 04 - ADC | Praktikum Sistem Embedded")
    print("=" * 60)

    try:
        ser = serial.Serial(args.port, args.baud, timeout=1)
        time.sleep(2)
        ser.flushInput()

        timestamps, data = collect_data(ser, args.duration, args.output)
        ser.close()

        if data:
            diff = [d['difference'] for d in data]
            print(f"\n[STATISTIK SELISIH]")
            print(f"  Rata-rata selisih: {np.mean(diff):.2f} mV" if HAS_MATPLOTLIB else "")
            print(f"  Max selisih      : {max(diff)} mV")
            print(f"  Min selisih      : {min(diff)} mV")
            plot_data(timestamps, data, args.output)

    except serial.SerialException as e:
        print(f"[ERROR] Gagal membuka port serial: {e}")
    except KeyboardInterrupt:
        print("\n[INFO] Dihentikan oleh pengguna.")


if __name__ == '__main__':
    main()
