"""
==========================================================
 Debug Analysis Script - DAC Audio Tone
 Modul 05 - DAC & PWM
 Deskripsi: Membaca data nada audio dari serial,
            menyimpan ke CSV, dan menampilkan grafik.
==========================================================
"""

import serial
import csv
import time
import sys
import re
import matplotlib.pyplot as plt

SERIAL_PORT = '/dev/ttyUSB0'
BAUD_RATE = 115200
TIMEOUT = 2

def baca_serial(port=SERIAL_PORT, baud=BAUD_RATE, durasi=30):
    """Membaca data nada audio dari serial port."""
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

            # Parsing: [AUDIO] Nada: 440 Hz (A4) - DAC sinus
            match = re.search(r'Nada:\s*(\d+)\s*Hz\s*\((\w+)\)\s*-\s*(.+)', line)
            if match:
                frekuensi = int(match.group(1))
                nama_nada = match.group(2)
                metode = match.group(3).strip()
                waktu = time.time() - waktu_mulai
                data_list.append({
                    'waktu': round(waktu, 2),
                    'frekuensi': frekuensi,
                    'nada': nama_nada,
                    'metode': metode
                })

            # Parsing silence
            if 'Diam' in line:
                waktu = time.time() - waktu_mulai
                data_list.append({
                    'waktu': round(waktu, 2),
                    'frekuensi': 0,
                    'nada': 'DIAM',
                    'metode': 'silence'
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

def simpan_csv(data_list, nama_file='dac_audio_tone.csv'):
    """Menyimpan data ke file CSV."""
    if not data_list:
        print("[WARNING] Tidak ada data untuk disimpan")
        return

    with open(nama_file, 'w', newline='') as f:
        writer = csv.DictWriter(f, fieldnames=['waktu', 'frekuensi', 'nada', 'metode'])
        writer.writeheader()
        writer.writerows(data_list)

    print(f"[INFO] Data disimpan ke {nama_file}")

def plot_grafik(data_list):
    """Menampilkan grafik frekuensi nada."""
    if not data_list:
        print("[WARNING] Tidak ada data untuk ditampilkan")
        return

    waktu = [d['waktu'] for d in data_list]
    frekuensi = [d['frekuensi'] for d in data_list]

    fig, ax = plt.subplots(figsize=(10, 5))

    ax.step(waktu, frekuensi, 'b-', where='post', linewidth=2, label='Frekuensi Nada')
    ax.fill_between(waktu, frekuensi, step='post', alpha=0.2)
    ax.set_xlabel('Waktu (detik)')
    ax.set_ylabel('Frekuensi (Hz)')
    ax.set_title('DAC Audio Tone - Frekuensi vs Waktu')
    ax.set_ylim(-50, 1000)
    ax.axhline(y=440, color='r', linestyle='--', alpha=0.5, label='440 Hz (A4)')
    ax.axhline(y=880, color='g', linestyle='--', alpha=0.5, label='880 Hz (A5)')
    ax.grid(True, alpha=0.3)
    ax.legend()

    plt.tight_layout()
    plt.savefig('dac_audio_tone.png', dpi=150)
    plt.show()
    print("[INFO] Grafik disimpan ke dac_audio_tone.png")

def main():
    """Fungsi utama."""
    print("=" * 55)
    print(" Debug Analysis - DAC Audio Tone")
    print(" Modul 05 - DAC & PWM")
    print("=" * 55)

    port = sys.argv[1] if len(sys.argv) > 1 else SERIAL_PORT
    durasi = int(sys.argv[2]) if len(sys.argv) > 2 else 30

    data = baca_serial(port=port, durasi=durasi)
    simpan_csv(data)
    plot_grafik(data)

if __name__ == '__main__':
    main()
