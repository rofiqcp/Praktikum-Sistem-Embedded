"""
ESP32_09_I2C_Multi_Sensor - Debug Analysis
Modul 06 - I2C & Sensor

Deskripsi: Parser serial + analisis data multi-sensor (BMP280 + BH1750).
           Grafik suhu, tekanan, cahaya secara bersamaan.
           Korelasi antar-sensor dan penyimpanan ke CSV.

Penggunaan:
    python debug_analysis.py [PORT]
    Contoh: python debug_analysis.py /dev/ttyUSB0
"""

import serial
import csv
import time
import sys
import os
from datetime import datetime

# Konfigurasi default
DEFAULT_PORT = '/dev/ttyUSB0'
BAUD_RATE = 115200
CSV_FILENAME = 'multi_sensor_data.csv'
PLOT_FILENAME = 'multi_sensor_plot.png'
MAX_SAMPLES = 500

def parse_serial_data(port, duration=120):
    """Membaca dan mem-parsing data serial multi-sensor dari ESP32."""
    data_list = []

    print(f"[INFO] Membuka port serial {port} pada {BAUD_RATE} baud...")
    try:
        ser = serial.Serial(port, BAUD_RATE, timeout=2)
        time.sleep(2)
        ser.reset_input_buffer()
        print(f"[INFO] Port serial terbuka. Membaca data selama {duration} detik...")
        print(f"[INFO] Tekan Ctrl+C untuk berhenti lebih awal.\n")
    except serial.SerialException as e:
        print(f"[ERROR] Gagal membuka port serial: {e}")
        return data_list

    start_time = time.time()
    sample_count = 0

    try:
        while (time.time() - start_time) < duration and sample_count < MAX_SAMPLES:
            if ser.in_waiting > 0:
                line = ser.readline().decode('utf-8', errors='ignore').strip()
                if not line:
                    continue

                print(f"  RAW: {line}")

                # Parse format: DATA,sample,suhu_C,tekanan_hPa,cahaya_lux,klasifikasi
                if line.startswith('DATA,'):
                    parts = line.split(',')
                    if len(parts) >= 6:
                        try:
                            sample_num = int(parts[1])

                            # Cek apakah ada error
                            if 'ERR' in parts[2]:
                                print(f"  [WARN] Sampel #{sample_num} mengandung error")
                                continue

                            suhu = float(parts[2])
                            tekanan = float(parts[3])
                            cahaya = float(parts[4])
                            klasifikasi = parts[5]
                            timestamp = time.time() - start_time

                            entry = {
                                'timestamp': timestamp,
                                'sample': sample_num,
                                'suhu_C': suhu,
                                'tekanan_hPa': tekanan,
                                'cahaya_lux': cahaya,
                                'klasifikasi': klasifikasi
                            }
                            data_list.append(entry)
                            sample_count += 1

                            print(f"  [DATA] #{sample_num}: T={suhu:.1f}°C "
                                  f"P={tekanan:.1f}hPa L={cahaya:.1f}lux [{klasifikasi}]")
                        except (ValueError, IndexError) as e:
                            print(f"  [WARN] Gagal parse: {e}")

    except KeyboardInterrupt:
        print("\n[INFO] Pembacaan dihentikan oleh pengguna.")
    finally:
        ser.close()
        print(f"[INFO] Port serial ditutup. Total sampel: {sample_count}")

    return data_list


def save_to_csv(data_list, filename):
    """Menyimpan data ke file CSV."""
    filepath = os.path.join(os.path.dirname(__file__), filename)
    with open(filepath, 'w', newline='') as csvfile:
        writer = csv.DictWriter(csvfile,
                                fieldnames=['timestamp', 'sample', 'suhu_C',
                                            'tekanan_hPa', 'cahaya_lux', 'klasifikasi'])
        writer.writeheader()
        writer.writerows(data_list)

    print(f"[INFO] Data disimpan ke {filepath} ({len(data_list)} baris)")
    return filepath


def create_plots(data_list, filename):
    """Membuat grafik analisis multi-sensor."""
    try:
        import matplotlib
        matplotlib.use('Agg')
        import matplotlib.pyplot as plt
    except ImportError:
        print("[WARN] matplotlib tidak tersedia. Grafik tidak dibuat.")
        print("       Install dengan: pip install matplotlib")
        return

    if not data_list:
        print("[WARN] Tidak ada data untuk grafik.")
        return

    times = [d['timestamp'] for d in data_list]
    suhu = [d['suhu_C'] for d in data_list]
    tekanan = [d['tekanan_hPa'] for d in data_list]
    cahaya = [d['cahaya_lux'] for d in data_list]

    fig, axes = plt.subplots(3, 2, figsize=(15, 12))
    fig.suptitle('Analisis Multi-Sensor I2C (BMP280 + BH1750) - ESP32',
                 fontsize=14, fontweight='bold')

    # Plot 1: Suhu vs Waktu
    ax1 = axes[0, 0]
    ax1.plot(times, suhu, 'r-o', markersize=3, linewidth=1, label='BMP280')
    ax1.set_xlabel('Waktu (detik)')
    ax1.set_ylabel('Suhu (°C)')
    ax1.set_title('Suhu (BMP280)')
    ax1.grid(True, alpha=0.3)
    ax1.legend()

    # Plot 2: Tekanan vs Waktu
    ax2 = axes[0, 1]
    ax2.plot(times, tekanan, 'b-o', markersize=3, linewidth=1, label='BMP280')
    ax2.set_xlabel('Waktu (detik)')
    ax2.set_ylabel('Tekanan (hPa)')
    ax2.set_title('Tekanan Udara (BMP280)')
    ax2.grid(True, alpha=0.3)
    ax2.legend()

    # Plot 3: Cahaya vs Waktu
    ax3 = axes[1, 0]
    ax3.plot(times, cahaya, 'g-o', markersize=3, linewidth=1, label='BH1750')
    ax3.set_xlabel('Waktu (detik)')
    ax3.set_ylabel('Cahaya (lux)')
    ax3.set_title('Intensitas Cahaya (BH1750)')
    ax3.grid(True, alpha=0.3)
    ax3.legend()

    # Plot 4: Semua sensor dalam satu grafik (normalized)
    ax4 = axes[1, 1]
    if len(suhu) > 0:
        suhu_min, suhu_max = min(suhu), max(suhu)
        tek_min, tek_max = min(tekanan), max(tekanan)
        cah_min, cah_max = min(cahaya), max(cahaya)

        def normalize(vals, vmin, vmax):
            rng = vmax - vmin if vmax != vmin else 1
            return [(v - vmin) / rng for v in vals]

        suhu_n = normalize(suhu, suhu_min, suhu_max)
        tek_n = normalize(tekanan, tek_min, tek_max)
        cah_n = normalize(cahaya, cah_min, cah_max)

        ax4.plot(times, suhu_n, 'r-', linewidth=1, alpha=0.8, label='Suhu')
        ax4.plot(times, tek_n, 'b-', linewidth=1, alpha=0.8, label='Tekanan')
        ax4.plot(times, cah_n, 'g-', linewidth=1, alpha=0.8, label='Cahaya')
        ax4.set_xlabel('Waktu (detik)')
        ax4.set_ylabel('Nilai Normalisasi (0-1)')
        ax4.set_title('Semua Sensor (Normalisasi)')
        ax4.legend()
        ax4.grid(True, alpha=0.3)

    # Plot 5: Korelasi Suhu vs Cahaya
    ax5 = axes[2, 0]
    ax5.scatter(suhu, cahaya, c=times, cmap='viridis', s=20, alpha=0.7)
    ax5.set_xlabel('Suhu (°C)')
    ax5.set_ylabel('Cahaya (lux)')
    ax5.set_title('Korelasi Suhu vs Cahaya')
    ax5.grid(True, alpha=0.3)
    cbar = plt.colorbar(ax5.collections[0], ax=ax5, label='Waktu (s)')

    # Plot 6: Statistik Box Plot
    ax6 = axes[2, 1]
    bp_data = [suhu, [t/10 for t in tekanan], cahaya]  # Skala tekanan /10
    bp = ax6.boxplot(bp_data, labels=['Suhu (°C)', 'Tekanan (x10 hPa)', 'Cahaya (lux)'],
                     patch_artist=True)
    colors_bp = ['#ff6b6b', '#4ecdc4', '#45b7d1']
    for patch, color in zip(bp['boxes'], colors_bp):
        patch.set_facecolor(color)
        patch.set_alpha(0.7)
    ax6.set_title('Distribusi Nilai Sensor')
    ax6.grid(True, alpha=0.3)

    plt.tight_layout()
    filepath = os.path.join(os.path.dirname(__file__), filename)
    plt.savefig(filepath, dpi=150, bbox_inches='tight')
    plt.close()
    print(f"[INFO] Grafik disimpan ke {filepath}")


def print_summary(data_list):
    """Menampilkan ringkasan analisis."""
    print("\n" + "=" * 60)
    print("       RINGKASAN ANALISIS MULTI-SENSOR I2C")
    print("=" * 60)

    if not data_list:
        print("  Tidak ada data untuk dianalisis.")
        return

    suhu = [d['suhu_C'] for d in data_list]
    tekanan = [d['tekanan_hPa'] for d in data_list]
    cahaya = [d['cahaya_lux'] for d in data_list]

    print(f"\n  Total sampel: {len(data_list)}")
    print(f"  Durasi: {data_list[-1]['timestamp'] - data_list[0]['timestamp']:.1f} detik")

    print(f"\n  BMP280 - Suhu:")
    print(f"    Min: {min(suhu):.2f}°C | Max: {max(suhu):.2f}°C | "
          f"Avg: {sum(suhu)/len(suhu):.2f}°C")

    print(f"\n  BMP280 - Tekanan:")
    print(f"    Min: {min(tekanan):.2f} hPa | Max: {max(tekanan):.2f} hPa | "
          f"Avg: {sum(tekanan)/len(tekanan):.2f} hPa")

    print(f"\n  BH1750 - Cahaya:")
    print(f"    Min: {min(cahaya):.2f} lux | Max: {max(cahaya):.2f} lux | "
          f"Avg: {sum(cahaya)/len(cahaya):.2f} lux")

    # Klasifikasi
    from collections import Counter
    klas_count = Counter(d['klasifikasi'] for d in data_list)
    print(f"\n  Klasifikasi Cahaya:")
    for klas, cnt in klas_count.most_common():
        print(f"    {klas:20s}: {cnt}")

    print("=" * 60)


def main():
    """Fungsi utama program."""
    port = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_PORT
    duration = int(sys.argv[2]) if len(sys.argv) > 2 else 120

    print("=" * 60)
    print("  ESP32 Multi-Sensor (BMP280 + BH1750) - Debug Analysis")
    print(f"  Port: {port} | Durasi: {duration}s")
    print("=" * 60)

    data_list = parse_serial_data(port, duration)

    if not data_list:
        print("[WARN] Tidak ada data yang terkumpul.")
        return

    save_to_csv(data_list, CSV_FILENAME)
    create_plots(data_list, PLOT_FILENAME)
    print_summary(data_list)


if __name__ == '__main__':
    main()
