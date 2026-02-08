"""
==========================================================
 Debug Analysis Script - PWM Frequency Sweep
 Modul 05 - DAC & PWM
 Deskripsi: Membaca data sweep frekuensi dari serial,
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

def baca_serial(port=SERIAL_PORT, baud=BAUD_RATE, durasi=60):
    """Membaca data sweep frekuensi dari serial port."""
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

            # Parsing: [SWEEP] Target= 1000 Hz | Aktual= 1000 Hz | PSC=0 | ARR=71999
            match = re.search(
                r'Target=\s*(\d+)\s*Hz\s*\|\s*Aktual=\s*(\d+)\s*Hz\s*\|\s*PSC=(\d+)\s*\|\s*ARR=(\d+)',
                line
            )
            if match:
                target = int(match.group(1))
                aktual = int(match.group(2))
                psc = int(match.group(3))
                arr = int(match.group(4))
                waktu = time.time() - waktu_mulai
                error_pct = abs(target - aktual) / target * 100 if target > 0 else 0
                data_list.append({
                    'waktu': round(waktu, 2),
                    'target_hz': target,
                    'aktual_hz': aktual,
                    'psc': psc,
                    'arr': arr,
                    'error_pct': round(error_pct, 2)
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

def simpan_csv(data_list, nama_file='pwm_frequency_sweep.csv'):
    """Menyimpan data ke file CSV."""
    if not data_list:
        print("[WARNING] Tidak ada data untuk disimpan")
        return

    with open(nama_file, 'w', newline='') as f:
        writer = csv.DictWriter(f, fieldnames=['waktu', 'target_hz', 'aktual_hz', 'psc', 'arr', 'error_pct'])
        writer.writeheader()
        writer.writerows(data_list)

    print(f"[INFO] Data disimpan ke {nama_file}")

def plot_grafik(data_list):
    """Menampilkan grafik frequency sweep."""
    if not data_list:
        print("[WARNING] Tidak ada data untuk ditampilkan")
        return

    waktu = [d['waktu'] for d in data_list]
    target = [d['target_hz'] for d in data_list]
    aktual = [d['aktual_hz'] for d in data_list]
    error = [d['error_pct'] for d in data_list]

    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(10, 8))

    # Grafik frekuensi vs waktu
    ax1.plot(waktu, target, 'b--', label='Target', alpha=0.7)
    ax1.plot(waktu, aktual, 'r-o', markersize=3, label='Aktual')
    ax1.set_xlabel('Waktu (detik)')
    ax1.set_ylabel('Frekuensi (Hz)')
    ax1.set_title('PWM Frequency Sweep - Target vs Aktual')
    ax1.set_yscale('log')
    ax1.grid(True, alpha=0.3)
    ax1.legend()

    # Grafik error
    ax2.bar(range(len(error)), error, color='orange', alpha=0.7)
    ax2.set_xlabel('Sampel ke-')
    ax2.set_ylabel('Error (%)')
    ax2.set_title('Error Frekuensi (Target vs Aktual)')
    ax2.grid(True, alpha=0.3, axis='y')

    plt.tight_layout()
    plt.savefig('pwm_frequency_sweep.png', dpi=150)
    plt.show()
    print("[INFO] Grafik disimpan ke pwm_frequency_sweep.png")

def main():
    """Fungsi utama."""
    print("=" * 55)
    print(" Debug Analysis - PWM Frequency Sweep")
    print(" Modul 05 - DAC & PWM")
    print("=" * 55)

    port = sys.argv[1] if len(sys.argv) > 1 else SERIAL_PORT
    durasi = int(sys.argv[2]) if len(sys.argv) > 2 else 60

    data = baca_serial(port=port, durasi=durasi)
    simpan_csv(data)
    plot_grafik(data)

if __name__ == '__main__':
    main()
