"""
=============================================================================
 Program     : STM32_10_ADC_Sampling_Rate - Debug & Analisis
 Modul       : 04 - ADC
 Deskripsi   : Serial monitor untuk mengukur dan membandingkan kecepatan
               sampling ADC pada berbagai sampling time. Grafik bar chart.
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
from collections import deque, defaultdict

# ======================== KONFIGURASI ========================
SERIAL_PORT = '/dev/ttyUSB0'
BAUD_RATE = 115200
CSV_FILENAME = 'adc_sampling_rate_log.csv'
MAX_DATA_POINTS = 100

# ======================== VARIABEL GLOBAL ====================
sps_data = defaultdict(list)  # Dict: sampling_time -> [sps_values]
latest_sps = {}               # Dict: sampling_time -> latest_sps
start_time = time.time()

def parse_line(line):
    """Parsing: [n] STIME:name SPS:val SAMPLES:val"""
    match = re.search(r'\[(\d+)\]\s*STIME:(\S+)\s*SPS:(\d+)\s*SAMPLES:(\d+)', line)
    if match:
        return {
            'counter': int(match.group(1)),
            'stime': match.group(2),
            'sps': int(match.group(3)),
            'samples': int(match.group(4))
        }
    return None

def save_to_csv(data, filename):
    """Menyimpan data ke CSV"""
    file_exists = os.path.isfile(filename)
    with open(filename, 'a', newline='') as csvfile:
        fieldnames = ['timestamp', 'counter', 'stime', 'sps', 'samples']
        writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
        if not file_exists:
            writer.writeheader()
        writer.writerow({
            'timestamp': datetime.now().strftime('%Y-%m-%d %H:%M:%S.%f'),
            **data
        })

def update_plot(frame):
    """Update bar chart SPS"""
    ax.clear()
    if latest_sps:
        names = list(latest_sps.keys())
        values = list(latest_sps.values())
        colors = plt.cm.viridis([i / len(names) for i in range(len(names))])
        bars = ax.bar(names, values, color=colors, edgecolor='black')

        # Tambahkan label nilai di atas bar
        for bar, val in zip(bars, values):
            ax.text(bar.get_x() + bar.get_width()/2., bar.get_height() + 100,
                    f'{val}', ha='center', va='bottom', fontsize=9, fontweight='bold')

    ax.set_title('Kecepatan Sampling ADC (SPS) vs Sampling Time')
    ax.set_xlabel('Sampling Time')
    ax.set_ylabel('Samples Per Second (SPS)')
    ax.tick_params(axis='x', rotation=45)
    ax.grid(True, alpha=0.3, axis='y')
    plt.tight_layout()

def main():
    global ax
    print("=" * 60)
    print("  STM32 ADC Sampling Rate - Serial Monitor & Analyzer")
    print("=" * 60)

    fig, ax = plt.subplots(figsize=(12, 6))

    try:
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
        print(f"\n[INFO] Terhubung ke {SERIAL_PORT}")
        time.sleep(2)
        ser.flushInput()

        ani = animation.FuncAnimation(fig, update_plot, interval=3000, cache_frame_data=False)

        while True:
            if ser.in_waiting > 0:
                try:
                    line = ser.readline().decode('utf-8', errors='ignore').strip()
                    if line:
                        print(f"  >> {line}")
                        data = parse_line(line)
                        if data:
                            sps_data[data['stime']].append(data['sps'])
                            latest_sps[data['stime']] = data['sps']
                            save_to_csv(data, CSV_FILENAME)
                except Exception as e:
                    print(f"[ERROR] {e}")
            plt.pause(0.01)

    except serial.SerialException as e:
        print(f"\n[ERROR] Serial: {e}")
    except KeyboardInterrupt:
        print(f"\n[INFO] Program dihentikan. Data: {CSV_FILENAME}")
        if latest_sps:
            print("\n[RINGKASAN] SPS terakhir:")
            for name, sps in latest_sps.items():
                print(f"  {name}: {sps} SPS")
    finally:
        try:
            ser.close()
        except:
            pass

if __name__ == '__main__':
    main()
