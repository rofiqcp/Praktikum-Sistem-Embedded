#!/usr/bin/env python3
"""
Debug & Analysis Script - ADC Statistical Analysis
Modul 04 - ADC | Praktikum Sistem Embedded

Script ini menganalisis data statistik ADC yang dikirim oleh ESP32,
membuat histogram, plot distribusi, dan analisis SNR.
"""

import serial
import time
import csv
import argparse
import re

try:
    import matplotlib.pyplot as plt
    import numpy as np
    from scipy import stats as scipy_stats
    HAS_MATPLOTLIB = True
    HAS_SCIPY = True
except ImportError:
    try:
        import matplotlib.pyplot as plt
        import numpy as np
        HAS_MATPLOTLIB = True
    except ImportError:
        HAS_MATPLOTLIB = False
    HAS_SCIPY = False
    if not HAS_MATPLOTLIB:
        print("[WARN] matplotlib/numpy not installed. Plotting disabled.")
        print("Install with: pip install matplotlib numpy")


def parse_args():
    parser = argparse.ArgumentParser(
        description='ADC Statistical Analysis - Debug & Analysis Tool'
    )
    parser.add_argument('-p', '--port', default='/dev/ttyUSB0',
                        help='Serial port (default: /dev/ttyUSB0)')
    parser.add_argument('-b', '--baud', type=int, default=115200,
                        help='Baud rate (default: 115200)')
    parser.add_argument('-d', '--duration', type=int, default=60,
                        help='Durasi pengambilan data (detik)')
    parser.add_argument('-o', '--output', default='adc_statistics.csv',
                        help='Nama file output CSV')
    return parser.parse_args()


def parse_data_raw(line):
    """Parse: DATA_RAW:2048,2049,2050,..."""
    if line.startswith('DATA_RAW:'):
        try:
            values = [int(x) for x in line[9:].split(',') if x.strip()]
            return values
        except ValueError:
            return None
    return None


def parse_stats_line(line):
    """Parse: STATS:min=2000,max=2100,mean=2050.50,std=15.3000,snr=42.50,median=2050.00"""
    if line.startswith('STATS:'):
        match = re.search(
            r'min=(\d+),max=(\d+),mean=([\d.]+),std=([\d.]+),'
            r'snr=([\d.]+),median=([\d.]+)',
            line
        )
        if match:
            return {
                'min': int(match.group(1)),
                'max': int(match.group(2)),
                'mean': float(match.group(3)),
                'std': float(match.group(4)),
                'snr': float(match.group(5)),
                'median': float(match.group(6))
            }
    return None


def collect_data(ser, duration, output_file):
    """Kumpulkan data dari serial port."""
    all_raw_data = []
    all_stats = []
    start_time = time.time()

    print(f"\n[INFO] Mengumpulkan data selama {duration} detik...")
    print("-" * 80)

    with open(output_file, 'w', newline='') as csvfile:
        writer = csv.writer(csvfile)
        writer.writerow(['batch', 'min', 'max', 'mean', 'std', 'snr', 'median'])

        batch_num = 0

        while (time.time() - start_time) < duration:
            try:
                if ser.in_waiting:
                    line = ser.readline().decode('utf-8', errors='ignore').strip()
                    if line:
                        # Cek apakah baris berisi data mentah
                        raw_data = parse_data_raw(line)
                        if raw_data:
                            all_raw_data.append(raw_data)
                            print(f"  [DATA] Batch {len(all_raw_data)}: "
                                  f"{len(raw_data)} sampel diterima")
                            continue

                        # Cek apakah baris berisi statistik
                        stats = parse_stats_line(line)
                        if stats:
                            batch_num += 1
                            all_stats.append(stats)
                            writer.writerow([
                                batch_num, stats['min'], stats['max'],
                                f"{stats['mean']:.2f}", f"{stats['std']:.4f}",
                                f"{stats['snr']:.2f}", f"{stats['median']:.2f}"
                            ])
                            print(f"  [STATS] Batch {batch_num}: "
                                  f"Mean={stats['mean']:.1f}, "
                                  f"Std={stats['std']:.2f}, "
                                  f"SNR={stats['snr']:.1f} dB")
                            continue

                        # Tampilkan baris lainnya
                        if line and not line.startswith('║') and not line.startswith('╔') \
                           and not line.startswith('╠') and not line.startswith('╚'):
                            print(f"  {line}")

            except Exception as e:
                print(f"[ERROR] {e}")

    print("-" * 80)
    print(f"[INFO] Total batch: {len(all_stats)}, Raw datasets: {len(all_raw_data)}")
    return all_raw_data, all_stats


def plot_data(all_raw_data, all_stats, output_file):
    """Buat grafik analisis statistik."""
    if not HAS_MATPLOTLIB:
        return

    fig, axes = plt.subplots(2, 2, figsize=(14, 10))
    fig.suptitle('ADC Statistical Analysis - Analisis Komprehensif',
                 fontsize=14, fontweight='bold')

    # Plot 1: Histogram gabungan dari semua batch
    if all_raw_data:
        all_values = []
        for batch in all_raw_data:
            all_values.extend(batch)
        
        axes[0][0].hist(all_values, bins=50, color='steelblue',
                        edgecolor='black', alpha=0.7, density=True)
        
        # Overlay distribusi normal jika scipy tersedia
        if HAS_SCIPY and len(all_values) > 10:
            mu, sigma = np.mean(all_values), np.std(all_values)
            x = np.linspace(min(all_values), max(all_values), 100)
            axes[0][0].plot(x, scipy_stats.norm.pdf(x, mu, sigma),
                           'r-', linewidth=2, label=f'Normal (μ={mu:.1f}, σ={sigma:.2f})')
            axes[0][0].legend()
        
        axes[0][0].set_xlabel('Nilai ADC')
        axes[0][0].set_ylabel('Densitas')
        axes[0][0].set_title(f'Distribusi Nilai ADC (N={len(all_values)})')
        axes[0][0].grid(True, alpha=0.3)

    # Plot 2: SNR per batch
    if all_stats:
        batches = list(range(1, len(all_stats) + 1))
        snr_values = [s['snr'] for s in all_stats]
        
        axes[0][1].bar(batches, snr_values, color='lightgreen', edgecolor='black')
        axes[0][1].axhline(y=74, color='red', linestyle='--', alpha=0.5,
                          label='Ideal 12-bit (74 dB)')
        axes[0][1].axhline(y=np.mean(snr_values), color='blue', linestyle='--',
                          alpha=0.5, label=f'Rata-rata ({np.mean(snr_values):.1f} dB)')
        axes[0][1].set_xlabel('Batch')
        axes[0][1].set_ylabel('SNR (dB)')
        axes[0][1].set_title('Signal-to-Noise Ratio per Batch')
        axes[0][1].legend()
        axes[0][1].grid(True, alpha=0.3, axis='y')

    # Plot 3: Mean dan StdDev per batch
    if all_stats:
        means = [s['mean'] for s in all_stats]
        stds = [s['std'] for s in all_stats]
        batches = list(range(1, len(all_stats) + 1))
        
        ax3 = axes[1][0]
        ax3_twin = ax3.twinx()
        
        line1 = ax3.plot(batches, means, 'b-o', markersize=5, label='Mean')
        line2 = ax3_twin.plot(batches, stds, 'r-s', markersize=5, label='StdDev')
        
        ax3.set_xlabel('Batch')
        ax3.set_ylabel('Mean (nilai ADC)', color='blue')
        ax3_twin.set_ylabel('Standar Deviasi', color='red')
        ax3.set_title('Mean dan Standar Deviasi per Batch')
        
        lines = line1 + line2
        labels = [l.get_label() for l in lines]
        ax3.legend(lines, labels, loc='upper right')
        ax3.grid(True, alpha=0.3)

    # Plot 4: Box plot per batch (dari raw data)
    if all_raw_data and len(all_raw_data) > 0:
        # Tampilkan maksimal 10 batch terakhir
        plot_batches = all_raw_data[-10:]
        labels_box = [f'B{i+1}' for i in range(len(plot_batches))]
        bp = axes[1][1].boxplot(plot_batches, labels=labels_box, patch_artist=True)
        colors = plt.cm.viridis(np.linspace(0.2, 0.8, len(plot_batches)))
        for patch, color in zip(bp['boxes'], colors):
            patch.set_facecolor(color)
        axes[1][1].set_xlabel('Batch')
        axes[1][1].set_ylabel('Nilai ADC')
        axes[1][1].set_title('Box Plot per Batch (maks 10 terakhir)')
        axes[1][1].grid(True, alpha=0.3, axis='y')

    plt.tight_layout()
    plot_file = output_file.replace('.csv', '_plot.png')
    plt.savefig(plot_file, dpi=150)
    print(f"[INFO] Grafik disimpan ke: {plot_file}")
    plt.show()


def main():
    args = parse_args()

    print("=" * 60)
    print("  ADC Statistical Analysis - Debug & Analysis Tool")
    print("=" * 60)

    try:
        ser = serial.Serial(args.port, args.baud, timeout=1)
        time.sleep(2)
        ser.flushInput()

        all_raw_data, all_stats = collect_data(ser, args.duration, args.output)
        ser.close()

        if all_stats:
            snr_values = [s['snr'] for s in all_stats]
            std_values = [s['std'] for s in all_stats]
            print(f"\n[RINGKASAN KESELURUHAN]")
            print(f"  Jumlah batch     : {len(all_stats)}")
            print(f"  SNR rata-rata    : {np.mean(snr_values):.2f} dB" if HAS_MATPLOTLIB else "")
            print(f"  StdDev rata-rata : {np.mean(std_values):.4f}" if HAS_MATPLOTLIB else "")
            plot_data(all_raw_data, all_stats, args.output)
        else:
            print("[WARN] Tidak ada data statistik yang berhasil di-parse.")

    except serial.SerialException as e:
        print(f"[ERROR] Gagal membuka port serial: {e}")
    except KeyboardInterrupt:
        print("\n[INFO] Dihentikan oleh pengguna.")


if __name__ == '__main__':
    main()
