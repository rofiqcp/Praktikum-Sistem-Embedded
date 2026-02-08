"""
==========================================================
 Debug Analysis Script - PWM RGB LED Rainbow
 Modul 05 - DAC & PWM
 Deskripsi: Membaca data warna RGB dari serial,
            menyimpan ke CSV, dan menampilkan grafik.
==========================================================
"""

import serial
import csv
import time
import sys
import re
import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
import numpy as np

SERIAL_PORT = '/dev/ttyUSB0'
BAUD_RATE = 115200
TIMEOUT = 2

def baca_serial(port=SERIAL_PORT, baud=BAUD_RATE, durasi=30):
    """Membaca data RGB dari serial port."""
    data_list = []
    try:
        ser = serial.Serial(port, baud, timeout=TIMEOUT)
        print(f"[INFO] Terhubung ke {port} @ {baud} baud")
        print(f"[INFO] Membaca data selama {durasi} detik...\n")

        waktu_mulai = time.time()
        while (time.time() - waktu_mulai) < durasi:
            line = ser.readline().decode('utf-8', errors='ignore').strip()
            if not line:
                continue

            print(f"  >> {line}")

            # Parsing: [RGB] Hue=120 | R=  0 G=255 B=  0 | Hijau
            match = re.search(
                r'Hue=\s*(\d+)\s*\|\s*R=\s*(\d+)\s*G=\s*(\d+)\s*B=\s*(\d+)\s*\|\s*(\w+)',
                line
            )
            if match:
                hue = int(match.group(1))
                r = int(match.group(2))
                g = int(match.group(3))
                b = int(match.group(4))
                warna = match.group(5)
                waktu = time.time() - waktu_mulai
                data_list.append({
                    'waktu': round(waktu, 2),
                    'hue': hue,
                    'r': r,
                    'g': g,
                    'b': b,
                    'warna': warna
                })

        ser.close()
        print(f"\n[INFO] Selesai. Total data: {len(data_list)} sampel")

    except serial.SerialException as e:
        print(f"[ERROR] Gagal membuka serial port: {e}")
    except KeyboardInterrupt:
        print("\n[INFO] Dihentikan oleh pengguna")
        if 'ser' in locals():
            ser.close()

    return data_list

def simpan_csv(data_list, nama_file='pwm_rgb_led.csv'):
    """Menyimpan data ke file CSV."""
    if not data_list:
        print("[WARNING] Tidak ada data untuk disimpan")
        return

    with open(nama_file, 'w', newline='') as f:
        writer = csv.DictWriter(f, fieldnames=['waktu', 'hue', 'r', 'g', 'b', 'warna'])
        writer.writeheader()
        writer.writerows(data_list)

    print(f"[INFO] Data disimpan ke {nama_file}")

def plot_grafik(data_list):
    """Menampilkan grafik RGB rainbow."""
    if not data_list:
        print("[WARNING] Tidak ada data untuk ditampilkan")
        return

    hue = [d['hue'] for d in data_list]
    r_vals = [d['r'] for d in data_list]
    g_vals = [d['g'] for d in data_list]
    b_vals = [d['b'] for d in data_list]

    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(10, 8))

    # Grafik RGB channels vs Hue
    ax1.plot(hue, r_vals, 'r-', linewidth=2, label='Red (CH1)')
    ax1.plot(hue, g_vals, 'g-', linewidth=2, label='Green (CH2)')
    ax1.plot(hue, b_vals, 'b-', linewidth=2, label='Blue (CH3)')
    ax1.set_xlabel('Hue (derajat)')
    ax1.set_ylabel('Nilai PWM (0-255)')
    ax1.set_title('PWM RGB LED - Komponen Warna vs Hue')
    ax1.set_xlim(0, 360)
    ax1.set_ylim(-10, 270)
    ax1.grid(True, alpha=0.3)
    ax1.legend()

    # Grafik color bar berdasarkan hue
    for i in range(len(hue)):
        color = (r_vals[i]/255.0, g_vals[i]/255.0, b_vals[i]/255.0)
        ax2.axvspan(hue[i]-1, hue[i]+1, color=color, alpha=0.8)

    ax2.set_xlabel('Hue (derajat)')
    ax2.set_title('Warna Aktual RGB LED')
    ax2.set_xlim(0, 360)
    ax2.set_yticks([])

    plt.tight_layout()
    plt.savefig('pwm_rgb_led.png', dpi=150)
    plt.show()
    print("[INFO] Grafik disimpan ke pwm_rgb_led.png")

def main():
    """Fungsi utama."""
    print("=" * 55)
    print(" Debug Analysis - PWM RGB LED Rainbow")
    print(" Modul 05 - DAC & PWM")
    print("=" * 55)

    port = sys.argv[1] if len(sys.argv) > 1 else SERIAL_PORT
    durasi = int(sys.argv[2]) if len(sys.argv) > 2 else 30

    data = baca_serial(port=port, durasi=durasi)
    simpan_csv(data)
    plot_grafik(data)

if __name__ == '__main__':
    main()
