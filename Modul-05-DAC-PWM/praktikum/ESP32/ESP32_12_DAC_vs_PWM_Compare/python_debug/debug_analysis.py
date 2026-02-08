#!/usr/bin/env python3
"""
Debug & Analysis Script - DAC vs PWM Compare
Modul 05 - DAC & PWM | Praktikum Sistem Embedded

Membaca data perbandingan DAC dan PWM dari ESP32, menghitung error,
dan memvisualisasikan perbandingan kedua metode.
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
        description='DAC vs PWM Compare - Debug & Analysis Tool'
    )
    parser.add_argument('-p', '--port', default='/dev/ttyUSB0',
                        help='Serial port (default: /dev/ttyUSB0)')
    parser.add_argument('-b', '--baud', type=int, default=115200,
                        help='Baud rate (default: 115200)')
    parser.add_argument('-d', '--duration', type=int, default=60,
                        help='Durasi pengambilan data dalam detik (default: 60)')
    parser.add_argument('-o', '--output', default='dac_vs_pwm_compare.csv',
                        help='Nama file output CSV')
    return parser.parse_args()


def parse_data_line(line):
    """Parse output: DATA:<step>,<target>,<mv_a>,<mv_b>,<err_a>,<err_b>,<siklus>"""
    match = re.search(
        r'DATA:(\d+),([\d.]+),([\d.]+),([\d.]+),([-\d.]+),([-\d.]+),(\d+)',
        line
    )
    if match:
        return {
            'step': int(match.group(1)),
            'target_mv': float(match.group(2)),
            'mv_a': float(match.group(3)),
            'mv_b': float(match.group(4)),
            'err_a': float(match.group(5)),
            'err_b': float(match.group(6)),
            'cycle': int(match.group(7))
        }
    return None


def parse_summary_line(line):
    """Parse output: SUMMARY:<siklus>,<avg_err_a>,<avg_err_b>,<samples>"""
    match = re.search(r'SUMMARY:(\d+),([\d.]+),([\d.]+),(\d+)', line)
    if match:
        return {
            'cycle': int(match.group(1)),
            'avg_err_a': float(match.group(2)),
            'avg_err_b': float(match.group(3)),
            'samples': int(match.group(4))
        }
    return None


def collect_data(ser, duration, output_file):
    """Kumpulkan data dari serial port."""
    data = []
    summaries = []
    timestamps = []
    start_time = time.time()

    print(f"\n[INFO] Mengumpulkan data selama {duration} detik...")
    print(f"[INFO] Menyimpan ke: {output_file}")
    print("-" * 60)

    with open(output_file, 'w', newline='') as csvfile:
        writer = csv.writer(csvfile)
        writer.writerow(['timestamp', 'step', 'target_mv', 'dac_mv',
                         'pwm_mv', 'error_dac', 'error_pwm', 'cycle'])

        while (time.time() - start_time) < duration:
            try:
                if ser.in_waiting:
                    line = ser.readline().decode('utf-8', errors='ignore').strip()
                    if line:
                        print(f"  {line}")

                        parsed = parse_data_line(line)
                        if parsed:
                            elapsed = time.time() - start_time
                            timestamps.append(elapsed)
                            data.append(parsed)
                            writer.writerow([
                                f"{elapsed:.3f}",
                                parsed['step'],
                                parsed['target_mv'],
                                parsed['mv_a'],
                                parsed['mv_b'],
                                parsed['err_a'],
                                parsed['err_b'],
                                parsed['cycle']
                            ])

                        summary = parse_summary_line(line)
                        if summary:
                            summaries.append(summary)
            except Exception as e:
                print(f"[ERROR] {e}")
                break

    print("-" * 60)
    print(f"[INFO] Total data terkumpul: {len(data)} sampel, {len(summaries)} ringkasan")
    return timestamps, data, summaries


def plot_data(timestamps, data, summaries):
    """Visualisasi perbandingan DAC vs PWM."""
    if not HAS_MATPLOTLIB or not data:
        return

    targets = [d['target_mv'] for d in data]
    mv_a = [d['mv_a'] for d in data]
    mv_b = [d['mv_b'] for d in data]
    err_a = [d['err_a'] for d in data]
    err_b = [d['err_b'] for d in data]

    fig, axes = plt.subplots(2, 2, figsize=(14, 10))
    fig.suptitle('DAC vs PWM - Perbandingan Akurasi', fontsize=14)

    # Plot 1: Transfer function (output vs target)
    ax1 = axes[0, 0]
    ax1.plot(targets, mv_a, 'b-o', markersize=3, label='DAC (Metode A)', alpha=0.7)
    ax1.plot(targets, mv_b, 'r-s', markersize=3, label='PWM+RC (Metode B)', alpha=0.7)
    ax1.plot([0, 3300], [0, 3300], 'k--', alpha=0.3, label='Ideal')
    ax1.set_xlabel('Target (mV)')
    ax1.set_ylabel('Terukur (mV)')
    ax1.set_title('Transfer Function')
    ax1.legend()
    ax1.grid(True, alpha=0.3)

    # Plot 2: Error vs target
    ax2 = axes[0, 1]
    ax2.plot(targets, err_a, 'b-o', markersize=3, label='Error DAC', alpha=0.7)
    ax2.plot(targets, err_b, 'r-s', markersize=3, label='Error PWM+RC', alpha=0.7)
    ax2.axhline(y=0, color='k', linewidth=1)
    ax2.set_xlabel('Target (mV)')
    ax2.set_ylabel('Error (mV)')
    ax2.set_title('Error vs Target')
    ax2.legend()
    ax2.grid(True, alpha=0.3)

    # Plot 3: Histogram error
    ax3 = axes[1, 0]
    bins = np.linspace(min(min(err_a), min(err_b)) - 10,
                       max(max(err_a), max(err_b)) + 10, 30)
    ax3.hist(err_a, bins=bins, alpha=0.6, label='DAC', color='blue')
    ax3.hist(err_b, bins=bins, alpha=0.6, label='PWM+RC', color='red')
    ax3.set_xlabel('Error (mV)')
    ax3.set_ylabel('Frekuensi')
    ax3.set_title('Distribusi Error')
    ax3.legend()
    ax3.grid(True, alpha=0.3)

    # Plot 4: Ringkasan perbandingan
    ax4 = axes[1, 1]
    abs_err_a = [abs(e) for e in err_a]
    abs_err_b = [abs(e) for e in err_b]
    methods = ['DAC\n(Metode A)', 'PWM+RC\n(Metode B)']
    means = [np.mean(abs_err_a), np.mean(abs_err_b)]
    maxs = [np.max(abs_err_a), np.max(abs_err_b)]

    x = np.arange(len(methods))
    width = 0.35
    bars1 = ax4.bar(x - width / 2, means, width, label='Rata-rata', color=['blue', 'red'], alpha=0.6)
    bars2 = ax4.bar(x + width / 2, maxs, width, label='Maksimum', color=['blue', 'red'], alpha=0.3)
    ax4.set_ylabel('Error Absolut (mV)')
    ax4.set_title('Ringkasan Error')
    ax4.set_xticks(x)
    ax4.set_xticklabels(methods)
    ax4.legend()
    ax4.grid(True, alpha=0.3, axis='y')

    # Tambahkan nilai di atas bar
    for bar in bars1:
        height = bar.get_height()
        ax4.text(bar.get_x() + bar.get_width() / 2., height,
                 f'{height:.1f}', ha='center', va='bottom', fontsize=9)

    plt.tight_layout()
    plt.savefig('dac_vs_pwm_compare.png', dpi=150)
    print("[INFO] Grafik disimpan: dac_vs_pwm_compare.png")
    plt.show()


def main():
    args = parse_args()

    print("=" * 60)
    print("  DAC vs PWM Compare - Debug & Analysis Tool")
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

        timestamps, data, summaries = collect_data(ser, args.duration, args.output)
        ser.close()

        if data:
            err_a = [abs(d['err_a']) for d in data]
            err_b = [abs(d['err_b']) for d in data]

            print(f"\n[STATISTIK PERBANDINGAN]")
            print(f"  Total sampel          : {len(data)}")
            print(f"  --- Metode A (DAC) ---")
            print(f"    Error rata-rata     : {np.mean(err_a):.1f} mV" if HAS_MATPLOTLIB
                  else f"    Error rata-rata     : {sum(err_a) / len(err_a):.1f} mV")
            print(f"    Error max           : {max(err_a):.1f} mV")
            print(f"  --- Metode B (PWM+RC) ---")
            print(f"    Error rata-rata     : {np.mean(err_b):.1f} mV" if HAS_MATPLOTLIB
                  else f"    Error rata-rata     : {sum(err_b) / len(err_b):.1f} mV")
            print(f"    Error max           : {max(err_b):.1f} mV")
            print(f"  --- Kesimpulan ---")
            avg_a = sum(err_a) / len(err_a)
            avg_b = sum(err_b) / len(err_b)
            if avg_a < avg_b:
                print(f"    DAC lebih akurat ({avg_a:.1f} vs {avg_b:.1f} mV)")
            else:
                print(f"    PWM+RC lebih akurat ({avg_b:.1f} vs {avg_a:.1f} mV)")

            plot_data(timestamps, data, summaries)

    except serial.SerialException as e:
        print(f"[ERROR] Gagal membuka serial port: {e}")
    except KeyboardInterrupt:
        print("\n[INFO] Dihentikan oleh pengguna")


if __name__ == '__main__':
    main()
