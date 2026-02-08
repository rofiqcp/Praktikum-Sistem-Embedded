"""
==========================================================
 Modul 06 - STM32_05_I2C_EEPROM_AT24C32
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


def parse_serial(port='/dev/ttyUSB0', baudrate=115200, csv_file='eeprom_log.csv'):
    """Membaca data EEPROM dari serial dan menyimpan ke CSV"""
    print(f"[INFO] Membuka port {port} @ {baudrate} baud")

    try:
        ser = serial.Serial(port, baudrate, timeout=2)
        time.sleep(2)
        print("[INFO] Menunggu data EEPROM...")

        with open(csv_file, 'w', newline='') as f:
            writer = csv.writer(f)
            writer.writerow(['timestamp', 'event', 'counter', 'address'])

            while True:
                line = ser.readline().decode('utf-8', errors='replace').strip()
                if not line:
                    continue
                print(f"[SERIAL] {line}")

                # Parse counter data
                match_data = re.search(r'DATA,counter=(\d+)', line)
                if match_data:
                    counter = int(match_data.group(1))
                    ts = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
                    writer.writerow([ts, 'READ', counter, ''])
                    f.flush()

                # Parse save event
                match_save = re.search(r'SAVE,(\d+),(0x[0-9A-Fa-f]+)', line)
                if match_save:
                    counter = int(match_save.group(1))
                    addr = match_save.group(2)
                    ts = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
                    writer.writerow([ts, 'SAVE', counter, addr])
                    f.flush()
                    print(f"[LOG] EEPROM SAVE: counter={counter} @ {addr}")

    except serial.SerialException as e:
        print(f"[ERROR] {e}")
    except KeyboardInterrupt:
        print("\n[INFO] Dihentikan oleh pengguna")
    finally:
        if 'ser' in locals() and ser.is_open:
            ser.close()


def plot_hasil(csv_file='eeprom_log.csv'):
    """Plot data counter dan event save"""
    if not HAS_MATPLOTLIB:
        print("[ERROR] matplotlib diperlukan")
        return

    counters = []
    save_points = []
    try:
        with open(csv_file, 'r') as f:
            reader = csv.DictReader(f)
            idx = 0
            for row in reader:
                counters.append(int(row['counter']))
                if row['event'] == 'SAVE':
                    save_points.append(idx)
                idx += 1
    except FileNotFoundError:
        print(f"[ERROR] File {csv_file} tidak ditemukan")
        return

    fig, ax = plt.subplots(figsize=(10, 5))
    ax.plot(range(len(counters)), counters, 'b-', label='Counter', alpha=0.7)
    if save_points:
        ax.scatter(save_points, [counters[i] for i in save_points],
                   color='red', zorder=5, label='EEPROM Save', s=50)
    ax.set_xlabel('Sample')
    ax.set_ylabel('Counter')
    ax.set_title('AT24C32 EEPROM - Counter & Save Events')
    ax.legend()
    ax.grid(True, alpha=0.3)

    plt.tight_layout()
    plt.savefig('eeprom_plot.png', dpi=150)
    print("[INFO] Plot disimpan ke eeprom_plot.png")
    plt.show()


if __name__ == '__main__':
    if len(sys.argv) > 1 and sys.argv[1] == 'plot':
        csv_f = sys.argv[2] if len(sys.argv) > 2 else 'eeprom_log.csv'
        plot_hasil(csv_f)
    else:
        port = sys.argv[1] if len(sys.argv) > 1 else '/dev/ttyUSB0'
        parse_serial(port=port)
