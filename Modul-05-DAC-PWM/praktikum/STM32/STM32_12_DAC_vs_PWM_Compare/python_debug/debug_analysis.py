"""
==========================================================
 Script      : debug_analysis.py
 Program     : STM32_12_DAC_vs_PWM_Compare
 Modul       : 05 - DAC & PWM
 Deskripsi   : Membaca data serial perbandingan DAC vs PWM,
               menampilkan grafik error perbandingan,
               dan menyimpan data ke CSV.
==========================================================
"""

import serial
import csv
import re
import time
import sys
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from datetime import datetime

# ==================== Konfigurasi ====================
SERIAL_PORT = '/dev/ttyUSB0'     # Sesuaikan port
BAUD_RATE = 115200
CSV_FILE = 'dac_vs_pwm_compare_log.csv'
TIMEOUT = 1

# ==================== Pola Regex ====================
# Format F4: [CMP] Target=XXXX | DAC_RB=XXXX | PWM_RB=XXXX | ErrDAC=+X.X% | ErrPWM=+X.X%
# Format F1: [CMP] Target=XXXX | PWM1_RB=XXXX | PWM2_RB=XXXX | Err1=+X.X% | Err2=+X.X%
pattern_f4 = re.compile(
    r'\[CMP\]\s*Target=\s*(\d+)\s*\|\s*DAC_RB=\s*(\d+)\s*\|\s*PWM_RB=\s*(\d+)\s*\|\s*ErrDAC=\s*([+-]?\d+\.?\d*)%\s*\|\s*ErrPWM=\s*([+-]?\d+\.?\d*)%'
)
pattern_f1 = re.compile(
    r'\[CMP\]\s*Target=\s*(\d+)\s*\|\s*PWM1_RB=\s*(\d+)\s*\|\s*PWM2_RB=\s*(\d+)\s*\|\s*Err1=\s*([+-]?\d+\.?\d*)%\s*\|\s*Err2=\s*([+-]?\d+\.?\d*)%'
)
pattern_result = re.compile(
    r'\[RESULT\]\s*Rata-rata Error\s+(\w+)\s*:\s*([0-9.]+)%'
)

# ==================== Penyimpanan Data ====================
data_list = []
targets = []
readback_ch1 = []
readback_ch2 = []
errors_ch1 = []
errors_ch2 = []
avg_results = []

# ==================== Deteksi Mode (F4 atau F1) ====================
is_f4_mode = None  # None = belum terdeteksi

def parse_line(line):
    """Parse satu baris data serial."""
    global is_f4_mode

    # Coba parse format F4
    match_f4 = pattern_f4.search(line)
    if match_f4:
        is_f4_mode = True
        target = int(match_f4.group(1))
        rb1 = int(match_f4.group(2))  # DAC readback
        rb2 = int(match_f4.group(3))  # PWM readback
        err1 = float(match_f4.group(4))
        err2 = float(match_f4.group(5))

        voltage_target = target / 4095.0 * 3.3
        voltage_rb1 = rb1 / 4095.0 * 3.3
        voltage_rb2 = rb2 / 4095.0 * 3.3

        entry = {
            'timestamp': datetime.now().strftime('%H:%M:%S.%f')[:-3],
            'target': target,
            'target_V': round(voltage_target, 3),
            'dac_readback': rb1,
            'dac_V': round(voltage_rb1, 3),
            'pwm_readback': rb2,
            'pwm_V': round(voltage_rb2, 3),
            'error_dac_pct': err1,
            'error_pwm_pct': err2
        }
        data_list.append(entry)
        targets.append(target)
        readback_ch1.append(rb1)
        readback_ch2.append(rb2)
        errors_ch1.append(err1)
        errors_ch2.append(err2)

        print(f"  Target={target:4d} ({voltage_target:.2f}V) | "
              f"DAC={rb1:4d} ({voltage_rb1:.2f}V) Err={err1:+.1f}% | "
              f"PWM={rb2:4d} ({voltage_rb2:.2f}V) Err={err2:+.1f}%")
        return True

    # Coba parse format F1
    match_f1 = pattern_f1.search(line)
    if match_f1:
        is_f4_mode = False
        target = int(match_f1.group(1))
        rb1 = int(match_f1.group(2))
        rb2 = int(match_f1.group(3))
        err1 = float(match_f1.group(4))
        err2 = float(match_f1.group(5))

        voltage_target = target / 4095.0 * 3.3
        voltage_rb1 = rb1 / 4095.0 * 3.3
        voltage_rb2 = rb2 / 4095.0 * 3.3

        entry = {
            'timestamp': datetime.now().strftime('%H:%M:%S.%f')[:-3],
            'target': target,
            'target_V': round(voltage_target, 3),
            'pwm1_readback': rb1,
            'pwm1_V': round(voltage_rb1, 3),
            'pwm2_readback': rb2,
            'pwm2_V': round(voltage_rb2, 3),
            'error_pwm1_pct': err1,
            'error_pwm2_pct': err2
        }
        data_list.append(entry)
        targets.append(target)
        readback_ch1.append(rb1)
        readback_ch2.append(rb2)
        errors_ch1.append(err1)
        errors_ch2.append(err2)

        print(f"  Target={target:4d} ({voltage_target:.2f}V) | "
              f"PWM1={rb1:4d} ({voltage_rb1:.2f}V) Err={err1:+.1f}% | "
              f"PWM2={rb2:4d} ({voltage_rb2:.2f}V) Err={err2:+.1f}%")
        return True

    # Parse rata-rata error
    match_result = pattern_result.search(line)
    if match_result:
        name = match_result.group(1)
        value = float(match_result.group(2))
        avg_results.append({'name': name, 'avg_error': value})
        print(f"  >> Rata-rata Error {name}: {value:.2f}%")
        return True

    return False


def save_csv():
    """Simpan data ke file CSV."""
    if not data_list:
        print("Tidak ada data untuk disimpan.")
        return

    fieldnames = list(data_list[0].keys())
    with open(CSV_FILE, 'w', newline='') as f:
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(data_list)
    print(f"\nData tersimpan ke {CSV_FILE} ({len(data_list)} baris)")


def plot_comparison():
    """Buat grafik perbandingan DAC vs PWM."""
    if not targets:
        print("Tidak ada data untuk plot.")
        return

    fig, axes = plt.subplots(2, 2, figsize=(14, 10))
    fig.suptitle('Analisis Perbandingan DAC vs PWM', fontsize=14, fontweight='bold')

    label_ch1 = 'DAC' if is_f4_mode else 'PWM1'
    label_ch2 = 'PWM' if is_f4_mode else 'PWM2'

    # 1. Target vs Readback
    ax1 = axes[0][0]
    ax1.plot(targets, targets, 'k--', linewidth=1, label='Ideal')
    ax1.plot(targets, readback_ch1, 'b-o', markersize=4, label=f'{label_ch1} Readback')
    ax1.plot(targets, readback_ch2, 'r-s', markersize=4, label=f'{label_ch2} Readback')
    ax1.set_xlabel('Target (12-bit)')
    ax1.set_ylabel('ADC Readback (12-bit)')
    ax1.set_title('Target vs Readback')
    ax1.legend()
    ax1.grid(True, alpha=0.3)

    # 2. Error (%)
    ax2 = axes[0][1]
    ax2.plot(targets, errors_ch1, 'b-o', markersize=4, label=f'Error {label_ch1} (%)')
    ax2.plot(targets, errors_ch2, 'r-s', markersize=4, label=f'Error {label_ch2} (%)')
    ax2.axhline(y=0, color='k', linestyle='--', linewidth=0.8)
    ax2.set_xlabel('Target (12-bit)')
    ax2.set_ylabel('Error (%)')
    ax2.set_title('Error vs Target')
    ax2.legend()
    ax2.grid(True, alpha=0.3)

    # 3. Absolute Error
    abs_err1 = [abs(e) for e in errors_ch1]
    abs_err2 = [abs(e) for e in errors_ch2]
    ax3 = axes[1][0]
    width = 80
    x_pos = range(len(targets))
    ax3.bar([t - width/2 for t in targets], abs_err1, width=width,
            color='blue', alpha=0.6, label=f'|Error {label_ch1}|')
    ax3.bar([t + width/2 for t in targets], abs_err2, width=width,
            color='red', alpha=0.6, label=f'|Error {label_ch2}|')
    ax3.set_xlabel('Target (12-bit)')
    ax3.set_ylabel('Absolute Error (%)')
    ax3.set_title('Absolute Error Comparison')
    ax3.legend()
    ax3.grid(True, alpha=0.3)

    # 4. Tegangan aktual vs target
    v_target = [t / 4095.0 * 3.3 for t in targets]
    v_ch1 = [r / 4095.0 * 3.3 for r in readback_ch1]
    v_ch2 = [r / 4095.0 * 3.3 for r in readback_ch2]
    ax4 = axes[1][1]
    ax4.plot(v_target, v_target, 'k--', linewidth=1, label='Ideal')
    ax4.plot(v_target, v_ch1, 'b-o', markersize=4, label=f'{label_ch1} Actual (V)')
    ax4.plot(v_target, v_ch2, 'r-s', markersize=4, label=f'{label_ch2} Actual (V)')
    ax4.set_xlabel('Target Voltage (V)')
    ax4.set_ylabel('Actual Voltage (V)')
    ax4.set_title('Tegangan Target vs Aktual')
    ax4.legend()
    ax4.grid(True, alpha=0.3)

    plt.tight_layout()
    plot_file = 'dac_vs_pwm_comparison.png'
    plt.savefig(plot_file, dpi=150, bbox_inches='tight')
    print(f"Grafik tersimpan ke {plot_file}")
    plt.show()


def main():
    """Program utama - baca serial dan analisis."""
    print("=" * 60)
    print("  Debug Analisis: DAC vs PWM Compare")
    print("=" * 60)
    print(f"Port   : {SERIAL_PORT}")
    print(f"Baud   : {BAUD_RATE}")
    print(f"CSV    : {CSV_FILE}")
    print("-" * 60)

    try:
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=TIMEOUT)
        print(f"Serial terhubung pada {SERIAL_PORT}")
        time.sleep(2)  # Tunggu MCU reset
        ser.reset_input_buffer()
    except serial.SerialException as e:
        print(f"ERROR: Tidak bisa membuka {SERIAL_PORT}: {e}")
        sys.exit(1)

    print("\nMenunggu data... (Ctrl+C untuk berhenti)\n")
    cycle_count = 0

    try:
        while True:
            if ser.in_waiting > 0:
                line = ser.readline().decode('utf-8', errors='replace').strip()
                if not line:
                    continue

                # Deteksi cycle baru
                if 'Test Cycle' in line:
                    cycle_count += 1
                    print(f"\n{'='*50}")
                    print(f"  {line}")
                    print(f"{'='*50}")
                    continue

                # Parse data
                parsed = parse_line(line)

                # Jika sudah mendapatkan 2 cycle penuh, simpan dan plot
                if cycle_count >= 2 and 'RESULT' in line and 'PWM' in line:
                    print(f"\n2 cycle selesai, membuat laporan...")
                    break

    except KeyboardInterrupt:
        print("\n\nDihentikan oleh pengguna.")
    finally:
        ser.close()
        print("Serial ditutup.")

    # Simpan dan plot
    save_csv()
    plot_comparison()

    # Cetak ringkasan
    if avg_results:
        print("\n" + "=" * 50)
        print("  RINGKASAN PERBANDINGAN")
        print("=" * 50)
        for res in avg_results:
            print(f"  {res['name']:20s} : {res['avg_error']:.2f}% rata-rata error")
        print("=" * 50)

    print(f"\nTotal data point: {len(data_list)}")
    print("Analisis selesai!")


if __name__ == '__main__':
    main()
