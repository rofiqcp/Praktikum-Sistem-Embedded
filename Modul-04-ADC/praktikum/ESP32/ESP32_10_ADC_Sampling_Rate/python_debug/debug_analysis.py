#!/usr/bin/env python3
"""
Debug & Analysis Script - ADC Sampling Rate
Modul 04 - ADC | Praktikum Sistem Embedded

Script ini menganalisis dan memvisualisasikan hasil pengukuran sampling rate ADC.
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
        description='ADC Sampling Rate - Debug & Analysis Tool'
    )
    parser.add_argument('-p', '--port', default='/dev/ttyUSB0',
                        help='Serial port (default: /dev/ttyUSB0)')
    parser.add_argument('-b', '--baud', type=int, default=115200,
                        help='Baud rate (default: 115200)')
    parser.add_argument('-d', '--duration', type=int, default=60,
                        help='Durasi pengambilan data (detik)')
    parser.add_argument('-o', '--output', default='adc_sampling_rate.csv',
                        help='Nama file output CSV')
    return parser.parse_args()


def parse_line(line):
    """Parse:   11 dB   |    120000   |       83333   |     12.00   | 2048.0"""
    match = re.search(
        r'([\d.]+\s*dB)\s*\|\s*(\d+)\s*\|\s*([\d.]+)\s*\|\s*([\d.]+)\s*\|\s*([\d.]+)',
        line
    )
    if match:
        return {
            'atten': match.group(1).strip(),
            'time_us': int(match.group(2)),
            'rate_sps': float(match.group(3)),
            'us_per_sample': float(match.group(4)),
            'avg_adc': float(match.group(5))
        }
    return None


def parse_summary(line):
    """Parse:   >> 11 dB   Rata-rata: 83333 sps | ..."""
    match = re.search(
        r'>>\s*([\d.]+\s*dB)\s*Rata-rata:\s*([\d.]+)\s*sps\s*\|\s*Terbaik:\s*([\d.]+)\s*sps\s*\|\s*Terburuk:\s*([\d.]+)',
        line
    )
    if match:
        return {
            'atten': match.group(1).strip(),
            'avg_rate': float(match.group(2)),
            'best_rate': float(match.group(3)),
            'worst_rate': float(match.group(4))
        }
    return None


def collect_data(ser, duration, output_file):
    """Kumpulkan data dari serial port."""
    data = []
    summaries = []
    start_time = time.time()

    print(f"\n[INFO] Mengumpulkan data selama {duration} detik...")
    print("-" * 80)

    with open(output_file, 'w', newline='') as csvfile:
        writer = csv.writer(csvfile)
        writer.writerow(['timestamp', 'attenuation', 'time_us', 'rate_sps',
                         'us_per_sample', 'avg_adc'])

        while (time.time() - start_time) < duration:
            try:
                if ser.in_waiting:
                    line = ser.readline().decode('utf-8', errors='ignore').strip()
                    if line:
                        print(f"  {line}")
                        parsed = parse_line(line)
                        if parsed:
                            elapsed = time.time() - start_time
                            data.append(parsed)
                            writer.writerow([
                                f"{elapsed:.3f}", parsed['atten'],
                                parsed['time_us'], parsed['rate_sps'],
                                parsed['us_per_sample'], parsed['avg_adc']
                            ])
                        summary = parse_summary(line)
                        if summary:
                            summaries.append(summary)
            except Exception as e:
                print(f"[ERROR] {e}")

    print("-" * 80)
    print(f"[INFO] Total data: {len(data)} pengukuran, {len(summaries)} ringkasan")
    return data, summaries


def plot_data(data, summaries, output_file):
    """Buat grafik sampling rate."""
    if not HAS_MATPLOTLIB or not data:
        return

    # Kelompokkan data per atenuasi
    atten_groups = {}
    for d in data:
        key = d['atten']
        if key not in atten_groups:
            atten_groups[key] = []
        atten_groups[key].append(d['rate_sps'])

    fig, axes = plt.subplots(1, 2, figsize=(14, 6))
    fig.suptitle('ADC Sampling Rate - Analisis', fontsize=14, fontweight='bold')

    # Plot 1: Box plot per atenuasi
    labels = list(atten_groups.keys())
    values = [atten_groups[k] for k in labels]
    bp = axes[0].boxplot(values, labels=labels, patch_artist=True)
    colors = ['lightblue', 'lightgreen', 'lightyellow', 'lightsalmon']
    for patch, color in zip(bp['boxes'], colors[:len(labels)]):
        patch.set_facecolor(color)
    axes[0].set_xlabel('Atenuasi')
    axes[0].set_ylabel('Sampling Rate (sps)')
    axes[0].set_title('Sampling Rate per Atenuasi')
    axes[0].grid(True, alpha=0.3, axis='y')

    # Plot 2: Bar chart rata-rata
    if summaries:
        atten_labels = [s['atten'] for s in summaries]
        avg_rates = [s['avg_rate'] for s in summaries]
        best_rates = [s['best_rate'] for s in summaries]
        worst_rates = [s['worst_rate'] for s in summaries]

        x = np.arange(len(atten_labels))
        width = 0.25
        axes[1].bar(x - width, worst_rates, width, label='Terburuk', color='salmon')
        axes[1].bar(x, avg_rates, width, label='Rata-rata', color='steelblue')
        axes[1].bar(x + width, best_rates, width, label='Terbaik', color='lightgreen')
        axes[1].set_xlabel('Atenuasi')
        axes[1].set_ylabel('Sampling Rate (sps)')
        axes[1].set_title('Perbandingan Sampling Rate')
        axes[1].set_xticks(x)
        axes[1].set_xticklabels(atten_labels)
        axes[1].legend()
        axes[1].grid(True, alpha=0.3, axis='y')
    else:
        # Fallback jika tidak ada summary
        for label, vals in atten_groups.items():
            axes[1].hist(vals, bins=15, alpha=0.5, label=label)
        axes[1].set_xlabel('Sampling Rate (sps)')
        axes[1].set_ylabel('Frekuensi')
        axes[1].set_title('Distribusi Sampling Rate')
        axes[1].legend()
        axes[1].grid(True, alpha=0.3)

    plt.tight_layout()
    plot_file = output_file.replace('.csv', '_plot.png')
    plt.savefig(plot_file, dpi=150)
    print(f"[INFO] Grafik disimpan ke: {plot_file}")
    plt.show()


def main():
    args = parse_args()

    print("=" * 60)
    print("  ADC Sampling Rate - Debug & Analysis Tool")
    print("=" * 60)

    try:
        ser = serial.Serial(args.port, args.baud, timeout=1)
        time.sleep(2)
        ser.flushInput()

        data, summaries = collect_data(ser, args.duration, args.output)
        ser.close()

        if data:
            plot_data(data, summaries, args.output)

    except serial.SerialException as e:
        print(f"[ERROR] Gagal membuka port serial: {e}")
    except KeyboardInterrupt:
        print("\n[INFO] Dihentikan oleh pengguna.")


if __name__ == '__main__':
    main()
