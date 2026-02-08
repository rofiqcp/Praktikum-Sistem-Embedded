"""
==========================================================
 Modul 06 - STM32_04_I2C_Accel_MPU6050
 Debug & Analisis: Parser serial, logging CSV, plot IMU
==========================================================
"""

import serial
import csv
import re
import sys
import time
from datetime import datetime

try:
    import matplotlib.pyplot as plt
    HAS_MATPLOTLIB = True
except ImportError:
    HAS_MATPLOTLIB = False
    print("[WARN] matplotlib tidak tersedia")


def parse_serial(port='/dev/ttyUSB0', baudrate=115200, csv_file='mpu6050_log.csv'):
    """Membaca data MPU6050 dan menyimpan ke CSV"""
    print(f"[INFO] Membuka port {port} @ {baudrate} baud")

    try:
        ser = serial.Serial(port, baudrate, timeout=2)
        time.sleep(2)
        print("[INFO] Menunggu data MPU6050...")

        with open(csv_file, 'w', newline='') as f:
            writer = csv.writer(f)
            writer.writerow(['timestamp', 'reading', 'ax', 'ay', 'az', 'gx', 'gy', 'gz', 'temp'])

            while True:
                line = ser.readline().decode('utf-8', errors='replace').strip()
                if not line:
                    continue
                print(f"[SERIAL] {line}")

                match = re.search(r'DATA,(\d+),([\d.-]+),([\d.-]+),([\d.-]+),([\d.-]+),([\d.-]+),([\d.-]+),([\d.-]+)', line)
                if match:
                    reading = int(match.group(1))
                    ax, ay, az = float(match.group(2)), float(match.group(3)), float(match.group(4))
                    gx, gy, gz = float(match.group(5)), float(match.group(6)), float(match.group(7))
                    temp = float(match.group(8))
                    ts = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
                    writer.writerow([ts, reading, ax, ay, az, gx, gy, gz, temp])
                    f.flush()

    except serial.SerialException as e:
        print(f"[ERROR] {e}")
    except KeyboardInterrupt:
        print("\n[INFO] Dihentikan oleh pengguna")
    finally:
        if 'ser' in locals() and ser.is_open:
            ser.close()


def plot_hasil(csv_file='mpu6050_log.csv'):
    """Plot data akselerometer dan giroskop"""
    if not HAS_MATPLOTLIB:
        print("[ERROR] matplotlib diperlukan")
        return

    data = {'reading': [], 'ax': [], 'ay': [], 'az': [], 'gx': [], 'gy': [], 'gz': [], 'temp': []}
    try:
        with open(csv_file, 'r') as f:
            reader = csv.DictReader(f)
            for row in reader:
                for key in data:
                    data[key].append(float(row[key]))
    except FileNotFoundError:
        print(f"[ERROR] File {csv_file} tidak ditemukan")
        return

    fig, axes = plt.subplots(3, 1, figsize=(12, 10), sharex=True)

    axes[0].plot(data['reading'], data['ax'], 'r-', label='AX', alpha=0.8)
    axes[0].plot(data['reading'], data['ay'], 'g-', label='AY', alpha=0.8)
    axes[0].plot(data['reading'], data['az'], 'b-', label='AZ', alpha=0.8)
    axes[0].set_ylabel('Akselerasi (g)')
    axes[0].set_title('MPU6050 - Data IMU')
    axes[0].legend()
    axes[0].grid(True, alpha=0.3)

    axes[1].plot(data['reading'], data['gx'], 'r-', label='GX', alpha=0.8)
    axes[1].plot(data['reading'], data['gy'], 'g-', label='GY', alpha=0.8)
    axes[1].plot(data['reading'], data['gz'], 'b-', label='GZ', alpha=0.8)
    axes[1].set_ylabel('Kecepatan Sudut (dps)')
    axes[1].legend()
    axes[1].grid(True, alpha=0.3)

    axes[2].plot(data['reading'], data['temp'], 'm-', label='Suhu')
    axes[2].set_ylabel('Suhu (°C)')
    axes[2].set_xlabel('Pembacaan #')
    axes[2].legend()
    axes[2].grid(True, alpha=0.3)

    plt.tight_layout()
    plt.savefig('mpu6050_plot.png', dpi=150)
    print("[INFO] Plot disimpan ke mpu6050_plot.png")
    plt.show()


if __name__ == '__main__':
    if len(sys.argv) > 1 and sys.argv[1] == 'plot':
        csv_f = sys.argv[2] if len(sys.argv) > 2 else 'mpu6050_log.csv'
        plot_hasil(csv_f)
    else:
        port = sys.argv[1] if len(sys.argv) > 1 else '/dev/ttyUSB0'
        parse_serial(port=port)
