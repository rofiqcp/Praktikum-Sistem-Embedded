"""
==========================================================
 Debug Analysis Script - PWM LED Brightness Control
 Modul 05 - DAC & PWM
 Deskripsi: Mengirim perintah brightness via serial,
            membaca respons, dan menampilkan grafik.
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

def baca_dan_kirim(port=SERIAL_PORT, baud=BAUD_RATE):
    """Mengirim perintah brightness dan membaca respons."""
    data_list = []
    try:
        ser = serial.Serial(port, baud, timeout=TIMEOUT)
        print(f"[INFO] Terhubung ke {port} @ {baud} baud")
        time.sleep(2)  # Tunggu MCU boot

        # Baca pesan awal
        while ser.in_waiting:
            line = ser.readline().decode('utf-8', errors='ignore').strip()
            if line:
                print(f"  >> {line}")

        # Kirim serangkaian nilai brightness
        test_values = [0, 10, 25, 50, 75, 100, 75, 50, 25, 0]
        print(f"\n[INFO] Mengirim {len(test_values)} perintah brightness...\n")

        for val in test_values:
            cmd = f"{val}\r\n"
            ser.write(cmd.encode())
            print(f"  << Kirim: {val}%")

            time.sleep(0.5)

            # Baca respons
            while ser.in_waiting:
                line = ser.readline().decode('utf-8', errors='ignore').strip()
                if line:
                    print(f"  >> {line}")

                    # Parsing: [BRIGHTNESS] Set: 50% (duty=500/999)
                    match = re.search(r'Set:\s*(\d+)%\s*\(duty=(\d+)/\d+\)', line)
                    if match:
                        brightness = int(match.group(1))
                        duty = int(match.group(2))
                        waktu = time.time()
                        data_list.append({
                            'waktu': round(waktu, 2),
                            'brightness': brightness,
                            'duty': duty
                        })

            time.sleep(1)

        ser.close()
        print(f"\n[INFO] Selesai. Total data: {len(data_list)} respons")

    except serial.SerialException as e:
        print(f"[ERROR] Gagal membuka serial port: {e}")
    except KeyboardInterrupt:
        print("\n[INFO] Dihentikan oleh pengguna")
        if 'ser' in locals():
            ser.close()

    return data_list

def simpan_csv(data_list, nama_file='pwm_led_brightness.csv'):
    """Menyimpan data ke file CSV."""
    if not data_list:
        print("[WARNING] Tidak ada data untuk disimpan")
        return

    with open(nama_file, 'w', newline='') as f:
        writer = csv.DictWriter(f, fieldnames=['waktu', 'brightness', 'duty'])
        writer.writeheader()
        writer.writerows(data_list)

    print(f"[INFO] Data disimpan ke {nama_file}")

def plot_grafik(data_list):
    """Menampilkan grafik brightness control."""
    if not data_list:
        print("[WARNING] Tidak ada data untuk ditampilkan")
        return

    index = list(range(len(data_list)))
    brightness = [d['brightness'] for d in data_list]

    fig, ax = plt.subplots(figsize=(10, 5))

    ax.bar(index, brightness, color='orange', alpha=0.7, edgecolor='darkorange')
    ax.set_xlabel('Perintah ke-')
    ax.set_ylabel('Brightness (%)')
    ax.set_title('PWM LED Brightness Control via UART')
    ax.set_ylim(0, 110)
    ax.set_xticks(index)
    ax.grid(True, alpha=0.3, axis='y')

    # Tambahkan label nilai
    for i, v in enumerate(brightness):
        ax.text(i, v + 2, f'{v}%', ha='center', fontsize=9)

    plt.tight_layout()
    plt.savefig('pwm_led_brightness.png', dpi=150)
    plt.show()
    print("[INFO] Grafik disimpan ke pwm_led_brightness.png")

def main():
    """Fungsi utama."""
    print("=" * 55)
    print(" Debug Analysis - PWM LED Brightness Control")
    print(" Modul 05 - DAC & PWM")
    print("=" * 55)

    port = sys.argv[1] if len(sys.argv) > 1 else SERIAL_PORT

    data = baca_dan_kirim(port=port)
    simpan_csv(data)
    plot_grafik(data)

if __name__ == '__main__':
    main()
