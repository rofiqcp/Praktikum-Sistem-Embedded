"""
==========================================================
 Modul 06 - STM32_12_I2C_Error_Recovery
 Debug & Analisis: Parser serial, logging CSV, plot statistik error
==========================================================
"""

import serial
import csv
import re
import sys
import time
from datetime import datetime

try:
    import matplotlib.pyplot as plt
    import numpy as np
    HAS_MATPLOTLIB = True
except ImportError:
    HAS_MATPLOTLIB = False
    print("[WARN] matplotlib tidak tersedia")


def parse_serial(port='/dev/ttyUSB0', baudrate=115200, csv_file='error_recovery_log.csv'):
    """Membaca data error recovery dan menyimpan ke CSV"""
    print(f"[INFO] Membuka port {port} @ {baudrate} baud")

    try:
        ser = serial.Serial(port, baudrate, timeout=2)
        time.sleep(2)
        print("[INFO] Menunggu data error recovery...")

        with open(csv_file, 'w', newline='') as f:
            writer = csv.writer(f)
            writer.writerow(['timestamp', 'type', 'reading', 'status',
                             'ax', 'ay', 'az', 'extra'])

            while True:
                line = ser.readline().decode('utf-8', errors='replace').strip()
                if not line:
                    continue
                print(f"[SERIAL] {line}")

                # Parse DATA lines
                match_data = re.search(r'DATA,(\d+),(OK|FAIL),([\d.-]+),([\d.-]+),([\d.-]+)', line)
                if match_data:
                    ts = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
                    writer.writerow([ts, 'DATA', match_data.group(1),
                                     match_data.group(2), match_data.group(3),
                                     match_data.group(4), match_data.group(5), ''])
                    f.flush()

                # Parse STATS lines
                match_stats = re.search(r'STATS,(\d+),(\d+),(\d+),(\d+),(\d+),(\d+),(\d+),(\d+),([\d.]+)', line)
                if match_stats:
                    ts = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
                    writer.writerow([ts, 'STATS', match_stats.group(1),
                                     match_stats.group(9),
                                     match_stats.group(2), match_stats.group(3),
                                     match_stats.group(6), match_stats.group(7)])
                    f.flush()
                    print(f"[STATS] Total={match_stats.group(1)}, "
                          f"Sukses={match_stats.group(2)}, "
                          f"Rate={match_stats.group(9)}%")

    except serial.SerialException as e:
        print(f"[ERROR] {e}")
    except KeyboardInterrupt:
        print("\n[INFO] Dihentikan oleh pengguna")
    finally:
        if 'ser' in locals() and ser.is_open:
            ser.close()


def plot_hasil(csv_file='error_recovery_log.csv'):
    """Plot statistik error dan success rate"""
    if not HAS_MATPLOTLIB:
        print("[ERROR] matplotlib diperlukan")
        return

    readings = []
    statuses = []
    try:
        with open(csv_file, 'r') as f:
            reader = csv.DictReader(f)
            for row in reader:
                if row['type'] == 'DATA':
                    readings.append(int(row['reading']))
                    statuses.append(1 if row['status'] == 'OK' else 0)
    except FileNotFoundError:
        print(f"[ERROR] File {csv_file} tidak ditemukan")
        return

    if not readings:
        print("[WARN] Tidak ada data untuk diplot")
        return

    fig, axes = plt.subplots(2, 1, figsize=(12, 8))

    # Plot 1: Status per pembacaan
    colors = ['green' if s == 1 else 'red' for s in statuses]
    axes[0].bar(readings, [1]*len(readings), color=colors, edgecolor='none')
    axes[0].set_ylabel('Status')
    axes[0].set_title('I2C Error Recovery - Status per Pembacaan')
    axes[0].set_yticks([0, 1])
    axes[0].set_yticklabels(['FAIL', 'OK'])

    # Tambah legend manual
    from matplotlib.patches import Patch
    legend_elements = [Patch(facecolor='green', label='Sukses'),
                       Patch(facecolor='red', label='Gagal')]
    axes[0].legend(handles=legend_elements)

    # Plot 2: Running success rate
    window = 10
    if len(statuses) >= window:
        running_rate = []
        for i in range(len(statuses)):
            start = max(0, i - window + 1)
            running_rate.append(sum(statuses[start:i+1]) / (i - start + 1) * 100)
        axes[1].plot(readings, running_rate, 'b-', linewidth=2, label=f'Success Rate (window={window})')
        axes[1].axhline(y=100, color='g', linestyle='--', alpha=0.5, label='100%')
        axes[1].axhline(y=90, color='orange', linestyle='--', alpha=0.5, label='90%')
    else:
        total_rate = sum(statuses) / len(statuses) * 100 if statuses else 0
        axes[1].axhline(y=total_rate, color='b', linewidth=2, label=f'Success Rate: {total_rate:.1f}%')

    axes[1].set_xlabel('Pembacaan #')
    axes[1].set_ylabel('Success Rate (%)')
    axes[1].set_title('I2C Error Recovery - Running Success Rate')
    axes[1].set_ylim(0, 105)
    axes[1].legend()
    axes[1].grid(True, alpha=0.3)

    plt.suptitle('STM32 I2C Error Recovery Analysis', fontsize=14, fontweight='bold')
    plt.tight_layout()
    plt.savefig('error_recovery_plot.png', dpi=150)
    print("[INFO] Plot disimpan ke error_recovery_plot.png")
    plt.show()


if __name__ == '__main__':
    if len(sys.argv) > 1 and sys.argv[1] == 'plot':
        csv_f = sys.argv[2] if len(sys.argv) > 2 else 'error_recovery_log.csv'
        plot_hasil(csv_f)
    else:
        port = sys.argv[1] if len(sys.argv) > 1 else '/dev/ttyUSB0'
        parse_serial(port=port)
