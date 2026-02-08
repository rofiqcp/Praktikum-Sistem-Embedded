#!/usr/bin/env python3
"""
Debug & Analysis Script - DAC Triangle Wave
Modul 05 - DAC & PWM | Praktikum Sistem Embedded

Membaca data gelombang segitiga dari ESP32 dan memvisualisasikan.
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
    print("[WARN] matplotlib/numpy tidak terinstal. Plotting dinonaktifkan.")
    print("Install dengan: pip install matplotlib numpy")


def parse_args():
    parser = argparse.ArgumentParser(
        description='DAC Triangle Wave - Debug & Analysis Tool'
    )
    parser.add_argument('-p', '--port', default='/dev/ttyUSB0',
                        help='Serial port (default: /dev/ttyUSB0)')
    parser.add_argument('-b', '--baud', type=int, default=115200,
                        help='Baud rate (default: 115200)')
    parser.add_argument('-d', '--duration', type=int, default=30,
                        help='Durasi pengambilan data dalam detik (default: 30)')
    parser.add_argument('-o', '--output', default='dac_triangle_wave.csv',
                        help='Nama file output CSV')
    return parser.parse_args()


def parse_line(line):
    """Parse output: DATA:<nilai>,<arah>,<siklus>"""
    match = re.search(r'DATA:(\d+),(NAIK|TURUN),(\d+)', line)
    if match:
        return {
            'value': int(match.group(1)),
            'direction': match.group(2),
            'cycles': int(match.group(3))
        }
    return None


def collect_data(ser, duration, output_file):
    """Kumpulkan data dari serial port."""
    data = []
    timestamps = []
    start_time = time.time()

    print(f"\n[INFO] Mengumpulkan data selama {duration} detik...")
    print(f"[INFO] Menyimpan ke: {output_file}")
    print("-" * 60)

    with open(output_file, 'w', newline='') as csvfile:
        writer = csv.writer(csvfile)
        writer.writerow(['timestamp', 'value', 'direction', 'cycles'])

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
                                parsed['value'],
                                parsed['direction'],
                                parsed['cycles']
                            ])
            except Exception as e:
                print(f"[ERROR] {e}")
                break

    print("-" * 60)
    print(f"[INFO] Total data terkumpul: {len(data)} sampel")
    return timestamps, data


def plot_data(timestamps, data):
    """Visualisasi data gelombang segitiga."""
    if not HAS_MATPLOTLIB or not data:
        return

    values = [d['value'] for d in data]
    cycles = [d['cycles'] for d in data]

    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(12, 8))
    fig.suptitle('DAC Triangle Wave - Analisis', fontsize=14)

    # Plot nilai DAC vs waktu
    ax1.plot(timestamps, values, 'b-', linewidth=1)
    ax1.set_xlabel('Waktu (detik)')
    ax1.set_ylabel('Nilai DAC (0-255)')
    ax1.set_title('Gelombang Segitiga - Nilai DAC vs Waktu')
    ax1.set_ylim(-10, 265)
    ax1.grid(True, alpha=0.3)

    # Warnai berdasarkan arah
    for i in range(len(data)):
        color = 'green' if data[i]['direction'] == 'NAIK' else 'red'
        ax1.scatter(timestamps[i], values[i], c=color, s=10, zorder=5)

    # Plot siklus kumulatif
    ax2.plot(timestamps, cycles, 'g-o', markersize=4)
    ax2.set_xlabel('Waktu (detik)')
    ax2.set_ylabel('Total Siklus')
    ax2.set_title('Jumlah Siklus Kumulatif')
    ax2.grid(True, alpha=0.3)

    plt.tight_layout()
    plt.savefig('dac_triangle_wave.png', dpi=150)
    print("[INFO] Grafik disimpan: dac_triangle_wave.png")
    plt.show()


def main():
    args = parse_args()

    print("=" * 60)
    print("  DAC Triangle Wave - Debug & Analysis Tool")
    print("  Modul 05 - DAC & PWM")
    print("=" * 60)
    print(f"  Port     : {args.port}")
    print(f"  Baud     : {args.baud}")
    print(f"  Durasi   : {args.duration} detik")
    print(f"  Output   : {args.output}")
    print("=" * 60)

    try:
        ser = serial.Serial(args.port, args.baud, timeout=1)
        time.sleep(2)
        ser.reset_input_buffer()

        timestamps, data = collect_data(ser, args.duration, args.output)
        ser.close()

        if data:
            values = [d['value'] for d in data]
            print(f"\n[STATISTIK]")
            print(f"  Nilai min       : {min(values)}")
            print(f"  Nilai max       : {max(values)}")
            print(f"  Total siklus    : {data[-1]['cycles']}")
            plot_data(timestamps, data)

    except serial.SerialException as e:
        print(f"[ERROR] Gagal membuka serial port: {e}")
    except KeyboardInterrupt:
        print("\n[INFO] Dihentikan oleh pengguna")


if __name__ == '__main__':
    main()
