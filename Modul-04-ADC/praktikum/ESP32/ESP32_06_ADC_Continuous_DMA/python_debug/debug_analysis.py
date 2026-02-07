#!/usr/bin/env python3
"""
Debug & Analysis Script - ADC Continuous DMA
Modul 04 - ADC | Praktikum Sistem Embedded

Script ini menganalisis data dari mode ADC continuous DMA,
menampilkan throughput, distribusi sampel, dan statistik batch.
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
        description='ADC Continuous DMA - Debug & Analysis Tool'
    )
    parser.add_argument('-p', '--port', default='/dev/ttyUSB0',
                        help='Serial port (default: /dev/ttyUSB0)')
    parser.add_argument('-b', '--baud', type=int, default=115200,
                        help='Baud rate (default: 115200)')
    parser.add_argument('-d', '--duration', type=int, default=30,
                        help='Durasi pengambilan data (detik)')
    parser.add_argument('-o', '--output', default='adc_dma.csv',
                        help='Nama file output CSV')
    return parser.parse_args()


def parse_line(line):
    """Parse: [Batch 0001] Samples:  64 | Avg: 2048 | Min: 2000 | Max: 2100 | Voltage: 1650.0 mV | Bytes: 256"""
    match = re.search(
        r'\[Batch\s+(\d+)\]\s+Samples:\s*(\d+)\s*\|\s*Avg:\s*(\d+)\s*\|'
        r'\s*Min:\s*(\d+)\s*\|\s*Max:\s*(\d+)\s*\|\s*Voltage:\s*([\d.]+)\s*mV'
        r'\s*\|\s*Bytes:\s*(\d+)',
        line
    )
    if match:
        return {
            'batch': int(match.group(1)),
            'samples': int(match.group(2)),
            'avg': int(match.group(3)),
            'min': int(match.group(4)),
            'max': int(match.group(5)),
            'voltage': float(match.group(6)),
            'bytes': int(match.group(7))
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
        writer.writerow(['timestamp', 'batch', 'samples', 'avg', 'min', 'max',
                         'voltage_mV', 'bytes'])

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
                                f"{elapsed:.3f}", parsed['batch'],
                                parsed['samples'], parsed['avg'],
                                parsed['min'], parsed['max'],
                                parsed['voltage'], parsed['bytes']
                            ])
            except Exception as e:
                print(f"[ERROR] {e}")

    print("-" * 90)
    print(f"[INFO] Total batch: {len(data)}")
    return timestamps, data


def plot_data(timestamps, data, output_file):
    """Buat grafik analisis DMA."""
    if not HAS_MATPLOTLIB or not data:
        return

    avg_vals = [d['avg'] for d in data]
    min_vals = [d['min'] for d in data]
    max_vals = [d['max'] for d in data]
    voltages = [d['voltage'] for d in data]
    samples = [d['samples'] for d in data]

    fig, axes = plt.subplots(2, 2, figsize=(14, 10))
    fig.suptitle('ADC Continuous DMA - Analisis', fontsize=14, fontweight='bold')

    # Plot 1: Nilai rata-rata dengan min/max range
    axes[0][0].fill_between(timestamps, min_vals, max_vals,
                            alpha=0.3, color='blue', label='Min-Max Range')
    axes[0][0].plot(timestamps, avg_vals, 'b-', label='Rata-rata', linewidth=1.5)
    axes[0][0].set_xlabel('Waktu (detik)')
    axes[0][0].set_ylabel('Nilai ADC')
    axes[0][0].set_title('Rata-rata ADC per Batch (dengan Min/Max)')
    axes[0][0].legend()
    axes[0][0].grid(True, alpha=0.3)

    # Plot 2: Tegangan vs waktu
    axes[0][1].plot(timestamps, voltages, 'r-', linewidth=1)
    axes[0][1].set_xlabel('Waktu (detik)')
    axes[0][1].set_ylabel('Tegangan (mV)')
    axes[0][1].set_title('Tegangan vs Waktu')
    axes[0][1].grid(True, alpha=0.3)

    # Plot 3: Jumlah sampel per batch
    axes[1][0].bar(range(len(samples)), samples, color='steelblue', alpha=0.7)
    axes[1][0].set_xlabel('Batch Number')
    axes[1][0].set_ylabel('Jumlah Sampel')
    axes[1][0].set_title('Sampel per Batch DMA')
    axes[1][0].grid(True, alpha=0.3, axis='y')

    # Plot 4: Spread (Max-Min) per batch
    spreads = [d['max'] - d['min'] for d in data]
    axes[1][1].plot(timestamps, spreads, 'g-o', markersize=3)
    axes[1][1].set_xlabel('Waktu (detik)')
    axes[1][1].set_ylabel('Spread (Max-Min)')
    axes[1][1].set_title('Noise Spread per Batch')
    axes[1][1].grid(True, alpha=0.3)

    plt.tight_layout()
    plot_file = output_file.replace('.csv', '_plot.png')
    plt.savefig(plot_file, dpi=150)
    print(f"[INFO] Grafik disimpan ke: {plot_file}")
    plt.show()


def main():
    args = parse_args()

    print("=" * 60)
    print("  ADC Continuous DMA - Debug & Analysis Tool")
    print("=" * 60)

    try:
        ser = serial.Serial(args.port, args.baud, timeout=1)
        time.sleep(2)
        ser.flushInput()

        timestamps, data = collect_data(ser, args.duration, args.output)
        ser.close()

        if data:
            total_samples = sum(d['samples'] for d in data)
            total_time = timestamps[-1] - timestamps[0] if len(timestamps) > 1 else 1
            print(f"\n[STATISTIK DMA]")
            print(f"  Total batch   : {len(data)}")
            print(f"  Total sampel  : {total_samples}")
            print(f"  Throughput    : {total_samples/total_time:.0f} sampel/detik")
            plot_data(timestamps, data, args.output)

    except serial.SerialException as e:
        print(f"[ERROR] Gagal membuka port serial: {e}")
    except KeyboardInterrupt:
        print("\n[INFO] Dihentikan oleh pengguna.")


if __name__ == '__main__':
    main()
