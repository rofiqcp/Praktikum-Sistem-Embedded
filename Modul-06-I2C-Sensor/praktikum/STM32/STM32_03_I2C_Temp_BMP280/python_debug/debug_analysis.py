"""
==========================================================
 Modul 06 - STM32_03_I2C_Temp_BMP280
 Debug & Analisis: Parser serial, logging CSV, plot suhu/tekanan
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


def parse_serial(port='/dev/ttyUSB0', baudrate=115200, csv_file='bmp280_log.csv'):
    """Membaca data BMP280 dari serial dan menyimpan ke CSV"""
    print(f"[INFO] Membuka port {port} @ {baudrate} baud")

    try:
        ser = serial.Serial(port, baudrate, timeout=2)
        time.sleep(2)
        print("[INFO] Menunggu data BMP280...")

        with open(csv_file, 'w', newline='') as f:
            writer = csv.writer(f)
            writer.writerow(['timestamp', 'reading', 'suhu_c', 'tekanan_hpa', 'ketinggian_m'])

            while True:
                line = ser.readline().decode('utf-8', errors='replace').strip()
                if not line:
                    continue
                print(f"[SERIAL] {line}")

                # Format: DATA,nomor,suhu,tekanan,ketinggian
                match = re.search(r'DATA,(\d+),([\d.]+),([\d.]+),([\d.-]+)', line)
                if match:
                    reading = int(match.group(1))
                    suhu = float(match.group(2))
                    tekanan = float(match.group(3))
                    ketinggian = float(match.group(4))
                    ts = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
                    writer.writerow([ts, reading, suhu, tekanan, ketinggian])
                    f.flush()
                    print(f"[LOG] #{reading} T={suhu:.2f}C P={tekanan:.2f}hPa Alt={ketinggian:.1f}m")

    except serial.SerialException as e:
        print(f"[ERROR] {e}")
    except KeyboardInterrupt:
        print("\n[INFO] Dihentikan oleh pengguna")
    finally:
        if 'ser' in locals() and ser.is_open:
            ser.close()


def plot_hasil(csv_file='bmp280_log.csv'):
    """Plot data suhu dan tekanan"""
    if not HAS_MATPLOTLIB:
        print("[ERROR] matplotlib diperlukan")
        return

    readings, suhu_list, tekanan_list, alt_list = [], [], [], []
    try:
        with open(csv_file, 'r') as f:
            reader = csv.DictReader(f)
            for row in reader:
                readings.append(int(row['reading']))
                suhu_list.append(float(row['suhu_c']))
                tekanan_list.append(float(row['tekanan_hpa']))
                alt_list.append(float(row['ketinggian_m']))
    except FileNotFoundError:
        print(f"[ERROR] File {csv_file} tidak ditemukan")
        return

    fig, axes = plt.subplots(3, 1, figsize=(12, 10), sharex=True)

    axes[0].plot(readings, suhu_list, 'r-o', markersize=3, label='Suhu')
    axes[0].set_ylabel('Suhu (°C)')
    axes[0].set_title('BMP280 - Data Sensor')
    axes[0].legend()
    axes[0].grid(True, alpha=0.3)

    axes[1].plot(readings, tekanan_list, 'b-o', markersize=3, label='Tekanan')
    axes[1].set_ylabel('Tekanan (hPa)')
    axes[1].legend()
    axes[1].grid(True, alpha=0.3)

    axes[2].plot(readings, alt_list, 'g-o', markersize=3, label='Ketinggian')
    axes[2].set_ylabel('Ketinggian (m)')
    axes[2].set_xlabel('Pembacaan #')
    axes[2].legend()
    axes[2].grid(True, alpha=0.3)

    plt.tight_layout()
    plt.savefig('bmp280_plot.png', dpi=150)
    print("[INFO] Plot disimpan ke bmp280_plot.png")
    plt.show()


if __name__ == '__main__':
    if len(sys.argv) > 1 and sys.argv[1] == 'plot':
        csv_f = sys.argv[2] if len(sys.argv) > 2 else 'bmp280_log.csv'
        plot_hasil(csv_f)
    else:
        port = sys.argv[1] if len(sys.argv) > 1 else '/dev/ttyUSB0'
        parse_serial(port=port)
