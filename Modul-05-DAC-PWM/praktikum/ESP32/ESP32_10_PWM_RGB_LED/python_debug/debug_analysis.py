#!/usr/bin/env python3
"""
Debug & Analysis Script - PWM RGB LED Rainbow
Modul 05 - DAC & PWM | Praktikum Sistem Embedded

Membaca data warna RGB dari ESP32 dan memvisualisasikan efek rainbow.
"""

import serial
import time
import csv
import argparse
import re

try:
    import matplotlib.pyplot as plt
    import matplotlib.patches as mpatches
    import numpy as np
    HAS_MATPLOTLIB = True
except ImportError:
    HAS_MATPLOTLIB = False
    print("[WARN] matplotlib/numpy tidak terinstal. Plotting dinonaktifkan.")
    print("Install dengan: pip install matplotlib numpy")


def parse_args():
    parser = argparse.ArgumentParser(
        description='PWM RGB LED - Debug & Analysis Tool'
    )
    parser.add_argument('-p', '--port', default='/dev/ttyUSB0',
                        help='Serial port (default: /dev/ttyUSB0)')
    parser.add_argument('-b', '--baud', type=int, default=115200,
                        help='Baud rate (default: 115200)')
    parser.add_argument('-d', '--duration', type=int, default=30,
                        help='Durasi pengambilan data dalam detik (default: 30)')
    parser.add_argument('-o', '--output', default='pwm_rgb_led.csv',
                        help='Nama file output CSV')
    return parser.parse_args()


def parse_line(line):
    """Parse output: DATA:<hue>,<r>,<g>,<b>,<siklus>"""
    match = re.search(r'DATA:([\d.]+),(\d+),(\d+),(\d+),(\d+)', line)
    if match:
        return {
            'hue': float(match.group(1)),
            'r': int(match.group(2)),
            'g': int(match.group(3)),
            'b': int(match.group(4)),
            'cycle': int(match.group(5))
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
        writer.writerow(['timestamp', 'hue', 'r', 'g', 'b', 'cycle'])

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
                                parsed['hue'],
                                parsed['r'],
                                parsed['g'],
                                parsed['b'],
                                parsed['cycle']
                            ])
            except Exception as e:
                print(f"[ERROR] {e}")
                break

    print("-" * 60)
    print(f"[INFO] Total data terkumpul: {len(data)} sampel")
    return timestamps, data


def plot_data(timestamps, data):
    """Visualisasi efek rainbow RGB."""
    if not HAS_MATPLOTLIB or not data:
        return

    r_vals = [d['r'] for d in data]
    g_vals = [d['g'] for d in data]
    b_vals = [d['b'] for d in data]
    hues = [d['hue'] for d in data]

    fig, (ax1, ax2, ax3) = plt.subplots(3, 1, figsize=(12, 10))
    fig.suptitle('PWM RGB LED Rainbow - Analisis', fontsize=14)

    # Plot komponen R, G, B vs waktu
    ax1.plot(timestamps, r_vals, 'r-', label='Merah', linewidth=1.5)
    ax1.plot(timestamps, g_vals, 'g-', label='Hijau', linewidth=1.5)
    ax1.plot(timestamps, b_vals, 'b-', label='Biru', linewidth=1.5)
    ax1.set_xlabel('Waktu (detik)')
    ax1.set_ylabel('Nilai PWM (0-255)')
    ax1.set_title('Komponen Warna R, G, B vs Waktu')
    ax1.set_ylim(-10, 265)
    ax1.legend()
    ax1.grid(True, alpha=0.3)

    # Plot hue vs waktu
    ax2.plot(timestamps, hues, 'k-', linewidth=1.5)
    ax2.set_xlabel('Waktu (detik)')
    ax2.set_ylabel('Hue (°)')
    ax2.set_title('Hue (Sudut Warna) vs Waktu')
    ax2.set_ylim(-10, 370)
    ax2.grid(True, alpha=0.3)

    # Color bar menunjukkan warna aktual
    for i in range(len(data)):
        color = (r_vals[i] / 255.0, g_vals[i] / 255.0, b_vals[i] / 255.0)
        ax3.axvspan(timestamps[i] - 0.5, timestamps[i] + 0.5,
                    color=color, alpha=0.8)
    ax3.set_xlabel('Waktu (detik)')
    ax3.set_title('Warna Aktual')
    ax3.set_yticks([])

    plt.tight_layout()
    plt.savefig('pwm_rgb_led.png', dpi=150)
    print("[INFO] Grafik disimpan: pwm_rgb_led.png")
    plt.show()


def main():
    args = parse_args()

    print("=" * 60)
    print("  PWM RGB LED Rainbow - Debug & Analysis Tool")
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
            print(f"\n[STATISTIK]")
            print(f"  Total sampel      : {len(data)}")
            print(f"  Total siklus      : {data[-1]['cycle']}")
            print(f"  Range hue         : {min(d['hue'] for d in data):.0f}° - {max(d['hue'] for d in data):.0f}°")
            plot_data(timestamps, data)

    except serial.SerialException as e:
        print(f"[ERROR] Gagal membuka serial port: {e}")
    except KeyboardInterrupt:
        print("\n[INFO] Dihentikan oleh pengguna")


if __name__ == '__main__':
    main()
