"""
==========================================================
 Debug Analysis Script - PWM LED Breathing
 Modul 05 - DAC & PWM
 Deskripsi: Membaca data breathing LED dari serial,
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
    """Membaca data breathing LED dari serial port."""
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

            # Parsing: [BREATH] Duty=500/1000 ( 50.0%) | Arah=Naik
            match = re.search(r'Duty=\s*(\d+)/\d+\s*\(\s*([\d.]+)%\)\s*\|\s*Arah=(\w+)', line)
            if match:
                duty = int(match.group(1))
                persen = float(match.group(2))
                arah = match.group(3)
                waktu = time.time() - waktu_mulai
                data_list.append({
                    'waktu': round(waktu, 3),
                    'duty': duty,
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

def simpan_csv(data_list, nama_file='pwm_led_breathing.csv'):
    """Menyimpan data ke file CSV."""
    if not data_list:
        print("[WARNING] Tidak ada data untuk disimpan")
        return

    with open(nama_file, 'w', newline='') as f:
        writer = csv.DictWriter(f, fieldnames=['waktu', 'duty', 'persen', 'arah'])
        writer.writeheader()
        writer.writerows(data_list)

    print(f"[INFO] Data disimpan ke {nama_file}")

def plot_grafik(data_list):
    """Menampilkan grafik efek breathing."""
    if not data_list:
        print("[WARNING] Tidak ada data untuk ditampilkan")
        return

    waktu = [d['waktu'] for d in data_list]
    persen = [d['persen'] for d in data_list]

    fig, ax = plt.subplots(figsize=(10, 5))

    # Warnai berdasarkan arah
    for i in range(len(waktu) - 1):
        color = 'green' if data_list[i]['arah'] == 'Naik' else 'red'
        ax.plot(waktu[i:i+2], persen[i:i+2], color=color, linewidth=2)

    ax.set_xlabel('Waktu (detik)')
    ax.set_ylabel('Duty Cycle (%)')
    ax.set_title('PWM LED Breathing - Efek Nafas')
    ax.set_ylim(-5, 105)
    ax.grid(True, alpha=0.3)

    # Legend manual
    from matplotlib.lines import Line2D
    legend_elements = [
        Line2D([0], [0], color='green', linewidth=2, label='Fade In (Naik)'),
        Line2D([0], [0], color='red', linewidth=2, label='Fade Out (Turun)')
    ]
    ax.legend(handles=legend_elements)

    plt.tight_layout()
    plt.savefig('pwm_led_breathing.png', dpi=150)
    plt.show()
    print("[INFO] Grafik disimpan ke pwm_led_breathing.png")

def main():
    """Fungsi utama."""
    print("=" * 55)
    print(" Debug Analysis - PWM LED Breathing")
    print(" Modul 05 - DAC & PWM")
    print("=" * 55)

    port = sys.argv[1] if len(sys.argv) > 1 else SERIAL_PORT
    durasi = int(sys.argv[2]) if len(sys.argv) > 2 else 30

    data = baca_serial(port=port, durasi=durasi)
    simpan_csv(data)
    plot_grafik(data)

if __name__ == '__main__':
    main()
