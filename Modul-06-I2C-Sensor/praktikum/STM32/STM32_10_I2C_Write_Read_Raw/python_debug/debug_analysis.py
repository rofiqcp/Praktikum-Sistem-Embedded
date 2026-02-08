"""
==========================================================
 Modul 06 - STM32_10_I2C_Write_Read_Raw
 Debug & Analisis: Parser serial, logging CSV, plot
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


def parse_serial(port='/dev/ttyUSB0', baudrate=115200, csv_file='raw_i2c_log.csv'):
    """Membaca data raw I2C dan menyimpan ke CSV"""
    print(f"[INFO] Membuka port {port} @ {baudrate} baud")

    try:
        ser = serial.Serial(port, baudrate, timeout=2)
        time.sleep(2)
        print("[INFO] Menunggu data...")

        with open(csv_file, 'w', newline='') as f:
            writer = csv.writer(f)
            writer.writerow(['timestamp', 'iterasi', 'ax_raw', 'ay_raw', 'az_raw', 'who_am_i'])

            while True:
                line = ser.readline().decode('utf-8', errors='replace').strip()
                if not line:
                    continue
                print(f"[SERIAL] {line}")

                match = re.search(r'DATA,(\d+),([-\d]+),([-\d]+),([-\d]+),(0x[0-9A-Fa-f]+)', line)
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


def plot_hasil(csv_file='raw_i2c_log.csv'):
    """Plot data raw akselerometer"""
    if not HAS_MATPLOTLIB:
        print("[ERROR] matplotlib diperlukan")
        return

    iters, ax_list, ay_list, az_list = [], [], [], []
    try:
        with open(csv_file, 'r') as f:
            reader = csv.DictReader(f)
            for row in reader:
                iters.append(int(row['iterasi']))
                ax_list.append(int(row['ax_raw']))
                ay_list.append(int(row['ay_raw']))
                az_list.append(int(row['az_raw']))
    except FileNotFoundError:
        print(f"[ERROR] File {csv_file} tidak ditemukan")
        return

    fig, ax = plt.subplots(figsize=(10, 6))
    ax.plot(iters, ax_list, 'r-', label='AX raw')
    ax.plot(iters, ay_list, 'g-', label='AY raw')
    ax.plot(iters, az_list, 'b-', label='AZ raw')
    ax.set_xlabel('Iterasi')
    ax.set_ylabel('Raw Value')
    ax.set_title('I2C Raw Read - Akselerometer Data')
    ax.legend()
    ax.grid(True, alpha=0.3)

    plt.tight_layout()
    plt.savefig('raw_i2c_plot.png', dpi=150)
    print("[INFO] Plot disimpan ke raw_i2c_plot.png")
    plt.show()


if __name__ == '__main__':
    if len(sys.argv) > 1 and sys.argv[1] == 'plot':
        csv_f = sys.argv[2] if len(sys.argv) > 2 else 'raw_i2c_log.csv'
        plot_hasil(csv_f)
    else:
        port = sys.argv[1] if len(sys.argv) > 1 else '/dev/ttyUSB0'
        parse_serial(port=port)
