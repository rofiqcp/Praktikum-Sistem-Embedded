"""
==========================================================
 Debug Analysis Script - PWM Servo Control
 Modul 05 - DAC & PWM
 Deskripsi: Membaca data posisi servo dari serial,
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
    """Membaca data servo dari serial port."""
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

            # Parsing: [SERVO] Sudut=  90 deg | Pulse=1500 us | Arah=CW
            match = re.search(r'Sudut=\s*(\d+)\s*deg\s*\|\s*Pulse=\s*(\d+)\s*us\s*\|\s*Arah=(\w+)', line)
            if match:
                sudut = int(match.group(1))
                pulse = int(match.group(2))
                arah = match.group(3).strip()
                waktu = time.time() - waktu_mulai
                data_list.append({
                    'waktu': round(waktu, 3),
                    'sudut': sudut,
                    'pulse_us': pulse,
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

def simpan_csv(data_list, nama_file='pwm_servo_control.csv'):
    """Menyimpan data ke file CSV."""
    if not data_list:
        print("[WARNING] Tidak ada data untuk disimpan")
        return

    with open(nama_file, 'w', newline='') as f:
        writer = csv.DictWriter(f, fieldnames=['waktu', 'sudut', 'pulse_us', 'arah'])
        writer.writeheader()
        writer.writerows(data_list)

    print(f"[INFO] Data disimpan ke {nama_file}")

def plot_grafik(data_list):
    """Menampilkan grafik posisi servo."""
    if not data_list:
        print("[WARNING] Tidak ada data untuk ditampilkan")
        return

    waktu = [d['waktu'] for d in data_list]
    sudut = [d['sudut'] for d in data_list]
    pulse = [d['pulse_us'] for d in data_list]

    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(10, 8))

    # Grafik sudut vs waktu
    ax1.plot(waktu, sudut, 'b-o', markersize=4, label='Sudut Servo')
    ax1.set_xlabel('Waktu (detik)')
    ax1.set_ylabel('Sudut (derajat)')
    ax1.set_title('PWM Servo Control - Sudut vs Waktu')
    ax1.set_ylim(-10, 190)
    ax1.axhline(y=0, color='r', linestyle='--', alpha=0.3)
    ax1.axhline(y=90, color='g', linestyle='--', alpha=0.3, label='90° (tengah)')
    ax1.axhline(y=180, color='r', linestyle='--', alpha=0.3)
    ax1.grid(True, alpha=0.3)
    ax1.legend()

    # Grafik pulse width vs sudut
    ax2.plot(sudut, pulse, 'r-s', markersize=4, label='Pulse Width')
    ax2.set_xlabel('Sudut (derajat)')
    ax2.set_ylabel('Pulse Width (us)')
    ax2.set_title('Linearitas Servo - Sudut vs Pulse Width')
    ax2.grid(True, alpha=0.3)
    ax2.legend()

    plt.tight_layout()
    plt.savefig('pwm_servo_control.png', dpi=150)
    plt.show()
    print("[INFO] Grafik disimpan ke pwm_servo_control.png")

def main():
    """Fungsi utama."""
    print("=" * 55)
    print(" Debug Analysis - PWM Servo Control")
    print(" Modul 05 - DAC & PWM")
    print("=" * 55)

    port = sys.argv[1] if len(sys.argv) > 1 else SERIAL_PORT
    durasi = int(sys.argv[2]) if len(sys.argv) > 2 else 30

    data = baca_serial(port=port, durasi=durasi)
    simpan_csv(data)
    plot_grafik(data)

if __name__ == '__main__':
    main()
