"""
==========================================================
 Debug Analysis Script - PWM Buzzer Melody
 Modul 05 - DAC & PWM
 Deskripsi: Membaca data melodi dari serial,
            menyimpan ke CSV, dan menampilkan grafik.
==========================================================
"""

import serial
import csv
import time
import sys
import re
import matplotlib.pyplot as plt
import numpy as np

SERIAL_PORT = '/dev/ttyUSB0'
BAUD_RATE = 115200
TIMEOUT = 2

def baca_serial(port=SERIAL_PORT, baud=BAUD_RATE, durasi=60):
    """Membaca data melodi dari serial port."""
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

            # Parsing: [MELODY] Not  1/43: C4  | Freq= 262 Hz | Dur=400 ms
            match = re.search(
                r'Not\s+(\d+)/(\d+):\s*(\S+)\s*\|\s*Freq=\s*(\d+)\s*Hz\s*\|\s*Dur=\s*(\d+)',
                line
            )
            if match:
                nomor = int(match.group(1))
                total = int(match.group(2))
                nama = match.group(3).strip()
                freq = int(match.group(4))
                durasi_ms = int(match.group(5))
                waktu = time.time() - waktu_mulai
                data_list.append({
                    'waktu': round(waktu, 2),
                    'not_ke': nomor,
                    'nama_not': nama,
                    'frekuensi': freq,
                    'durasi_ms': durasi_ms
                })

        ser.close()
        print(f"\n[INFO] Selesai. Total data: {len(data_list)} not")

    except serial.SerialException as e:
        print(f"[ERROR] Gagal membuka serial port: {e}")
    except KeyboardInterrupt:
        print("\n[INFO] Dihentikan oleh pengguna")
        if 'ser' in locals():
            ser.close()

    return data_list

def simpan_csv(data_list, nama_file='pwm_buzzer_melody.csv'):
    """Menyimpan data ke file CSV."""
    if not data_list:
        print("[WARNING] Tidak ada data untuk disimpan")
        return

    with open(nama_file, 'w', newline='') as f:
        writer = csv.DictWriter(f, fieldnames=['waktu', 'not_ke', 'nama_not', 'frekuensi', 'durasi_ms'])
        writer.writeheader()
        writer.writerows(data_list)

    print(f"[INFO] Data disimpan ke {nama_file}")

def plot_grafik(data_list):
    """Menampilkan grafik melodi."""
    if not data_list:
        print("[WARNING] Tidak ada data untuk ditampilkan")
        return

    waktu = [d['waktu'] for d in data_list]
    frekuensi = [d['frekuensi'] for d in data_list]
    nama = [d['nama_not'] for d in data_list]
    durasi = [d['durasi_ms'] for d in data_list]

    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(12, 8))

    # Grafik frekuensi vs waktu (piano roll style)
    for i in range(len(waktu)):
        if frekuensi[i] > 0:
            width = durasi[i] / 1000.0
            ax1.barh(frekuensi[i], width, left=waktu[i], height=20,
                    color='steelblue', alpha=0.7, edgecolor='navy')

    ax1.set_xlabel('Waktu (detik)')
    ax1.set_ylabel('Frekuensi (Hz)')
    ax1.set_title('Melodi Twinkle Twinkle - Piano Roll')
    ax1.grid(True, alpha=0.3)

    # Grafik not berurutan
    not_ke = [d['not_ke'] for d in data_list]
    colors = ['red' if f == 0 else 'blue' for f in frekuensi]
    ax2.bar(not_ke, frekuensi, color=colors, alpha=0.7, edgecolor='navy')
    ax2.set_xlabel('Not ke-')
    ax2.set_ylabel('Frekuensi (Hz)')
    ax2.set_title('Frekuensi Setiap Not')
    ax2.grid(True, alpha=0.3, axis='y')

    # Tambahkan label nama not
    for i, (n, f) in enumerate(zip(not_ke, frekuensi)):
        if f > 0 and i < len(nama):
            ax2.text(n, f + 10, nama[i], ha='center', fontsize=6, rotation=45)

    plt.tight_layout()
    plt.savefig('pwm_buzzer_melody.png', dpi=150)
    plt.show()
    print("[INFO] Grafik disimpan ke pwm_buzzer_melody.png")

def main():
    """Fungsi utama."""
    print("=" * 55)
    print(" Debug Analysis - PWM Buzzer Melody")
    print(" Modul 05 - DAC & PWM")
    print("=" * 55)

    port = sys.argv[1] if len(sys.argv) > 1 else SERIAL_PORT
    durasi = int(sys.argv[2]) if len(sys.argv) > 2 else 60

    data = baca_serial(port=port, durasi=durasi)
    simpan_csv(data)
    plot_grafik(data)

if __name__ == '__main__':
    main()
