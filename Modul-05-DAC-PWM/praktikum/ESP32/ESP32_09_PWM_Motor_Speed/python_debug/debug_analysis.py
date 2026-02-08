#!/usr/bin/env python3
"""
Debug & Analysis Script - PWM Motor Speed Control
Modul 05 - DAC & PWM | Praktikum Sistem Embedded

Membaca data kecepatan motor DC dari ESP32 dan memvisualisasikan.
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
        description='PWM Motor Speed - Debug & Analysis Tool'
    )
    parser.add_argument('-p', '--port', default='/dev/ttyUSB0',
                        help='Serial port (default: /dev/ttyUSB0)')
    parser.add_argument('-b', '--baud', type=int, default=115200,
                        help='Baud rate (default: 115200)')
    parser.add_argument('-d', '--duration', type=int, default=60,
                        help='Durasi pengambilan data dalam detik (default: 60)')
    parser.add_argument('-o', '--output', default='pwm_motor_speed.csv',
                        help='Nama file output CSV')
    return parser.parse_args()


def parse_line(line):
    """Parse output: DATA:<arah>,<kecepatan>,<siklus>"""
    match = re.search(r'DATA:(\w+),(\d+),(\d+)', line)
    if match:
        return {
            'direction': match.group(1),
            'speed': int(match.group(2)),
            'cycle': int(match.group(3))
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
        writer.writerow(['timestamp', 'direction', 'speed_pct', 'cycle'])

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
                                parsed['direction'],
                                parsed['speed'],
                                parsed['cycle']
                            ])
            except Exception as e:
                print(f"[ERROR] {e}")
                break

    print("-" * 60)
    print(f"[INFO] Total data terkumpul: {len(data)} sampel")
    return timestamps, data


def plot_data(timestamps, data):
    """Visualisasi kontrol kecepatan motor."""
    if not HAS_MATPLOTLIB or not data:
        return

    # Konversi kecepatan: maju positif, mundur negatif
    speeds = []
    for d in data:
        if d['direction'] == 'MUNDUR':
            speeds.append(-d['speed'])
        else:
            speeds.append(d['speed'])

    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(12, 8))
    fig.suptitle('PWM Motor Speed Control - Analisis', fontsize=14)

    # Plot kecepatan vs waktu (positif=maju, negatif=mundur)
    colors = []
    for d in data:
        if d['direction'] == 'MAJU':
            colors.append('green')
        elif d['direction'] == 'MUNDUR':
            colors.append('red')
        else:
            colors.append('gray')

    ax1.scatter(timestamps, speeds, c=colors, s=30, zorder=5)
    ax1.plot(timestamps, speeds, 'k-', alpha=0.3)
    ax1.set_xlabel('Waktu (detik)')
    ax1.set_ylabel('Kecepatan (%)')
    ax1.set_title('Kecepatan Motor vs Waktu (Hijau=Maju, Merah=Mundur)')
    ax1.axhline(y=0, color='black', linewidth=2)
    ax1.grid(True, alpha=0.3)

    # Plot distribusi arah
    directions = [d['direction'] for d in data]
    dir_counts = {}
    for d in directions:
        dir_counts[d] = dir_counts.get(d, 0) + 1

    ax2.bar(dir_counts.keys(), dir_counts.values(),
            color=['green', 'red', 'gray', 'orange'])
    ax2.set_xlabel('Arah')
    ax2.set_ylabel('Jumlah Sampel')
    ax2.set_title('Distribusi Arah Motor')
    ax2.grid(True, alpha=0.3, axis='y')

    plt.tight_layout()
    plt.savefig('pwm_motor_speed.png', dpi=150)
    print("[INFO] Grafik disimpan: pwm_motor_speed.png")
    plt.show()


def main():
    args = parse_args()

    print("=" * 60)
    print("  PWM Motor Speed - Debug & Analysis Tool")
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
            directions = set(d['direction'] for d in data)
            print(f"  Arah tercatat     : {', '.join(directions)}")
            plot_data(timestamps, data)

    except serial.SerialException as e:
        print(f"[ERROR] Gagal membuka serial port: {e}")
    except KeyboardInterrupt:
        print("\n[INFO] Dihentikan oleh pengguna")


if __name__ == '__main__':
    main()
