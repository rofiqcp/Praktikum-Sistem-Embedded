#!/usr/bin/env python3
"""
Debug & Analysis Script - ADC Temperature Internal
Modul 04 - ADC | Praktikum Sistem Embedded

Script ini memvisualisasikan pembacaan sensor suhu internal ESP32.
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
        description='ADC Temperature Internal - Debug & Analysis Tool'
    )
    parser.add_argument('-p', '--port', default='/dev/ttyUSB0',
                        help='Serial port (default: /dev/ttyUSB0)')
    parser.add_argument('-b', '--baud', type=int, default=115200,
                        help='Baud rate (default: 115200)')
    parser.add_argument('-d', '--duration', type=int, default=60,
                        help='Durasi pengambilan data (detik)')
    parser.add_argument('-o', '--output', default='adc_temperature.csv',
                        help='Nama file output CSV')
    return parser.parse_args()


def parse_line(line):
    """Parse: [0001] Suhu:  45.23 °C |  113.41 °F | Status: NORMAL"""
    match = re.search(
        r'\[(\d+)\]\s+Suhu:\s*([\d.]+)\s*°C\s*\|\s*([\d.]+)\s*°F\s*\|\s*Status:\s*(\w+)',
        line
    )
    if match:
        return {
            'counter': int(match.group(1)),
            'temp_c': float(match.group(2)),
            'temp_f': float(match.group(3)),
            'status': match.group(4).strip()
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
        writer.writerow(['timestamp', 'counter', 'temp_celsius',
                         'temp_fahrenheit', 'status'])

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
                                parsed['temp_c'], parsed['temp_f'],
                                parsed['status']
                            ])
            except Exception as e:
                print(f"[ERROR] {e}")

    print("-" * 70)
    print(f"[INFO] Total data: {len(data)} sampel")
    return timestamps, data


def plot_data(timestamps, data, output_file):
    """Buat grafik suhu."""
    if not HAS_MATPLOTLIB or not data:
        return

    temps_c = [d['temp_c'] for d in data]
    temps_f = [d['temp_f'] for d in data]

    fig, axes = plt.subplots(2, 2, figsize=(14, 10))
    fig.suptitle('Internal Temperature Sensor - Analisis', fontsize=14, fontweight='bold')

    # Plot 1: Suhu vs Waktu (Celsius)
    axes[0][0].plot(timestamps, temps_c, 'r-', linewidth=1.5)
    axes[0][0].fill_between(timestamps, min(temps_c) - 1, temps_c,
                            alpha=0.2, color='red')
    axes[0][0].axhline(y=80, color='red', linestyle='--', alpha=0.5, label='Overheat (80°C)')
    axes[0][0].axhline(y=45, color='orange', linestyle='--', alpha=0.5, label='Warm (45°C)')
    axes[0][0].set_xlabel('Waktu (detik)')
    axes[0][0].set_ylabel('Suhu (°C)')
    axes[0][0].set_title('Suhu Chip vs Waktu')
    axes[0][0].legend()
    axes[0][0].grid(True, alpha=0.3)

    # Plot 2: Suhu vs Waktu (Fahrenheit)
    axes[0][1].plot(timestamps, temps_f, 'b-', linewidth=1.5)
    axes[0][1].set_xlabel('Waktu (detik)')
    axes[0][1].set_ylabel('Suhu (°F)')
    axes[0][1].set_title('Suhu Chip vs Waktu (Fahrenheit)')
    axes[0][1].grid(True, alpha=0.3)

    # Plot 3: Histogram distribusi suhu
    axes[1][0].hist(temps_c, bins=20, color='salmon', edgecolor='black', alpha=0.7)
    axes[1][0].set_xlabel('Suhu (°C)')
    axes[1][0].set_ylabel('Frekuensi')
    axes[1][0].set_title(f'Distribusi Suhu (N={len(temps_c)})')
    axes[1][0].grid(True, alpha=0.3)
    stats_text = (f'Mean: {np.mean(temps_c):.2f}°C\n'
                  f'Std: {np.std(temps_c):.2f}°C\n'
                  f'Min: {min(temps_c):.2f}°C\n'
                  f'Max: {max(temps_c):.2f}°C')
    axes[1][0].text(0.98, 0.98, stats_text, transform=axes[1][0].transAxes,
                    fontsize=10, verticalalignment='top', horizontalalignment='right',
                    bbox=dict(boxstyle='round', facecolor='wheat', alpha=0.5))

    # Plot 4: Rate of change
    if len(temps_c) > 1:
        dt = np.diff(timestamps)
        dtemp = np.diff(temps_c)
        rate = [dtemp[i] / dt[i] if dt[i] > 0 else 0 for i in range(len(dt))]
        axes[1][1].plot(timestamps[1:], rate, 'g-', linewidth=1)
        axes[1][1].axhline(y=0, color='black', linestyle='-', alpha=0.3)
        axes[1][1].set_xlabel('Waktu (detik)')
        axes[1][1].set_ylabel('Laju Perubahan (°C/s)')
        axes[1][1].set_title('Laju Perubahan Suhu')
        axes[1][1].grid(True, alpha=0.3)

    plt.tight_layout()
    plot_file = output_file.replace('.csv', '_plot.png')
    plt.savefig(plot_file, dpi=150)
    print(f"[INFO] Grafik disimpan ke: {plot_file}")
    plt.show()


def main():
    args = parse_args()

    print("=" * 60)
    print("  Internal Temperature Sensor - Debug & Analysis Tool")
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
