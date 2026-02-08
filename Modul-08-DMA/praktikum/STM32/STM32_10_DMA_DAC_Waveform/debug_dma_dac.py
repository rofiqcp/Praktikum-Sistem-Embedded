#!/usr/bin/env python3
"""
============================================================================
Debug Script - STM32_10_DMA_DAC_Waveform
============================================================================
Deskripsi : Parsing dan visualisasi data gelombang dari program DMA PWM
Fitur     : - Parse tabel gelombang [DATA] WAVE
            - Visualisasi 4 tipe gelombang (sinus, segitiga, gergaji, kotak)
            - Monitor status DMA real-time
            - Analisis kualitas gelombang
============================================================================
"""

import sys
import re
import argparse

try:
    import serial
    HAS_SERIAL = True
except ImportError:
    HAS_SERIAL = False
    print("[WARN] pyserial tidak terinstall. Install: pip install pyserial")

try:
    import matplotlib.pyplot as plt
    import numpy as np
    HAS_MATPLOTLIB = True
except ImportError:
    HAS_MATPLOTLIB = False
    print("[WARN] matplotlib tidak terinstall. Install: pip install matplotlib")


def parse_data_line(line):
    """Parse baris [DATA] menjadi (tag, dict)."""
    match = re.match(r'\[DATA\]\s+(\w+),(.*)', line.strip())
    if not match:
        return None, {}
    tag = match.group(1)
    fields = {}
    for pair in re.findall(r'(\w+)=([^,]+)', match.group(2)):
        key, val = pair
        try:
            if '.' in val:
                fields[key] = float(val)
            elif val.startswith('0x'):
                fields[key] = int(val, 16)
            else:
                fields[key] = int(val)
        except ValueError:
            fields[key] = val
    return tag, fields


def read_from_serial(port, baudrate=115200, timeout=60):
    """Baca data dari serial port."""
    if not HAS_SERIAL:
        print("ERROR: pyserial diperlukan")
        sys.exit(1)
    lines = []
    print(f"Membaca dari {port} @ {baudrate} baud (timeout={timeout}s)...")
    try:
        ser = serial.Serial(port, baudrate, timeout=1)
        import time
        start = time.time()
        while (time.time() - start) < timeout:
            raw = ser.readline()
            if raw:
                line = raw.decode('utf-8', errors='replace').strip()
                if line:
                    print(f"  > {line}")
                    lines.append(line)
        ser.close()
    except serial.SerialException as e:
        print(f"ERROR serial: {e}")
    return lines


def read_from_file(filepath):
    """Baca dari file log."""
    try:
        with open(filepath, 'r') as f:
            lines = [l.strip() for l in f if l.strip()]
        print(f"Membaca {len(lines)} baris dari {filepath}")
        return lines
    except FileNotFoundError:
        print(f"ERROR: File tidak ditemukan: {filepath}")
        return []


def analyze_waveforms(lines):
    """Ekstrak data tabel gelombang dan status."""
    wave_data = {'sine': [], 'tri': [], 'saw': [], 'sqr': []}
    status_list = []
    config = {}

    for line in lines:
        if '[DATA]' not in line:
            continue
        tag, fields = parse_data_line(line)
        if not tag:
            continue

        if tag == 'WAVE':
            idx = fields.get('idx', 0)
            wave_data['sine'].append((idx, fields.get('sine', 0)))
            wave_data['tri'].append((idx, fields.get('tri', 0)))
            wave_data['saw'].append((idx, fields.get('saw', 0)))
            wave_data['sqr'].append((idx, fields.get('sqr', 0)))
        elif tag == 'STATUS':
            status_list.append(fields)
        elif tag == 'CONFIG':
            config = fields

    return wave_data, status_list, config


def print_analysis(wave_data, status_list, config):
    """Cetak analisis ke konsol."""
    print("\n" + "=" * 60)
    print("  ANALISIS DMA DAC WAVEFORM")
    print("=" * 60)

    if config:
        print(f"  PWM Frequency  : {config.get('pwm_freq', 'N/A')} Hz")
        print(f"  Wave Frequency : {config.get('wave_freq', 'N/A')} Hz")
        print(f"  Samples/cycle  : {config.get('samples', 'N/A')}")
        print(f"  Resolution     : {config.get('resolution', 'N/A')} levels")

    num_samples = len(wave_data['sine'])
    print(f"\n  Tabel gelombang: {num_samples} sampel")

    if num_samples > 0:
        for name in ['sine', 'tri', 'saw', 'sqr']:
            values = [v for _, v in wave_data[name]]
            if values:
                mn, mx, avg = min(values), max(values), sum(values) / len(values)
                print(f"  {name:6s}: min={mn}, max={mx}, avg={avg:.1f}")

    if status_list:
        waveforms_seen = set()
        for s in status_list:
            w = s.get('wave', 'N/A')
            if isinstance(w, str):
                waveforms_seen.add(w)
        print(f"\n  Gelombang terdeteksi: {', '.join(waveforms_seen)}")
        print(f"  Status sampel: {len(status_list)}")

    print()


def plot_waveforms(wave_data, config):
    """Visualisasi 4 tipe gelombang."""
    if not HAS_MATPLOTLIB:
        print("Matplotlib tidak tersedia, skip visualisasi.")
        return

    fig, axes = plt.subplots(2, 2, figsize=(14, 10))
    fig.suptitle('STM32 DMA DAC Waveform - Tabel Gelombang', fontsize=14,
                 fontweight='bold')

    wave_names = {
        'sine': ('Sinus', '#e74c3c'),
        'tri': ('Segitiga', '#2ecc71'),
        'saw': ('Gergaji (Sawtooth)', '#3498db'),
        'sqr': ('Kotak Halus', '#9b59b6'),
    }

    resolution = config.get('resolution', 1000)

    for idx, (key, (title, color)) in enumerate(wave_names.items()):
        ax = axes[idx // 2, idx % 2]
        data = wave_data[key]
        if data:
            x_vals = [d[0] for d in data]
            y_vals = [d[1] for d in data]
            ax.plot(x_vals, y_vals, color=color, linewidth=2, alpha=0.9)
            ax.fill_between(x_vals, y_vals, alpha=0.15, color=color)
            ax.set_ylim(-50, resolution + 50)
            ax.axhline(y=resolution / 2, color='gray', linestyle='--', alpha=0.3)
        ax.set_title(f'Gelombang {title}')
        ax.set_xlabel('Indeks Sampel')
        ax.set_ylabel('Duty Cycle (0-999)')
        ax.grid(True, alpha=0.3)

    plt.tight_layout()
    plt.savefig('dma_dac_waveforms.png', dpi=150)
    print("Grafik disimpan: dma_dac_waveforms.png")
    plt.show()


def plot_status(status_list):
    """Visualisasi status DMA selama operasi."""
    if not HAS_MATPLOTLIB or not status_list:
        return

    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(12, 8))
    fig.suptitle('DMA DAC Waveform - Status Runtime', fontsize=14, fontweight='bold')

    cycles = [s.get('cycles', 0) for s in status_list]
    errors = [s.get('errors', 0) for s in status_list]
    uptimes = [s.get('uptime', 0) for s in status_list]

    ax1.plot(range(len(cycles)), cycles, 'b-', linewidth=1.5, label='DMA Cycles')
    ax1.set_xlabel('Sample')
    ax1.set_ylabel('DMA Cycles Completed')
    ax1.set_title('DMA Transfer Cycles')
    ax1.legend()
    ax1.grid(True, alpha=0.3)

    ax2.plot(range(len(errors)), errors, 'r-o', markersize=3, label='Errors')
    ax2.set_xlabel('Sample')
    ax2.set_ylabel('Error Count')
    ax2.set_title('Error DMA Seiring Waktu')
    ax2.legend()
    ax2.grid(True, alpha=0.3)

    plt.tight_layout()
    plt.savefig('dma_dac_status.png', dpi=150)
    print("Grafik status disimpan: dma_dac_status.png")
    plt.show()


def generate_demo_data():
    """Buat data simulasi untuk demo."""
    import math
    lines = []
    lines.append("[DATA] CONFIG,pwm_freq=1000,wave_freq=15,samples=64,resolution=1000")
    for i in range(64):
        phase = i / 64.0
        sine = int((math.sin(2 * math.pi * phase) + 1.0) * 0.5 * 999)
        tri = int(phase * 2 * 999) if phase < 0.5 else int((1.0 - phase) * 2 * 999)
        saw = int(phase * 999)
        sqr = 999 if phase < 0.5 else 0
        lines.append(f"[DATA] WAVE,idx={i},sine={sine},tri={tri},saw={saw},sqr={sqr}")
    for i in range(20):
        lines.append(f"[DATA] STATUS,wave=SINUS,cycles={i * 100},half={i * 200},"
                     f"errors=0,uptime={i}")
    return lines


def main():
    parser = argparse.ArgumentParser(
        description='Debug Script STM32 DMA DAC Waveform')
    parser.add_argument('--port', '-p', type=str, default=None,
                        help='Serial port (misal: /dev/ttyUSB0)')
    parser.add_argument('--file', '-f', type=str, default=None,
                        help='File log input')
    parser.add_argument('--baud', '-b', type=int, default=115200,
                        help='Baudrate (default: 115200)')
    parser.add_argument('--timeout', '-t', type=int, default=60,
                        help='Timeout serial (detik)')
    parser.add_argument('--no-plot', action='store_true',
                        help='Skip visualisasi grafik')
    args = parser.parse_args()

    if args.port:
        lines = read_from_serial(args.port, args.baud, args.timeout)
    elif args.file:
        lines = read_from_file(args.file)
    else:
        print("Gunakan --port atau --file. Menjalankan mode demo...")
        lines = generate_demo_data()

    wave_data, status_list, config = analyze_waveforms(lines)
    print_analysis(wave_data, status_list, config)

    if not args.no_plot:
        plot_waveforms(wave_data, config)
        plot_status(status_list)


if __name__ == '__main__':
    main()
