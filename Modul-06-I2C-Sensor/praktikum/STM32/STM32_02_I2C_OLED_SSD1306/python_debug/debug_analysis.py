"""
==========================================================
 Modul 06 - STM32_02_I2C_OLED_SSD1306
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


def parse_serial(port='/dev/ttyUSB0', baudrate=115200, csv_file='oled_log.csv'):
    """Membaca data serial dari STM32 OLED dan menyimpan ke CSV"""
    print(f"[INFO] Membuka port {port} @ {baudrate} baud")

    counters = []
    uptimes = []

    try:
        ser = serial.Serial(port, baudrate, timeout=2)
        time.sleep(2)
        print("[INFO] Port serial terbuka. Menunggu data...")

        with open(csv_file, 'w', newline='') as f:
            writer = csv.writer(f)
            writer.writerow(['timestamp', 'counter', 'uptime_s'])

            while True:
                line = ser.readline().decode('utf-8', errors='replace').strip()
                if not line:
                    continue
                print(f"[SERIAL] {line}")

                match = re.search(r'Counter:\s*(\d+),\s*Uptime:\s*(\d+)', line)
                if match:
                    counter = int(match.group(1))
                    uptime = int(match.group(2))
                    ts = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
                    writer.writerow([ts, counter, uptime])
                    f.flush()
                    counters.append(counter)
                    uptimes.append(uptime)
                    print(f"[LOG] Counter={counter}, Uptime={uptime}s")

    except serial.SerialException as e:
        print(f"[ERROR] {e}")
    except KeyboardInterrupt:
        print("\n[INFO] Dihentikan oleh pengguna")
    finally:
        if 'ser' in locals() and ser.is_open:
            ser.close()

    return counters, uptimes


def plot_hasil(csv_file='oled_log.csv'):
    """Plot data counter vs uptime"""
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
    ax.plot(uptimes, counters, 'b-o', markersize=4, label='Counter')
    ax.set_xlabel('Uptime (detik)')
    ax.set_ylabel('Counter')
    ax.set_title('STM32 OLED SSD1306 - Counter vs Uptime')
    ax.legend()
    ax.grid(True, alpha=0.3)
    plt.tight_layout()
    plt.savefig('oled_plot.png', dpi=150)
    print("[INFO] Plot disimpan ke oled_plot.png")
    plt.show()


if __name__ == '__main__':
    if len(sys.argv) > 1 and sys.argv[1] == 'plot':
        csv_f = sys.argv[2] if len(sys.argv) > 2 else 'oled_log.csv'
        plot_hasil(csv_f)
    else:
        port = sys.argv[1] if len(sys.argv) > 1 else '/dev/ttyUSB0'
        parse_serial(port=port)
