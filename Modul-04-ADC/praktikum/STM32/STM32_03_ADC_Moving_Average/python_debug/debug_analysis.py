"""
=============================================================================
 Program     : STM32_03_ADC_Moving_Average - Debug & Analisis
 Modul       : 04 - ADC
 Deskripsi   : Membaca data ADC raw vs filtered (MA-16 & MA-32),
               menyimpan ke CSV, dan membandingkan di grafik real-time.
=============================================================================
"""

import serial
import csv
import time
import re
import os
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from datetime import datetime
from collections import deque

# ======================== KONFIGURASI ========================
SERIAL_PORT = '/dev/ttyUSB0'
BAUD_RATE = 115200
CSV_FILENAME = 'adc_moving_average_log.csv'
MAX_DATA_POINTS = 300

# ======================== VARIABEL GLOBAL ====================
timestamps = deque(maxlen=MAX_DATA_POINTS)
raw_values = deque(maxlen=MAX_DATA_POINTS)
ma16_values = deque(maxlen=MAX_DATA_POINTS)
ma32_values = deque(maxlen=MAX_DATA_POINTS)
start_time = time.time()

def parse_line(line):
    """Parsing: [counter] RAW:val MA16:val MA32:val"""
    match = re.search(r'\[(\d+)\]\s*RAW:(\d+)\s*MA16:(\d+)\s*MA32:(\d+)', line)
    if match:
        return {
            'counter': int(match.group(1)),
            'raw': int(match.group(2)),
            'ma16': int(match.group(3)),
            'ma32': int(match.group(4))
        }
    return None

def save_to_csv(data, filename):
    """Menyimpan data ke CSV"""
    file_exists = os.path.isfile(filename)
    with open(filename, 'a', newline='') as csvfile:
        fieldnames = ['timestamp', 'counter', 'raw', 'ma16', 'ma32']
        writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
        if not file_exists:
            writer.writeheader()
        writer.writerow({
            'timestamp': datetime.now().strftime('%Y-%m-%d %H:%M:%S.%f'),
            'counter': data['counter'],
            'raw': data['raw'],
            'ma16': data['ma16'],
            'ma32': data['ma32']
        })

def update_plot(frame):
    """Update grafik perbandingan"""
    ax.clear()
    if len(timestamps) > 0:
        ax.plot(list(timestamps), list(raw_values), 'b-', alpha=0.4, linewidth=1, label='Raw')
        ax.plot(list(timestamps), list(ma16_values), 'r-', linewidth=2, label='MA-16')
        ax.plot(list(timestamps), list(ma32_values), 'g-', linewidth=2, label='MA-32')
        ax.set_ylim(-100, 4200)
    ax.set_title('ADC Moving Average: Raw vs MA-16 vs MA-32')
    ax.set_xlabel('Waktu (detik)')
    ax.set_ylabel('Nilai ADC')
    ax.legend(loc='upper right')
    ax.grid(True, alpha=0.3)

def main():
    global ax
    print("=" * 60)
    print("  STM32 ADC Moving Average - Serial Monitor & Analyzer")
    print("=" * 60)

    fig, ax = plt.subplots(figsize=(12, 6))
    plt.tight_layout()

    try:
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
        print(f"\n[INFO] Terhubung ke {SERIAL_PORT}")
        time.sleep(2)
        ser.flushInput()

        ani = animation.FuncAnimation(fig, update_plot, interval=200, cache_frame_data=False)

        while True:
            if ser.in_waiting > 0:
                try:
                    line = ser.readline().decode('utf-8', errors='ignore').strip()
                    if line:
                        print(f"  >> {line}")
                        data = parse_line(line)
                        if data:
                            elapsed = time.time() - start_time
                            timestamps.append(elapsed)
                            raw_values.append(data['raw'])
                            ma16_values.append(data['ma16'])
                            ma32_values.append(data['ma32'])
                            save_to_csv(data, CSV_FILENAME)
                except Exception as e:
                    print(f"[ERROR] {e}")
            plt.pause(0.01)

    except serial.SerialException as e:
        print(f"\n[ERROR] Serial: {e}")
    except KeyboardInterrupt:
        print(f"\n[INFO] Program dihentikan. Data: {CSV_FILENAME}")
    finally:
        try:
            ser.close()
        except:
            pass

if __name__ == '__main__':
    main()
