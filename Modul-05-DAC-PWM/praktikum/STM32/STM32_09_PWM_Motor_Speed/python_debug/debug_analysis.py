"""
==========================================================
 Debug Analysis Script - PWM Motor Speed Control
 Modul 05 - DAC & PWM
 Deskripsi: Membaca data kecepatan motor dari serial,
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
    """Membaca data kecepatan motor dari serial port."""
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

            # Parsing: [MOTOR] Kecepatan=500/999 ( 50.1%) | Arah=MAJU
            match = re.search(r'Kecepatan=\s*(\d+)/\d+\s*\(\s*([\d.]+)%\)\s*\|\s*Arah=(\w+)', line)
            if match:
                kecepatan = int(match.group(1))
                persen = float(match.group(2))
                arah = match.group(3)
                waktu = time.time() - waktu_mulai
                data_list.append({
                    'waktu': round(waktu, 2),
                    'kecepatan': kecepatan,
                    'persen': persen,
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

def simpan_csv(data_list, nama_file='pwm_motor_speed.csv'):
    """Menyimpan data ke file CSV."""
    if not data_list:
        print("[WARNING] Tidak ada data untuk disimpan")
        return

    with open(nama_file, 'w', newline='') as f:
        writer = csv.DictWriter(f, fieldnames=['waktu', 'kecepatan', 'persen', 'arah'])
        writer.writeheader()
        writer.writerows(data_list)

    print(f"[INFO] Data disimpan ke {nama_file}")

def plot_grafik(data_list):
    """Menampilkan grafik kecepatan motor."""
    if not data_list:
        print("[WARNING] Tidak ada data untuk ditampilkan")
        return

    waktu = [d['waktu'] for d in data_list]
    persen = [d['persen'] for d in data_list]
    arah = [d['arah'] for d in data_list]

    # Tandai arah: positif untuk maju, negatif untuk mundur
    persen_signed = [p if a == 'MAJU' else -p for p, a in zip(persen, arah)]

    fig, ax = plt.subplots(figsize=(10, 5))

    colors = ['green' if a == 'MAJU' else 'red' for a in arah]
    ax.scatter(waktu, persen_signed, c=colors, s=10, alpha=0.7)
    ax.plot(waktu, persen_signed, 'b-', alpha=0.3, linewidth=0.5)
    ax.set_xlabel('Waktu (detik)')
    ax.set_ylabel('Kecepatan (%) [+ Maju, - Mundur]')
    ax.set_title('PWM Motor Speed Control - Kecepatan vs Waktu')
    ax.axhline(y=0, color='black', linewidth=0.5)
    ax.grid(True, alpha=0.3)

    from matplotlib.lines import Line2D
    legend_elements = [
        Line2D([0], [0], marker='o', color='w', markerfacecolor='green', markersize=8, label='Maju'),
        Line2D([0], [0], marker='o', color='w', markerfacecolor='red', markersize=8, label='Mundur')
    ]
    ax.legend(handles=legend_elements)

    plt.tight_layout()
    plt.savefig('pwm_motor_speed.png', dpi=150)
    plt.show()
    print("[INFO] Grafik disimpan ke pwm_motor_speed.png")

def main():
    """Fungsi utama."""
    print("=" * 55)
    print(" Debug Analysis - PWM Motor Speed Control")
    print(" Modul 05 - DAC & PWM")
    print("=" * 55)

    port = sys.argv[1] if len(sys.argv) > 1 else SERIAL_PORT
    durasi = int(sys.argv[2]) if len(sys.argv) > 2 else 60

    data = baca_serial(port=port, durasi=durasi)
    simpan_csv(data)
    plot_grafik(data)

if __name__ == '__main__':
    main()
