#!/usr/bin/env python3
"""
Debug & Analysis Script - ADC Battery Monitor
Modul 04 - ADC | Praktikum Sistem Embedded

Script ini memvisualisasikan status baterai dari waktu ke waktu,
termasuk tegangan, persentase, dan kurva discharge.
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
        description='ADC Battery Monitor - Debug & Analysis Tool'
    )
    parser.add_argument('-p', '--port', default='/dev/ttyUSB0',
                        help='Serial port (default: /dev/ttyUSB0)')
    parser.add_argument('-b', '--baud', type=int, default=115200,
                        help='Baud rate (default: 115200)')
    parser.add_argument('-d', '--duration', type=int, default=60,
                        help='Durasi pengambilan data (detik)')
    parser.add_argument('-o', '--output', default='adc_battery.csv',
                        help='Nama file output CSV')
    return parser.parse_args()


def parse_line(line):
    """Parse: [0001] Raw: 2048 | V_ADC: 1650 mV | V_Batt: 3300 mV (3.300 V) | 25% SEDANG"""
    match = re.search(
        r'\[(\d+)\]\s+Raw:\s*(\d+)\s*\|\s*V_ADC:\s*(\d+)\s*mV\s*\|'
        r'\s*V_Batt:\s*(\d+)\s*mV.*?\|\s*(\d+)%\s*(\w+)',
        line
    )
    if match:
        return {
            'counter': int(match.group(1)),
            'raw': int(match.group(2)),
            'v_adc': int(match.group(3)),
            'v_batt': int(match.group(4)),
            'percent': int(match.group(5)),
            'status': match.group(6).strip()
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
        writer.writerow(['timestamp', 'counter', 'raw', 'v_adc_mV',
                         'v_batt_mV', 'percent', 'status'])

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
                                parsed['raw'], parsed['v_adc'],
                                parsed['v_batt'], parsed['percent'],
                                parsed['status']
                            ])
            except Exception as e:
                print(f"[ERROR] {e}")

    print("-" * 90)
    print(f"[INFO] Total data: {len(data)} sampel")
    return timestamps, data


def plot_data(timestamps, data, output_file):
    """Buat grafik monitoring baterai."""
    if not HAS_MATPLOTLIB or not data:
        return

    v_batt = [d['v_batt'] / 1000.0 for d in data]  # Convert to V
    percent = [d['percent'] for d in data]

    fig, axes = plt.subplots(2, 1, figsize=(14, 8))
    fig.suptitle('Battery Monitor - Analisis', fontsize=14, fontweight='bold')

    # Plot 1: Tegangan baterai vs waktu
    axes[0].plot(timestamps, v_batt, 'b-', linewidth=1.5)
    axes[0].axhline(y=4.2, color='green', linestyle='--', alpha=0.5, label='Full (4.2V)')
    axes[0].axhline(y=3.7, color='orange', linestyle='--', alpha=0.5, label='Nominal (3.7V)')
    axes[0].axhline(y=3.0, color='red', linestyle='--', alpha=0.5, label='Empty (3.0V)')
    axes[0].fill_between(timestamps, 3.0, v_batt, alpha=0.1, color='blue')
    axes[0].set_xlabel('Waktu (detik)')
    axes[0].set_ylabel('Tegangan Baterai (V)')
    axes[0].set_title('Tegangan Baterai vs Waktu')
    axes[0].legend()
    axes[0].grid(True, alpha=0.3)
    axes[0].set_ylim(2.5, 4.5)

    # Plot 2: Persentase baterai vs waktu
    axes[1].plot(timestamps, percent, 'g-', linewidth=1.5)
    axes[1].fill_between(timestamps, 0, percent, alpha=0.2, color='green')
    axes[1].axhline(y=20, color='red', linestyle='--', alpha=0.5, label='Low Battery (20%)')
    axes[1].set_xlabel('Waktu (detik)')
    axes[1].set_ylabel('Baterai (%)')
    axes[1].set_title('Persentase Baterai vs Waktu')
    axes[1].legend()
    axes[1].grid(True, alpha=0.3)
    axes[1].set_ylim(-5, 105)

    plt.tight_layout()
    plot_file = output_file.replace('.csv', '_plot.png')
    plt.savefig(plot_file, dpi=150)
    print(f"[INFO] Grafik disimpan ke: {plot_file}")
    plt.show()


def main():
    args = parse_args()

    print("=" * 60)
    print("  ADC Battery Monitor - Debug & Analysis Tool")
    print("=" * 60)

    try:
        ser = serial.Serial(args.port, args.baud, timeout=1)
        time.sleep(2)
        ser.flushInput()

        timestamps, data = collect_data(ser, args.duration, args.output)
        ser.close()

        if data and HAS_MATPLOTLIB:
            v_batt = [d['v_batt'] for d in data]
            print(f"\n[STATISTIK BATERAI]")
            print(f"  Tegangan rata-rata: {np.mean(v_batt):.0f} mV ({np.mean(v_batt)/1000:.3f} V)")
            print(f"  Tegangan min      : {min(v_batt)} mV")
            print(f"  Tegangan max      : {max(v_batt)} mV")
            plot_data(timestamps, data, args.output)

    except serial.SerialException as e:
        print(f"[ERROR] Gagal membuka port serial: {e}")
    except KeyboardInterrupt:
        print("\n[INFO] Dihentikan oleh pengguna.")


if __name__ == '__main__':
    main()
