#!/usr/bin/env python3
"""
Debug & Analysis Script - ADC Light Sensor
Modul 04 - ADC | Praktikum Sistem Embedded

Script ini memvisualisasikan pembacaan sensor cahaya LDR.
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
        description='ADC Light Sensor - Debug & Analysis Tool'
    )
    parser.add_argument('-p', '--port', default='/dev/ttyUSB0',
                        help='Serial port (default: /dev/ttyUSB0)')
    parser.add_argument('-b', '--baud', type=int, default=115200,
                        help='Baud rate (default: 115200)')
    parser.add_argument('-d', '--duration', type=int, default=60,
                        help='Durasi pengambilan data (detik)')
    parser.add_argument('-o', '--output', default='adc_light_sensor.csv',
                        help='Nama file output CSV')
    return parser.parse_args()


def parse_line(line):
    """Parse: [0001] Raw: 2048 | 1650 mV | R_LDR:    15000 Ω | Lux:    120.5 | NORMAL"""
    match = re.search(
        r'\[(\d+)\]\s+Raw:\s*(\d+)\s*\|\s*(\d+)\s*mV\s*\|\s*R_LDR:\s*([\d.]+)\s*.*?\|\s*Lux:\s*([\d.]+)\s*\|\s*(\w+)',
        line
    )
    if match:
        return {
            'counter': int(match.group(1)),
            'raw': int(match.group(2)),
            'voltage': int(match.group(3)),
            'r_ldr': float(match.group(4)),
            'lux': float(match.group(5)),
            'level': match.group(6).strip()
        }
    return None


def collect_data(ser, duration, output_file):
    """Kumpulkan data dari serial port."""
    data = []
    timestamps = []
    start_time = time.time()

    print(f"\n[INFO] Mengumpulkan data selama {duration} detik...")
    print("-" * 90)

    with open(output_file, 'w', newline='') as csvfile:
        writer = csv.writer(csvfile)
        writer.writerow(['timestamp', 'counter', 'raw', 'voltage_mV',
                         'r_ldr_ohm', 'lux', 'level'])

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
                                parsed['r_ldr'], parsed['lux'],
                                parsed['level']
                            ])
            except Exception as e:
                print(f"[ERROR] {e}")

    print("-" * 90)
    print(f"[INFO] Total data: {len(data)} sampel")
    return timestamps, data


def plot_data(timestamps, data, output_file):
    """Buat grafik sensor cahaya."""
    if not HAS_MATPLOTLIB or not data:
        return

    lux_vals = [d['lux'] for d in data]
    r_ldr = [d['r_ldr'] for d in data]
    raw_vals = [d['raw'] for d in data]

    fig, axes = plt.subplots(2, 2, figsize=(14, 10))
    fig.suptitle('Light Sensor (LDR) - Analisis', fontsize=14, fontweight='bold')

    # Plot 1: Lux vs Waktu
    axes[0][0].plot(timestamps, lux_vals, 'orange', linewidth=1.5)
    axes[0][0].fill_between(timestamps, 0, lux_vals, alpha=0.2, color='yellow')
    axes[0][0].axhline(y=10, color='gray', linestyle='--', alpha=0.5, label='Dark/Dim')
    axes[0][0].axhline(y=100, color='green', linestyle='--', alpha=0.5, label='Dim/Normal')
    axes[0][0].axhline(y=500, color='red', linestyle='--', alpha=0.5, label='Normal/Bright')
    axes[0][0].set_xlabel('Waktu (detik)')
    axes[0][0].set_ylabel('Intensitas Cahaya (Lux)')
    axes[0][0].set_title('Intensitas Cahaya vs Waktu')
    axes[0][0].legend()
    axes[0][0].grid(True, alpha=0.3)

    # Plot 2: Lux (log scale) vs Waktu
    axes[0][1].semilogy(timestamps, lux_vals, 'b-', linewidth=1)
    axes[0][1].set_xlabel('Waktu (detik)')
    axes[0][1].set_ylabel('Lux (log scale)')
    axes[0][1].set_title('Intensitas Cahaya (Skala Logaritmik)')
    axes[0][1].grid(True, alpha=0.3)

    # Plot 3: Resistansi LDR vs Waktu
    axes[1][0].plot(timestamps, r_ldr, 'g-', linewidth=1)
    axes[1][0].set_xlabel('Waktu (detik)')
    axes[1][0].set_ylabel('Resistansi LDR (Ω)')
    axes[1][0].set_title('Resistansi LDR vs Waktu')
    axes[1][0].grid(True, alpha=0.3)

    # Plot 4: Distribusi level cahaya (pie chart)
    levels = [d['level'] for d in data]
    level_counts = {}
    for l in levels:
        level_counts[l] = level_counts.get(l, 0) + 1
    labels_pie = list(level_counts.keys())
    values_pie = list(level_counts.values())
    colors_pie = {'GELAP': 'gray', 'REDUP': 'lightyellow', 'NORMAL': 'lightgreen', 'TERANG': 'gold'}
    pie_colors = [colors_pie.get(l, 'lightblue') for l in labels_pie]
    axes[1][1].pie(values_pie, labels=labels_pie, colors=pie_colors,
                   autopct='%1.1f%%', startangle=90)
    axes[1][1].set_title('Distribusi Level Cahaya')

    plt.tight_layout()
    plot_file = output_file.replace('.csv', '_plot.png')
    plt.savefig(plot_file, dpi=150)
    print(f"[INFO] Grafik disimpan ke: {plot_file}")
    plt.show()


def main():
    args = parse_args()

    print("=" * 60)
    print("  ADC Light Sensor - Debug & Analysis Tool")
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
