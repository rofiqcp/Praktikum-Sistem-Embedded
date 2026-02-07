"""
=============================================================================
 Program     : STM32_04_ADC_Multi_Channel - Debug & Analisis
 Modul       : 04 - ADC
 Deskripsi   : Serial monitor untuk ADC multi-channel (PA0 & PA1),
               menyimpan ke CSV, dan menampilkan grafik perbandingan.
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
CSV_FILENAME = 'adc_multi_channel_log.csv'
MAX_DATA_POINTS = 200

# ======================== VARIABEL GLOBAL ====================
timestamps = deque(maxlen=MAX_DATA_POINTS)
ch0_raw = deque(maxlen=MAX_DATA_POINTS)
ch1_raw = deque(maxlen=MAX_DATA_POINTS)
ch0_mv = deque(maxlen=MAX_DATA_POINTS)
ch1_mv = deque(maxlen=MAX_DATA_POINTS)
start_time = time.time()

def parse_line(line):
    """Parsing: [counter] CH0_RAW:val CH0_MV:val CH1_RAW:val CH1_MV:val"""
    match = re.search(
        r'\[(\d+)\]\s*CH0_RAW:(\d+)\s*CH0_MV:(\d+)\s*CH1_RAW:(\d+)\s*CH1_MV:(\d+)', line)
    if match:
        return {
            'counter': int(match.group(1)),
            'ch0_raw': int(match.group(2)),
            'ch0_mv': int(match.group(3)),
            'ch1_raw': int(match.group(4)),
            'ch1_mv': int(match.group(5))
        }
    return None

def save_to_csv(data, filename):
    """Menyimpan data ke CSV"""
    file_exists = os.path.isfile(filename)
    with open(filename, 'a', newline='') as csvfile:
        fieldnames = ['timestamp', 'counter', 'ch0_raw', 'ch0_mv', 'ch1_raw', 'ch1_mv']
        writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
        if not file_exists:
            writer.writeheader()
        writer.writerow({
            'timestamp': datetime.now().strftime('%Y-%m-%d %H:%M:%S.%f'),
            **data
        })

def update_plot(frame):
    """Update grafik multi-channel"""
    ax1.clear()
    ax2.clear()
    if len(timestamps) > 0:
        t = list(timestamps)
        ax1.plot(t, list(ch0_raw), 'b-', linewidth=2, label='CH0 (PA0)')
        ax1.plot(t, list(ch1_raw), 'r-', linewidth=2, label='CH1 (PA1)')
        ax1.set_ylim(-100, 4200)
        ax1.set_title('ADC Raw - Multi Channel')
        ax1.set_ylabel('Nilai ADC')
        ax1.legend()
        ax1.grid(True, alpha=0.3)

        ax2.plot(t, list(ch0_mv), 'b-', linewidth=2, label='CH0 (mV)')
        ax2.plot(t, list(ch1_mv), 'r-', linewidth=2, label='CH1 (mV)')
        ax2.set_ylim(-100, 3500)
        ax2.set_title('Tegangan - Multi Channel')
        ax2.set_xlabel('Waktu (detik)')
        ax2.set_ylabel('Tegangan (mV)')
        ax2.legend()
        ax2.grid(True, alpha=0.3)

def main():
    global ax1, ax2
    print("=" * 60)
    print("  STM32 ADC Multi Channel - Serial Monitor & Analyzer")
    print("=" * 60)

    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(10, 8))
    plt.tight_layout(pad=3.0)

    try:
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
        print(f"\n[INFO] Terhubung ke {SERIAL_PORT}")
        time.sleep(2)
        ser.flushInput()

        ani = animation.FuncAnimation(fig, update_plot, interval=500, cache_frame_data=False)

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
                            ch0_raw.append(data['ch0_raw'])
                            ch1_raw.append(data['ch1_raw'])
                            ch0_mv.append(data['ch0_mv'])
                            ch1_mv.append(data['ch1_mv'])
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
