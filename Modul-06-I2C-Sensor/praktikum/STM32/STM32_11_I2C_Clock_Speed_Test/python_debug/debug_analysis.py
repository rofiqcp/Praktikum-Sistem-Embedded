"""
==========================================================
 Modul 06 - STM32_11_I2C_Clock_Speed_Test
 Debug & Analisis: Parser serial, logging CSV, plot perbandingan
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


def parse_serial(port='/dev/ttyUSB0', baudrate=115200, csv_file='speed_test_log.csv'):
    """Membaca data speed test dan menyimpan ke CSV"""
    print(f"[INFO] Membuka port {port} @ {baudrate} baud")

    try:
        ser = serial.Serial(port, baudrate, timeout=2)
        time.sleep(2)
        print("[INFO] Menunggu data speed test...")

        with open(csv_file, 'w', newline='') as f:
            writer = csv.writer(f)
            writer.writerow(['timestamp', 'type', 'speed_khz', 'time_ms',
                             'bytes', 'avg_ms', 'throughput_kbps', 'errors'])

            while True:
                line = ser.readline().decode('utf-8', errors='replace').strip()
                if not line:
                    continue
                print(f"[SERIAL] {line}")

                # Parse SPEED data
                match_speed = re.search(r'SPEED,(\d+),(\d+),(\d+),([\d.]+),([\d.]+),(\d+)', line)
                if match_speed:
                    ts = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
                    writer.writerow([ts, 'SPEED', match_speed.group(1),
                                     match_speed.group(2), match_speed.group(3),
                                     match_speed.group(4), match_speed.group(5),
                                     match_speed.group(6)])
                    f.flush()

                # Parse COMPARE data
                match_cmp = re.search(r'COMPARE,(\d+),(\d+),(\d+),([\d.]+)', line)
                if match_cmp:
                    ts = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
                    writer.writerow([ts, 'COMPARE', match_cmp.group(1),
                                     match_cmp.group(2), match_cmp.group(3),
                                     match_cmp.group(4), '', ''])
                    f.flush()
                    print(f"[LOG] Round #{match_cmp.group(1)}: "
                          f"100k={match_cmp.group(2)}ms, 400k={match_cmp.group(3)}ms, "
                          f"speedup={match_cmp.group(4)}x")

    except serial.SerialException as e:
        print(f"[ERROR] {e}")
    except KeyboardInterrupt:
        print("\n[INFO] Dihentikan oleh pengguna")
    finally:
        if 'ser' in locals() and ser.is_open:
            ser.close()


def plot_hasil(csv_file='speed_test_log.csv'):
    """Plot perbandingan kecepatan I2C"""
    if not HAS_MATPLOTLIB:
        print("[ERROR] matplotlib diperlukan")
        return

    speed_100k, speed_400k, throughput_100k, throughput_400k = [], [], [], []
    try:
        with open(csv_file, 'r') as f:
            reader = csv.DictReader(f)
            for row in reader:
                if row['type'] == 'SPEED':
                    khz = int(row['speed_khz'])
                    t = float(row['time_ms'])
                    tp = float(row['throughput_kbps']) if row['throughput_kbps'] else 0
                    if khz == 100:
                        speed_100k.append(t)
                        throughput_100k.append(tp)
                    elif khz == 400:
                        speed_400k.append(t)
                        throughput_400k.append(tp)
    except FileNotFoundError:
        print(f"[ERROR] File {csv_file} tidak ditemukan")
        return

    fig, axes = plt.subplots(1, 2, figsize=(14, 6))

    # Bar chart: waktu rata-rata
    if speed_100k and speed_400k:
        avg_100 = sum(speed_100k) / len(speed_100k)
        avg_400 = sum(speed_400k) / len(speed_400k)
        bars = axes[0].bar(['100 kHz', '400 kHz'], [avg_100, avg_400],
                          color=['steelblue', 'coral'], edgecolor='navy')
        axes[0].set_ylabel('Waktu Total (ms)')
        axes[0].set_title('Perbandingan Waktu Transaksi I2C')
        for bar, val in zip(bars, [avg_100, avg_400]):
            axes[0].text(bar.get_x() + bar.get_width()/2, bar.get_height() + 1,
                        f'{val:.1f}ms', ha='center', fontweight='bold')
        axes[0].grid(axis='y', alpha=0.3)

    # Bar chart: throughput
    if throughput_100k and throughput_400k:
        avg_tp100 = sum(throughput_100k) / len(throughput_100k)
        avg_tp400 = sum(throughput_400k) / len(throughput_400k)
        bars = axes[1].bar(['100 kHz', '400 kHz'], [avg_tp100, avg_tp400],
                          color=['steelblue', 'coral'], edgecolor='navy')
        axes[1].set_ylabel('Throughput (kbps)')
        axes[1].set_title('Perbandingan Throughput I2C')
        for bar, val in zip(bars, [avg_tp100, avg_tp400]):
            axes[1].text(bar.get_x() + bar.get_width()/2, bar.get_height() + 0.5,
                        f'{val:.1f}', ha='center', fontweight='bold')
        axes[1].grid(axis='y', alpha=0.3)

    plt.suptitle('I2C Speed Test: Standard Mode vs Fast Mode', fontsize=14, fontweight='bold')
    plt.tight_layout()
    plt.savefig('speed_test_plot.png', dpi=150)
    print("[INFO] Plot disimpan ke speed_test_plot.png")
    plt.show()


if __name__ == '__main__':
    if len(sys.argv) > 1 and sys.argv[1] == 'plot':
        csv_f = sys.argv[2] if len(sys.argv) > 2 else 'speed_test_log.csv'
        plot_hasil(csv_f)
    else:
        port = sys.argv[1] if len(sys.argv) > 1 else '/dev/ttyUSB0'
        parse_serial(port=port)
