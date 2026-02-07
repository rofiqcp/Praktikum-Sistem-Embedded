"""
=============================================================================
 Program     : STM32_05_ADC_Calibration - Debug & Analisis
 Modul       : 04 - ADC
 Deskripsi   : Serial monitor untuk membandingkan ADC sebelum dan sesudah
               kalibrasi, menyimpan ke CSV, dan menampilkan grafik.
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
CSV_FILENAME = 'adc_calibration_log.csv'
MAX_DATA_POINTS = 100

# ======================== VARIABEL GLOBAL ====================
timestamps = deque(maxlen=MAX_DATA_POINTS)
pre_cal_values = deque(maxlen=MAX_DATA_POINTS)
post_cal_values = deque(maxlen=MAX_DATA_POINTS)
diff_values = deque(maxlen=MAX_DATA_POINTS)
start_time = time.time()

def parse_line(line):
    """Parsing: [n] PRE_CAL:val PRE_MV:val POST_CAL:val POST_MV:val DIFF:val"""
    match = re.search(
        r'\[(\d+)\]\s*PRE_CAL:(\d+)\s*PRE_MV:(\d+)\s*POST_CAL:(\d+)\s*POST_MV:(\d+)\s*DIFF:(-?\d+)', line)
    if match:
        return {
            'counter': int(match.group(1)),
            'pre_cal': int(match.group(2)),
            'pre_mv': int(match.group(3)),
            'post_cal': int(match.group(4)),
            'post_mv': int(match.group(5)),
            'diff': int(match.group(6))
        }
    return None

def save_to_csv(data, filename):
    """Menyimpan data ke CSV"""
    file_exists = os.path.isfile(filename)
    with open(filename, 'a', newline='') as csvfile:
        fieldnames = ['timestamp', 'counter', 'pre_cal', 'pre_mv', 'post_cal', 'post_mv', 'diff']
        writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
        if not file_exists:
            writer.writeheader()
        writer.writerow({
            'timestamp': datetime.now().strftime('%Y-%m-%d %H:%M:%S.%f'),
            **data
        })

def update_plot(frame):
    """Update grafik perbandingan kalibrasi"""
    ax1.clear()
    ax2.clear()
    if len(timestamps) > 0:
        t = list(timestamps)
        ax1.plot(t, list(pre_cal_values), 'r-o', markersize=3, label='Sebelum Kalibrasi')
        ax1.plot(t, list(post_cal_values), 'g-o', markersize=3, label='Sesudah Kalibrasi')
        ax1.set_title('Perbandingan ADC: Sebelum vs Sesudah Kalibrasi')
        ax1.set_ylabel('Nilai ADC')
        ax1.legend()
        ax1.grid(True, alpha=0.3)

        ax2.bar(t, list(diff_values), width=0.3, color='blue', alpha=0.7)
        ax2.set_title('Selisih (Post - Pre Kalibrasi)')
        ax2.set_xlabel('Waktu (detik)')
        ax2.set_ylabel('Selisih ADC')
        ax2.axhline(y=0, color='r', linestyle='--')
        ax2.grid(True, alpha=0.3)

def main():
    global ax1, ax2
    print("=" * 60)
    print("  STM32 ADC Calibration - Serial Monitor & Analyzer")
    print("=" * 60)

    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(10, 8))
    plt.tight_layout(pad=3.0)

    try:
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
        print(f"\n[INFO] Terhubung ke {SERIAL_PORT}")
        time.sleep(2)
        ser.flushInput()

        ani = animation.FuncAnimation(fig, update_plot, interval=1000, cache_frame_data=False)

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
                            pre_cal_values.append(data['pre_cal'])
                            post_cal_values.append(data['post_cal'])
                            diff_values.append(data['diff'])
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
