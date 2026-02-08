"""
==========================================================
 Modul 06 - STM32_07_I2C_Light_BH1750
 Debug & Analisis: Parser serial, logging CSV, plot cahaya
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
    HAS_MATPLOTLIB = True
except ImportError:
    HAS_MATPLOTLIB = False
    print("[WARN] matplotlib tidak tersedia")


def parse_serial(port='/dev/ttyUSB0', baudrate=115200, csv_file='bh1750_log.csv'):
    """Membaca data BH1750 dan menyimpan ke CSV"""
    print(f"[INFO] Membuka port {port} @ {baudrate} baud")

    try:
        ser = serial.Serial(port, baudrate, timeout=2)
        time.sleep(2)
        print("[INFO] Menunggu data BH1750...")

        with open(csv_file, 'w', newline='') as f:
            writer = csv.writer(f)
            writer.writerow(['timestamp', 'reading', 'lux', 'lux_min', 'lux_max', 'lux_avg'])

            while True:
                line = ser.readline().decode('utf-8', errors='replace').strip()
                if not line:
                    continue
                print(f"[SERIAL] {line}")

                match = re.search(r'DATA,(\d+),([\d.]+),([\d.]+),([\d.]+),([\d.]+)', line)
                if match:
                    ts = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
                    writer.writerow([ts, match.group(1), match.group(2),
                                     match.group(3), match.group(4), match.group(5)])
                    f.flush()

    except serial.SerialException as e:
        print(f"[ERROR] {e}")
    except KeyboardInterrupt:
        print("\n[INFO] Dihentikan oleh pengguna")
    finally:
        if 'ser' in locals() and ser.is_open:
            ser.close()


def plot_hasil(csv_file='bh1750_log.csv'):
    """Plot data intensitas cahaya"""
    if not HAS_MATPLOTLIB:
        print("[ERROR] matplotlib diperlukan")
        return

    readings, lux_list, lux_min, lux_max, lux_avg = [], [], [], [], []
    try:
        with open(csv_file, 'r') as f:
            reader = csv.DictReader(f)
            for row in reader:
                readings.append(int(row['reading']))
                lux_list.append(float(row['lux']))
                lux_min.append(float(row['lux_min']))
                lux_max.append(float(row['lux_max']))
                lux_avg.append(float(row['lux_avg']))
    except FileNotFoundError:
        print(f"[ERROR] File {csv_file} tidak ditemukan")
        return

    fig, ax = plt.subplots(figsize=(12, 6))
    ax.plot(readings, lux_list, 'gold', linewidth=2, label='Lux (saat ini)')
    ax.fill_between(readings, lux_min, lux_max, alpha=0.2, color='orange', label='Min-Max range')
    ax.plot(readings, lux_avg, 'r--', linewidth=1, label='Rata-rata')
    ax.set_xlabel('Pembacaan #')
    ax.set_ylabel('Intensitas Cahaya (lux)')
    ax.set_title('BH1750 - Monitoring Cahaya')
    ax.legend()
    ax.grid(True, alpha=0.3)
    ax.set_yscale('log')

    plt.tight_layout()
    plt.savefig('bh1750_plot.png', dpi=150)
    print("[INFO] Plot disimpan ke bh1750_plot.png")
    plt.show()


if __name__ == '__main__':
    if len(sys.argv) > 1 and sys.argv[1] == 'plot':
        csv_f = sys.argv[2] if len(sys.argv) > 2 else 'bh1750_log.csv'
        plot_hasil(csv_f)
    else:
        port = sys.argv[1] if len(sys.argv) > 1 else '/dev/ttyUSB0'
        parse_serial(port=port)
