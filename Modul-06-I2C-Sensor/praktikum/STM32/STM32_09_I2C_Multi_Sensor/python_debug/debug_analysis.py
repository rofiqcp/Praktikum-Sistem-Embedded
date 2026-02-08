"""
==========================================================
 Modul 06 - STM32_09_I2C_Multi_Sensor
 Debug & Analisis: Parser serial, logging CSV, plot gabungan
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


def parse_serial(port='/dev/ttyUSB0', baudrate=115200, csv_file='multi_sensor_log.csv'):
    """Membaca data multi-sensor dan menyimpan ke CSV"""
    print(f"[INFO] Membuka port {port} @ {baudrate} baud")

    try:
        ser = serial.Serial(port, baudrate, timeout=2)
        time.sleep(2)
        print("[INFO] Menunggu data multi-sensor...")

        with open(csv_file, 'w', newline='') as f:
            writer = csv.writer(f)
            writer.writerow(['timestamp', 'reading', 'suhu_c', 'tekanan_hpa', 'lux', 'ketinggian_m'])

            while True:
                line = ser.readline().decode('utf-8', errors='replace').strip()
                if not line:
                    continue
                print(f"[SERIAL] {line}")

                match = re.search(r'DATA,(\d+),([\d.]+),([\d.]+),([\d.]+),([\d.-]+)', line)
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


def plot_hasil(csv_file='multi_sensor_log.csv'):
    """Plot data multi-sensor"""
    if not HAS_MATPLOTLIB:
        print("[ERROR] matplotlib diperlukan")
        return

    data = {'reading': [], 'suhu': [], 'tekanan': [], 'lux': [], 'alt': []}
    try:
        with open(csv_file, 'r') as f:
            reader = csv.DictReader(f)
            for row in reader:
                data['reading'].append(int(row['reading']))
                data['suhu'].append(float(row['suhu_c']))
                data['tekanan'].append(float(row['tekanan_hpa']))
                data['lux'].append(float(row['lux']))
                data['alt'].append(float(row['ketinggian_m']))
    except FileNotFoundError:
        print(f"[ERROR] File {csv_file} tidak ditemukan")
        return

    fig, axes = plt.subplots(2, 2, figsize=(14, 8))

    axes[0, 0].plot(data['reading'], data['suhu'], 'r-o', markersize=3)
    axes[0, 0].set_ylabel('Suhu (°C)')
    axes[0, 0].set_title('BMP280 - Suhu')
    axes[0, 0].grid(True, alpha=0.3)

    axes[0, 1].plot(data['reading'], data['tekanan'], 'b-o', markersize=3)
    axes[0, 1].set_ylabel('Tekanan (hPa)')
    axes[0, 1].set_title('BMP280 - Tekanan')
    axes[0, 1].grid(True, alpha=0.3)

    axes[1, 0].plot(data['reading'], data['lux'], 'gold', linewidth=2)
    axes[1, 0].set_ylabel('Cahaya (lux)')
    axes[1, 0].set_xlabel('Pembacaan #')
    axes[1, 0].set_title('BH1750 - Cahaya')
    axes[1, 0].grid(True, alpha=0.3)

    axes[1, 1].plot(data['reading'], data['alt'], 'g-o', markersize=3)
    axes[1, 1].set_ylabel('Ketinggian (m)')
    axes[1, 1].set_xlabel('Pembacaan #')
    axes[1, 1].set_title('Estimasi Ketinggian')
    axes[1, 1].grid(True, alpha=0.3)

    plt.suptitle('Multi Sensor Dashboard - BMP280 + BH1750', fontsize=14, fontweight='bold')
    plt.tight_layout()
    plt.savefig('multi_sensor_plot.png', dpi=150)
    print("[INFO] Plot disimpan ke multi_sensor_plot.png")
    plt.show()


if __name__ == '__main__':
    if len(sys.argv) > 1 and sys.argv[1] == 'plot':
        csv_f = sys.argv[2] if len(sys.argv) > 2 else 'multi_sensor_log.csv'
        plot_hasil(csv_f)
    else:
        port = sys.argv[1] if len(sys.argv) > 1 else '/dev/ttyUSB0'
        parse_serial(port=port)
