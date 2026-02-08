"""
==========================================================
 Modul 06 - STM32_08_I2C_LCD_PCF8574
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


def parse_serial(port='/dev/ttyUSB0', baudrate=115200, csv_file='lcd_log.csv'):
    """Membaca data LCD dari serial dan menyimpan ke CSV"""
    print(f"[INFO] Membuka port {port} @ {baudrate} baud")

    try:
        ser = serial.Serial(port, baudrate, timeout=2)
        time.sleep(2)
        print("[INFO] Menunggu data LCD...")

        with open(csv_file, 'w', newline='') as f:
            writer = csv.writer(f)
            writer.writerow(['timestamp', 'counter', 'uptime_s'])

            while True:
                line = ser.readline().decode('utf-8', errors='replace').strip()
                if not line:
                    continue
                print(f"[SERIAL] {line}")

                match = re.search(r'DATA,counter=(\d+),uptime=(\d+)', line)
                if match:
                    ts = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
                    writer.writerow([ts, match.group(1), match.group(2)])
                    f.flush()

    except serial.SerialException as e:
        print(f"[ERROR] {e}")
    except KeyboardInterrupt:
        print("\n[INFO] Dihentikan oleh pengguna")
    finally:
        if 'ser' in locals() and ser.is_open:
            ser.close()


def plot_hasil(csv_file='lcd_log.csv'):
    """Plot counter vs uptime"""
    if not HAS_MATPLOTLIB:
        print("[ERROR] matplotlib diperlukan")
        return

    counters, uptimes = [], []
    try:
        with open(csv_file, 'r') as f:
            reader = csv.DictReader(f)
            for row in reader:
                counters.append(int(row['counter']))
                uptimes.append(int(row['uptime_s']))
    except FileNotFoundError:
        print(f"[ERROR] File {csv_file} tidak ditemukan")
        return

    fig, ax = plt.subplots(figsize=(10, 5))
    ax.plot(uptimes, counters, 'g-o', markersize=3)
    ax.set_xlabel('Uptime (detik)')
    ax.set_ylabel('Counter')
    ax.set_title('LCD PCF8574 - Counter vs Uptime')
    ax.grid(True, alpha=0.3)

    plt.tight_layout()
    plt.savefig('lcd_plot.png', dpi=150)
    print("[INFO] Plot disimpan ke lcd_plot.png")
    plt.show()


if __name__ == '__main__':
    if len(sys.argv) > 1 and sys.argv[1] == 'plot':
        csv_f = sys.argv[2] if len(sys.argv) > 2 else 'lcd_log.csv'
        plot_hasil(csv_f)
    else:
        port = sys.argv[1] if len(sys.argv) > 1 else '/dev/ttyUSB0'
        parse_serial(port=port)
