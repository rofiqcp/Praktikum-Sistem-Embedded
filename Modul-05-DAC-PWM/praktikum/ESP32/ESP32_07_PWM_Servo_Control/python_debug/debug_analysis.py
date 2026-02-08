#!/usr/bin/env python3
"""
Debug & Analysis Script - PWM Servo Control
Modul 05 - DAC & PWM | Praktikum Sistem Embedded

Membaca data posisi servo dari ESP32 dan memvisualisasikan gerakan sweep.
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
        description='PWM Servo Control - Debug & Analysis Tool'
    )
    parser.add_argument('-p', '--port', default='/dev/ttyUSB0',
                        help='Serial port (default: /dev/ttyUSB0)')
    parser.add_argument('-b', '--baud', type=int, default=115200,
                        help='Baud rate (default: 115200)')
    parser.add_argument('-d', '--duration', type=int, default=30,
                        help='Durasi pengambilan data dalam detik (default: 30)')
    parser.add_argument('-o', '--output', default='pwm_servo_control.csv',
                        help='Nama file output CSV')
    return parser.parse_args()


def parse_line(line):
    """Parse output: DATA:<sudut>,<duty>,<pulse_us>,<siklus>"""
    match = re.search(r'DATA:(\d+),(\d+),(\d+),(\d+)', line)
    if match:
        return {
            'angle': int(match.group(1)),
            'duty': int(match.group(2)),
            'pulse_us': int(match.group(3)),
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
        writer.writerow(['timestamp', 'angle', 'duty', 'pulse_us', 'cycle'])

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
                                parsed['angle'],
                                parsed['duty'],
                                parsed['pulse_us'],
                                parsed['cycle']
                            ])
            except Exception as e:
                print(f"[ERROR] {e}")
                break

    print("-" * 60)
    print(f"[INFO] Total data terkumpul: {len(data)} sampel")
    return timestamps, data


def plot_data(timestamps, data):
    """Visualisasi gerakan servo."""
    if not HAS_MATPLOTLIB or not data:
        return

    angles = [d['angle'] for d in data]
    pulse_widths = [d['pulse_us'] for d in data]

    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(12, 8))
    fig.suptitle('PWM Servo Control - Analisis', fontsize=14)

    # Plot sudut vs waktu
    ax1.plot(timestamps, angles, 'b-o', markersize=3, linewidth=1.5)
    ax1.set_xlabel('Waktu (detik)')
    ax1.set_ylabel('Sudut (°)')
    ax1.set_title('Posisi Servo vs Waktu')
    ax1.set_ylim(-10, 190)
    ax1.axhline(y=0, color='gray', linestyle='--', alpha=0.3)
    ax1.axhline(y=90, color='gray', linestyle='--', alpha=0.3, label='90°')
    ax1.axhline(y=180, color='gray', linestyle='--', alpha=0.3)
    ax1.grid(True, alpha=0.3)
    ax1.legend()

    # Plot pulse width vs sudut
    ax2.scatter(angles, pulse_widths, c='red', s=20, alpha=0.6)
    ax2.set_xlabel('Sudut (°)')
    ax2.set_ylabel('Pulse Width (µs)')
    ax2.set_title('Linearitas Servo: Pulse Width vs Sudut')
    ax2.grid(True, alpha=0.3)

    # Garis linear ideal
    ideal_angles = np.linspace(0, 180, 100)
    ideal_pulse = 1000 + (ideal_angles / 180) * 1000
    ax2.plot(ideal_angles, ideal_pulse, 'g--', label='Ideal (linear)', alpha=0.7)
    ax2.legend()

    plt.tight_layout()
    plt.savefig('pwm_servo_control.png', dpi=150)
    print("[INFO] Grafik disimpan: pwm_servo_control.png")
    plt.show()


def main():
    args = parse_args()

    print("=" * 60)
    print("  PWM Servo Control - Debug & Analysis Tool")
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
            angles = [d['angle'] for d in data]
            print(f"\n[STATISTIK]")
            print(f"  Total posisi      : {len(data)}")
            print(f"  Sudut min         : {min(angles)}°")
            print(f"  Sudut max         : {max(angles)}°")
            print(f"  Total siklus      : {data[-1]['cycle']}")
            plot_data(timestamps, data)

    except serial.SerialException as e:
        print(f"[ERROR] Gagal membuka serial port: {e}")
    except KeyboardInterrupt:
        print("\n[INFO] Dihentikan oleh pengguna")


if __name__ == '__main__':
    main()
