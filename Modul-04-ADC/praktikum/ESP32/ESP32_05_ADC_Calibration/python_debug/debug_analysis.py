#!/usr/bin/env python3
"""
Debug & Analysis Script - ADC Calibration
Modul 04 - ADC | Praktikum Sistem Embedded

Script ini menganalisis perbedaan antara pembacaan manual dan terkalibrasi,
menghitung error, dan memvisualisasikan kurva linearitas ADC.
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
        description='ADC Calibration - Debug & Analysis Tool'
    )
    parser.add_argument('-p', '--port', default='/dev/ttyUSB0',
                        help='Serial port (default: /dev/ttyUSB0)')
    parser.add_argument('-b', '--baud', type=int, default=115200,
                        help='Baud rate (default: 115200)')
    parser.add_argument('-d', '--duration', type=int, default=60,
                        help='Durasi pengambilan data (detik)')
    parser.add_argument('-o', '--output', default='adc_calibration.csv',
                        help='Nama file output CSV')
    return parser.parse_args()


def parse_line(line):
    """Parse: [0001] | 2048   | 1650.0    |    1660    |  0.60%   | 11dB"""
    match = re.search(
        r'\[(\d+)\]\s*\|\s*(\d+)\s*\|\s*([\d.]+)\s*\|\s*(\d+)\s*\|\s*([\d.]+)%\s*\|\s*(\S+)',
        line
    )
    if match:
        return {
            'counter': int(match.group(1)),
            'raw': int(match.group(2)),
            'voltage_manual': float(match.group(3)),
            'voltage_cal': int(match.group(4)),
            'error_pct': float(match.group(5)),
            'atten': match.group(6)
        }
    return None


def collect_data(ser, duration, output_file):
    """Kumpulkan data dari serial port."""
    data = []
    timestamps = []
    start_time = time.time()

    print(f"\n[INFO] Mengumpulkan data selama {duration} detik...")
    print("[INFO] Putar potensiometer perlahan dari min ke max untuk analisis linearitas")
    print("-" * 80)

    with open(output_file, 'w', newline='') as csvfile:
        writer = csv.writer(csvfile)
        writer.writerow(['timestamp', 'counter', 'raw', 'voltage_manual_mV',
                         'voltage_calibrated_mV', 'error_percent', 'attenuation'])

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
                                parsed['raw'], parsed['voltage_manual'],
                                parsed['voltage_cal'], parsed['error_pct'],
                                parsed['atten']
                            ])
            except Exception as e:
                print(f"[ERROR] {e}")

    print("-" * 80)
    print(f"[INFO] Total data: {len(data)} sampel")
    return timestamps, data


def plot_data(timestamps, data, output_file):
    """Buat grafik analisis kalibrasi."""
    if not HAS_MATPLOTLIB or not data:
        return

    raw = [d['raw'] for d in data]
    v_manual = [d['voltage_manual'] for d in data]
    v_cal = [d['voltage_cal'] for d in data]
    error = [d['error_pct'] for d in data]

    fig, axes = plt.subplots(2, 2, figsize=(14, 10))
    fig.suptitle('ADC Calibration - Analisis Kalibrasi', fontsize=14, fontweight='bold')

    # Plot 1: Raw vs Tegangan (linearitas)
    sorted_pairs = sorted(zip(raw, v_manual, v_cal), key=lambda x: x[0])
    r_sorted, vm_sorted, vc_sorted = zip(*sorted_pairs) if sorted_pairs else ([], [], [])
    axes[0][0].plot(r_sorted, vm_sorted, 'b-', label='Manual', alpha=0.7)
    axes[0][0].plot(r_sorted, vc_sorted, 'r-', label='Kalibrasi', alpha=0.7)
    ideal_v = [r * 3300 / 4095 for r in r_sorted]
    axes[0][0].plot(r_sorted, ideal_v, 'g--', label='Ideal', alpha=0.5)
    axes[0][0].set_xlabel('Nilai Raw ADC')
    axes[0][0].set_ylabel('Tegangan (mV)')
    axes[0][0].set_title('Kurva Linearitas ADC')
    axes[0][0].legend()
    axes[0][0].grid(True, alpha=0.3)

    # Plot 2: Error vs Raw
    axes[0][1].scatter(raw, error, alpha=0.5, s=10, color='red')
    axes[0][1].set_xlabel('Nilai Raw ADC')
    axes[0][1].set_ylabel('Error (%)')
    axes[0][1].set_title('Error Kalibrasi vs Nilai Raw')
    axes[0][1].grid(True, alpha=0.3)

    # Plot 3: Error vs Waktu
    axes[1][0].plot(timestamps, error, 'r-', alpha=0.7)
    axes[1][0].set_xlabel('Waktu (detik)')
    axes[1][0].set_ylabel('Error (%)')
    axes[1][0].set_title('Error Kalibrasi vs Waktu')
    axes[1][0].grid(True, alpha=0.3)
    mean_err = np.mean(error)
    axes[1][0].axhline(y=mean_err, color='blue', linestyle='--',
                       label=f'Rata-rata: {mean_err:.2f}%')
    axes[1][0].legend()

    # Plot 4: Histogram error
    axes[1][1].hist(error, bins=30, color='salmon', edgecolor='black', alpha=0.7)
    axes[1][1].set_xlabel('Error (%)')
    axes[1][1].set_ylabel('Frekuensi')
    axes[1][1].set_title(f'Distribusi Error (mean={mean_err:.2f}%)')
    axes[1][1].grid(True, alpha=0.3)

    plt.tight_layout()
    plot_file = output_file.replace('.csv', '_plot.png')
    plt.savefig(plot_file, dpi=150)
    print(f"[INFO] Grafik disimpan ke: {plot_file}")
    plt.show()


def main():
    args = parse_args()

    print("=" * 60)
    print("  ADC Calibration - Debug & Analysis Tool")
    print("=" * 60)

    try:
        ser = serial.Serial(args.port, args.baud, timeout=1)
        time.sleep(2)
        ser.flushInput()

        timestamps, data = collect_data(ser, args.duration, args.output)
        ser.close()

        if data and HAS_MATPLOTLIB:
            errors = [d['error_pct'] for d in data]
            print(f"\n[STATISTIK ERROR KALIBRASI]")
            print(f"  Rata-rata error: {np.mean(errors):.3f}%")
            print(f"  Max error      : {max(errors):.3f}%")
            print(f"  Min error      : {min(errors):.3f}%")
            print(f"  Std deviasi    : {np.std(errors):.3f}%")
            plot_data(timestamps, data, args.output)

    except serial.SerialException as e:
        print(f"[ERROR] Gagal membuka port serial: {e}")
    except KeyboardInterrupt:
        print("\n[INFO] Dihentikan oleh pengguna.")


if __name__ == '__main__':
    main()
