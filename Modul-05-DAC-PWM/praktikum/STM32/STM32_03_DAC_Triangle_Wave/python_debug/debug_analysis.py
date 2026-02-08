"""
==========================================================
 Debug Analysis Script - DAC Triangle Wave
 Modul 05 - DAC & PWM
 Deskripsi: Membaca data gelombang segitiga dari serial,
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
    """Membaca data gelombang segitiga dari serial port."""
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

            # Parsing: [TRIANGLE] DAC=1234, Tegangan=1.00 V, Arah=Naik
            match = re.search(r'(?:DAC|PWM)=\s*(\d+),\s*Tegangan[~=]\s*([\d.]+).*?Arah=(\w+)', line)
            if match:
                nilai = int(match.group(1))
                tegangan = float(match.group(2))
                arah = match.group(3)
                waktu = time.time() - waktu_mulai
                data_list.append({
                    'waktu': round(waktu, 3),
                    'nilai': nilai,
                    'tegangan': tegangan,
                    'arah': arah
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

def simpan_csv(data_list, nama_file='dac_triangle_wave.csv'):
    """Menyimpan data ke file CSV."""
    if not data_list:
        print("[WARNING] Tidak ada data untuk disimpan")
        return

    with open(nama_file, 'w', newline='') as f:
        writer = csv.DictWriter(f, fieldnames=['waktu', 'nilai', 'tegangan', 'arah'])
        writer.writeheader()
        writer.writerows(data_list)

    print(f"[INFO] Data disimpan ke {nama_file}")

def plot_grafik(data_list):
    """Menampilkan grafik gelombang segitiga."""
    if not data_list:
        print("[WARNING] Tidak ada data untuk ditampilkan")
        return

    waktu = [d['waktu'] for d in data_list]
    tegangan = [d['tegangan'] for d in data_list]
    nilai = [d['nilai'] for d in data_list]

    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(10, 8))

    # Grafik tegangan vs waktu (gelombang segitiga)
    ax1.plot(waktu, tegangan, 'g-', linewidth=1.5, label='Triangle Wave')
    ax1.set_xlabel('Waktu (detik)')
    ax1.set_ylabel('Tegangan (V)')
    ax1.set_title('DAC Triangle Wave - Tegangan vs Waktu')
    ax1.set_ylim(-0.1, 3.5)
    ax1.grid(True, alpha=0.3)
    ax1.legend()

    # Grafik nilai DAC/PWM vs waktu
    ax2.plot(waktu, nilai, 'm-', linewidth=1.5, label='Nilai DAC/PWM')
    ax2.set_xlabel('Waktu (detik)')
    ax2.set_ylabel('Nilai (0-4095)')
    ax2.set_title('Nilai DAC/PWM vs Waktu')
    ax2.grid(True, alpha=0.3)
    ax2.legend()

    plt.tight_layout()
    plt.savefig('dac_triangle_wave.png', dpi=150)
    plt.show()
    print("[INFO] Grafik disimpan ke dac_triangle_wave.png")

def main():
    """Fungsi utama."""
    print("=" * 55)
    print(" Debug Analysis - DAC Triangle Wave")
    print(" Modul 05 - DAC & PWM")
    print("=" * 55)

    port = sys.argv[1] if len(sys.argv) > 1 else SERIAL_PORT
    durasi = int(sys.argv[2]) if len(sys.argv) > 2 else 30

    data = baca_serial(port=port, durasi=durasi)
    simpan_csv(data)
    plot_grafik(data)

if __name__ == '__main__':
    main()
