#!/usr/bin/env python3
"""
Debug & Analysis Script - PWM Frequency Sweep
Modul 05 - DAC & PWM | Praktikum Sistem Embedded

Membaca data sapuan frekuensi PWM dari ESP32 dan memvisualisasikan.
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
        description='PWM Frequency Sweep - Debug & Analysis Tool'
    )
    parser.add_argument('-p', '--port', default='/dev/ttyUSB0',
                        help='Serial port (default: /dev/ttyUSB0)')
    parser.add_argument('-b', '--baud', type=int, default=115200,
                        help='Baud rate (default: 115200)')
    parser.add_argument('-d', '--duration', type=int, default=60,
                        help='Durasi pengambilan data dalam detik (default: 60)')
    parser.add_argument('-o', '--output', default='pwm_freq_sweep.csv',
                        help='Nama file output CSV')
    return parser.parse_args()


def parse_line(line):
    """Parse output: DATA:<frekuensi>,<step>,<total_step>,<siklus>"""
    match = re.search(r'DATA:(\d+),(\d+),(\d+),(\d+)', line)
    if match:
        return {
            'frequency': int(match.group(1)),
            'step': int(match.group(2)),
            'total_steps': int(match.group(3)),
            'cycle': int(match.group(4))
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
        writer.writerow(['timestamp', 'frequency', 'step', 'total_steps', 'cycle'])

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
                                parsed['frequency'],
                                parsed['step'],
                                parsed['total_steps'],
                                parsed['cycle']
                            ])
            except Exception as e:
                print(f"[ERROR] {e}")
                break

    print("-" * 60)
    print(f"[INFO] Total data terkumpul: {len(data)} sampel")
    return timestamps, data


def plot_data(timestamps, data):
    """Visualisasi sapuan frekuensi."""
    if not HAS_MATPLOTLIB or not data:
        return

    freqs = [d['frequency'] for d in data]
    steps = [d['step'] for d in data]

    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(12, 8))
    fig.suptitle('PWM Frequency Sweep - Analisis', fontsize=14)

    # Plot frekuensi vs waktu (skala linear)
    ax1.plot(timestamps, freqs, 'b-', linewidth=1.5)
    ax1.set_xlabel('Waktu (detik)')
    ax1.set_ylabel('Frekuensi (Hz)')
    ax1.set_title('Frekuensi PWM vs Waktu')
    ax1.grid(True, alpha=0.3)

    # Plot frekuensi vs waktu (skala logaritmik)
    ax2.semilogy(timestamps, freqs, 'r-', linewidth=1.5)
    ax2.set_xlabel('Waktu (detik)')
    ax2.set_ylabel('Frekuensi (Hz) - Log')
    ax2.set_title('Frekuensi PWM vs Waktu (Skala Logaritmik)')
    ax2.grid(True, alpha=0.3, which='both')

    plt.tight_layout()
    plt.savefig('pwm_freq_sweep.png', dpi=150)
    print("[INFO] Grafik disimpan: pwm_freq_sweep.png")
    plt.show()


def main():
    args = parse_args()

    print("=" * 60)
    print("  PWM Frequency Sweep - Debug & Analysis Tool")
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
            freqs = [d['frequency'] for d in data]
            print(f"\n[STATISTIK]")
            print(f"  Frekuensi min     : {min(freqs)} Hz")
            print(f"  Frekuensi max     : {max(freqs)} Hz")
            print(f"  Total langkah     : {len(data)}")
            print(f"  Total siklus      : {data[-1]['cycle']}")
            plot_data(timestamps, data)

    except serial.SerialException as e:
        print(f"[ERROR] Gagal membuka serial port: {e}")
    except KeyboardInterrupt:
        print("\n[INFO] Dihentikan oleh pengguna")


if __name__ == '__main__':
    main()
