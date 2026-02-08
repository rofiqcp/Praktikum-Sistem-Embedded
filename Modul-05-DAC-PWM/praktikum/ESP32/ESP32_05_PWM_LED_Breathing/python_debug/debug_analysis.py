#!/usr/bin/env python3
"""
Debug & Analysis Script - PWM LED Breathing
Modul 05 - DAC & PWM | Praktikum Sistem Embedded

Membaca data efek breathing LED dari ESP32 dan memvisualisasikan.
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
        description='PWM LED Breathing - Debug & Analysis Tool'
    )
    parser.add_argument('-p', '--port', default='/dev/ttyUSB0',
                        help='Serial port (default: /dev/ttyUSB0)')
    parser.add_argument('-b', '--baud', type=int, default=115200,
                        help='Baud rate (default: 115200)')
    parser.add_argument('-d', '--duration', type=int, default=30,
                        help='Durasi pengambilan data dalam detik (default: 30)')
    parser.add_argument('-o', '--output', default='pwm_led_breathing.csv',
                        help='Nama file output CSV')
    return parser.parse_args()


def parse_line(line):
    """Parse output: DATA:<fase>,<target_duty>,<siklus>"""
    match = re.search(r'DATA:(FADE_IN|FADE_OUT),(\d+),(\d+)', line)
    if match:
        return {
            'phase': match.group(1),
            'target_duty': int(match.group(2)),
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
        writer.writerow(['timestamp', 'phase', 'target_duty', 'cycle'])

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
                                parsed['phase'],
                                parsed['target_duty'],
                                parsed['cycle']
                            ])
            except Exception as e:
                print(f"[ERROR] {e}")
                break

    print("-" * 60)
    print(f"[INFO] Total data terkumpul: {len(data)} sampel")
    return timestamps, data


def plot_data(timestamps, data):
    """Visualisasi pola breathing LED."""
    if not HAS_MATPLOTLIB or not data:
        return

    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(12, 8))
    fig.suptitle('PWM LED Breathing - Analisis', fontsize=14)

    # Rekonstruksi profil brightness dari event fade
    t_sim = []
    brightness = []
    for i, d in enumerate(data):
        t_start = timestamps[i]
        if d['phase'] == 'FADE_IN':
            # Simulasi fade in 2 detik
            for j in range(100):
                t_sim.append(t_start + j * 0.02)
                brightness.append(j / 100.0 * 100)
        else:
            # Simulasi fade out 2 detik
            for j in range(100):
                t_sim.append(t_start + j * 0.02)
                brightness.append((100 - j) / 100.0 * 100)

    ax1.plot(t_sim, brightness, 'b-', linewidth=1.5)
    ax1.set_xlabel('Waktu (detik)')
    ax1.set_ylabel('Kecerahan (%)')
    ax1.set_title('Profil Kecerahan LED (Simulasi)')
    ax1.set_ylim(-5, 105)
    ax1.grid(True, alpha=0.3)

    # Plot event fade
    phases = [d['phase'] for d in data]
    colors = ['green' if p == 'FADE_IN' else 'red' for p in phases]
    duties = [d['target_duty'] for d in data]
    ax2.scatter(timestamps, duties, c=colors, s=80, zorder=5,
                edgecolors='black', linewidths=0.5)
    ax2.set_xlabel('Waktu (detik)')
    ax2.set_ylabel('Target Duty')
    ax2.set_title('Event Fade (Hijau=IN, Merah=OUT)')
    ax2.grid(True, alpha=0.3)

    plt.tight_layout()
    plt.savefig('pwm_led_breathing.png', dpi=150)
    print("[INFO] Grafik disimpan: pwm_led_breathing.png")
    plt.show()


def main():
    args = parse_args()

    print("=" * 60)
    print("  PWM LED Breathing - Debug & Analysis Tool")
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
            cycles = [d['cycle'] for d in data]
            print(f"\n[STATISTIK]")
            print(f"  Total event   : {len(data)}")
            print(f"  Total siklus  : {max(cycles)}")
            fade_in_count = sum(1 for d in data if d['phase'] == 'FADE_IN')
            print(f"  Fade IN       : {fade_in_count} kali")
            print(f"  Fade OUT      : {len(data) - fade_in_count} kali")
            plot_data(timestamps, data)

    except serial.SerialException as e:
        print(f"[ERROR] Gagal membuka serial port: {e}")
    except KeyboardInterrupt:
        print("\n[INFO] Dihentikan oleh pengguna")


if __name__ == '__main__':
    main()
