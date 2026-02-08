"""
==========================================================
 Modul 06 - STM32_06_I2C_RTC_DS3231
 Debug & Analisis: Parser serial, logging CSV, plot suhu RTC
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


def parse_serial(port='/dev/ttyUSB0', baudrate=115200, csv_file='ds3231_log.csv'):
    """Membaca data DS3231 dan menyimpan ke CSV"""
    print(f"[INFO] Membuka port {port} @ {baudrate} baud")

    try:
        ser = serial.Serial(port, baudrate, timeout=2)
        time.sleep(2)
        print("[INFO] Menunggu data DS3231...")

        with open(csv_file, 'w', newline='') as f:
            writer = csv.writer(f)
            writer.writerow(['timestamp', 'reading', 'rtc_time', 'hari', 'tanggal', 'suhu_c', 'alarm'])

            while True:
                line = ser.readline().decode('utf-8', errors='replace').strip()
                if not line:
                    continue
                print(f"[SERIAL] {line}")

                match = re.search(r'DATA,(\d+),([\d:]+),(\w+),([\d/]+),([\d.]+),(\d+)', line)
                if match:
                    reading = match.group(1)
                    rtc_time = match.group(2)
                    hari = match.group(3)
                    tanggal = match.group(4)
                    suhu = float(match.group(5))
                    alarm = int(match.group(6))
                    ts = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
                    writer.writerow([ts, reading, rtc_time, hari, tanggal, suhu, alarm])
                    f.flush()

    except serial.SerialException as e:
        print(f"[ERROR] {e}")
    except KeyboardInterrupt:
        print("\n[INFO] Dihentikan oleh pengguna")
    finally:
        if 'ser' in locals() and ser.is_open:
            ser.close()


def plot_hasil(csv_file='ds3231_log.csv'):
    """Plot suhu internal DS3231"""
    if not HAS_MATPLOTLIB:
        print("[ERROR] matplotlib diperlukan")
        return

    readings, suhu_list = [], []
    try:
        with open(csv_file, 'r') as f:
            reader = csv.DictReader(f)
            for row in reader:
                readings.append(int(row['reading']))
                suhu_list.append(float(row['suhu_c']))
    except FileNotFoundError:
        print(f"[ERROR] File {csv_file} tidak ditemukan")
        return

    fig, ax = plt.subplots(figsize=(10, 5))
    ax.plot(readings, suhu_list, 'r-o', markersize=3, label='Suhu DS3231')
    ax.set_xlabel('Pembacaan #')
    ax.set_ylabel('Suhu (°C)')
    ax.set_title('DS3231 RTC - Suhu Internal')
    ax.legend()
    ax.grid(True, alpha=0.3)

    plt.tight_layout()
    plt.savefig('ds3231_plot.png', dpi=150)
    print("[INFO] Plot disimpan ke ds3231_plot.png")
    plt.show()


if __name__ == '__main__':
    if len(sys.argv) > 1 and sys.argv[1] == 'plot':
        csv_f = sys.argv[2] if len(sys.argv) > 2 else 'ds3231_log.csv'
        plot_hasil(csv_f)
    else:
        port = sys.argv[1] if len(sys.argv) > 1 else '/dev/ttyUSB0'
        parse_serial(port=port)
