#!/usr/bin/env python3
"""
Debug & Analysis Script - PWM LED Brightness Control
Modul 05 - DAC & PWM | Praktikum Sistem Embedded

Membaca data kecerahan LED dari ESP32 dan memvisualisasikan.
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
        description='PWM LED Brightness - Debug & Analysis Tool'
    )
    parser.add_argument('-p', '--port', default='/dev/ttyUSB0',
                        help='Serial port (default: /dev/ttyUSB0)')
    parser.add_argument('-b', '--baud', type=int, default=115200,
                        help='Baud rate (default: 115200)')
    parser.add_argument('-d', '--duration', type=int, default=60,
                        help='Durasi pengambilan data dalam detik (default: 60)')
    parser.add_argument('-o', '--output', default='pwm_led_brightness.csv',
                        help='Nama file output CSV')
    return parser.parse_args()


def parse_line(line):
    """Parse output: DATA:<persentase>,<duty>"""
    match = re.search(r'DATA:(\d+),(\d+)', line)
    if match:
        return {
            'percentage': int(match.group(1)),
            'duty': int(match.group(2))
        }
    return None


def collect_data(ser, duration, output_file):
    """Kumpulkan data dari serial port."""
    data = []
    timestamps = []
    start_time = time.time()

    print(f"\n[INFO] Mengumpulkan data selama {duration} detik...")
    print(f"[INFO] Menyimpan ke: {output_file}")
    print(f"[INFO] Ketik persentase (0-100) di serial monitor ESP32")
    print("-" * 60)

    with open(output_file, 'w', newline='') as csvfile:
        writer = csv.writer(csvfile)
        writer.writerow(['timestamp', 'percentage', 'duty'])

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
                                parsed['percentage'],
                                parsed['duty']
                            ])
            except Exception as e:
                print(f"[ERROR] {e}")
                break

    print("-" * 60)
    print(f"[INFO] Total data terkumpul: {len(data)} sampel")
    return timestamps, data


def plot_data(timestamps, data):
    """Visualisasi data kecerahan LED."""
    if not HAS_MATPLOTLIB or not data:
        return

    percentages = [d['percentage'] for d in data]
    duties = [d['duty'] for d in data]

    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(12, 8))
    fig.suptitle('PWM LED Brightness Control - Analisis', fontsize=14)

    # Plot persentase kecerahan
    ax1.step(timestamps, percentages, 'b-', where='post', linewidth=2)
    ax1.scatter(timestamps, percentages, c='blue', s=30, zorder=5)
    ax1.set_xlabel('Waktu (detik)')
    ax1.set_ylabel('Kecerahan (%)')
    ax1.set_title('Kecerahan LED vs Waktu')
    ax1.set_ylim(-5, 105)
    ax1.grid(True, alpha=0.3)

    # Plot duty cycle
    ax2.step(timestamps, duties, 'g-', where='post', linewidth=2)
    ax2.scatter(timestamps, duties, c='green', s=30, zorder=5)
    ax2.set_xlabel('Waktu (detik)')
    ax2.set_ylabel('Duty Cycle (0-8191)')
    ax2.set_title('Duty Cycle PWM vs Waktu')
    ax2.grid(True, alpha=0.3)

    plt.tight_layout()
    plt.savefig('pwm_led_brightness.png', dpi=150)
    print("[INFO] Grafik disimpan: pwm_led_brightness.png")
    plt.show()


def main():
    args = parse_args()

    print("=" * 60)
    print("  PWM LED Brightness - Debug & Analysis Tool")
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
            pcts = [d['percentage'] for d in data]
            print(f"\n[STATISTIK]")
            print(f"  Total perubahan   : {len(data)}")
            print(f"  Kecerahan min     : {min(pcts)}%")
            print(f"  Kecerahan max     : {max(pcts)}%")
            print(f"  Level unik        : {len(set(pcts))}")
            plot_data(timestamps, data)

    except serial.SerialException as e:
        print(f"[ERROR] Gagal membuka serial port: {e}")
    except KeyboardInterrupt:
        print("\n[INFO] Dihentikan oleh pengguna")


if __name__ == '__main__':
    main()
