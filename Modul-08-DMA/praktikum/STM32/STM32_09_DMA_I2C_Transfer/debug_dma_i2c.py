#!/usr/bin/env python3
"""
============================================================================
Debug Script - STM32_09_DMA_I2C_Transfer
============================================================================
Deskripsi : Parsing dan visualisasi data serial dari program DMA I2C
Fitur     : - Parse output [DATA] dari serial
            - Visualisasi perbandingan polling vs DMA
            - Grafik multi-ukuran transfer
            - Statistik error I2C
============================================================================
"""

import sys
import re
import argparse
from collections import defaultdict

# Cek dependensi opsional
try:
    import serial
    HAS_SERIAL = True
except ImportError:
    HAS_SERIAL = False
    print("[WARN] pyserial tidak terinstall. Install: pip install pyserial")

try:
    import matplotlib.pyplot as plt
    import matplotlib.ticker as ticker
    HAS_MATPLOTLIB = True
except ImportError:
    HAS_MATPLOTLIB = False
    print("[WARN] matplotlib tidak terinstall. Install: pip install matplotlib")


def parse_data_line(line):
    """Parse satu baris [DATA] dan kembalikan dict key=value."""
    match = re.match(r'\[DATA\]\s+(\w+),(.*)', line.strip())
    if not match:
        return None, {}
    tag = match.group(1)
    fields_str = match.group(2)
    fields = {}
    for pair in re.findall(r'(\w+)=([^,]+)', fields_str):
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
        print("ERROR: pyserial diperlukan untuk membaca serial port")
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
    """Baca data dari file log."""
    lines = []
    try:
        with open(filepath, 'r') as f:
            lines = [l.strip() for l in f.readlines() if l.strip()]
        print(f"Membaca {len(lines)} baris dari {filepath}")
    except FileNotFoundError:
        print(f"ERROR: File tidak ditemukan: {filepath}")
    return lines


def analyze_data(lines):
    """Analisis data yang sudah di-parse."""
    results = {
        'poll_tx': [],
        'dma_tx': [],
        'compare': [],
        'patterns': [],
        'multisize': [],
        'alignment': [],
        'summary': {},
        'status': [],
    }

    for line in lines:
        if '[DATA]' not in line:
            continue
        tag, fields = parse_data_line(line)
        if not tag:
            continue

        if tag == 'POLL_TX':
            results['poll_tx'].append(fields)
        elif tag == 'DMA_TX':
            results['dma_tx'].append(fields)
        elif tag == 'COMPARE':
            results['compare'].append(fields)
        elif tag == 'PATTERN':
            results['patterns'].append(fields)
        elif tag == 'MULTISIZE':
            results['multisize'].append(fields)
        elif tag == 'ALIGNMENT':
            results['alignment'].append(fields)
        elif tag == 'SUMMARY':
            results['summary'] = fields
        elif tag == 'STATUS':
            results['status'].append(fields)

    return results


def print_summary(results):
    """Cetak ringkasan analisis."""
    print("\n" + "=" * 60)
    print("  RINGKASAN ANALISIS DMA I2C TRANSFER")
    print("=" * 60)

    if results['summary']:
        s = results['summary']
        print(f"  Total tes       : {s.get('tests', 'N/A')}")
        print(f"  DMA berhasil    : {s.get('dma_ok', 'N/A')}")
        print(f"  Polling berhasil: {s.get('poll_ok', 'N/A')}")
        print(f"  DMA error       : {s.get('dma_err', 'N/A')}")
        print(f"  Polling error   : {s.get('poll_err', 'N/A')}")

    if results['multisize']:
        print("\n  Transfer Multi-Ukuran:")
        for ms in results['multisize']:
            print(f"    {ms.get('size', '?')} bytes: "
                  f"status={ms.get('status', '?')}, "
                  f"waktu={ms.get('time_ms', '?')} ms")

    print()


def plot_results(results):
    """Buat visualisasi grafik."""
    if not HAS_MATPLOTLIB:
        print("Matplotlib tidak tersedia, skip visualisasi.")
        return

    fig, axes = plt.subplots(2, 2, figsize=(14, 10))
    fig.suptitle('STM32 DMA I2C Transfer - Analisis', fontsize=14, fontweight='bold')

    # --- Grafik 1: Waktu transfer per ukuran (polling vs DMA) ---
    ax1 = axes[0, 0]
    if results['poll_tx'] and results['dma_tx']:
        poll_sizes = [d.get('size', 0) for d in results['poll_tx']]
        poll_times = [d.get('time_ms', 0) for d in results['poll_tx']]
        dma_sizes = [d.get('size', 0) for d in results['dma_tx']]
        dma_times = [d.get('time_ms', 0) for d in results['dma_tx']]

        ax1.bar([x - 2 for x in range(len(poll_sizes))], poll_times,
                width=4, label='Polling', color='#e74c3c', alpha=0.8)
        ax1.bar([x + 2 for x in range(len(dma_sizes))], dma_times,
                width=4, label='DMA', color='#2ecc71', alpha=0.8)
        ax1.set_xticks(range(len(poll_sizes)))
        ax1.set_xticklabels([str(s) for s in poll_sizes])
        ax1.set_xlabel('Ukuran (bytes)')
        ax1.set_ylabel('Waktu (ms)')
        ax1.set_title('Polling vs DMA: Waktu Transfer')
        ax1.legend()
    else:
        ax1.text(0.5, 0.5, 'Data tidak tersedia', ha='center', va='center')
        ax1.set_title('Polling vs DMA: Waktu Transfer')

    # --- Grafik 2: Multi-size DMA transfer ---
    ax2 = axes[0, 1]
    if results['multisize']:
        ms_sizes = [d.get('size', 0) for d in results['multisize']]
        ms_times = [d.get('time_ms', 0) for d in results['multisize']]
        colors = ['#3498db' if d.get('status', -1) == 0 else '#e74c3c'
                  for d in results['multisize']]
        ax2.bar(range(len(ms_sizes)), ms_times, color=colors, alpha=0.8)
        ax2.set_xticks(range(len(ms_sizes)))
        ax2.set_xticklabels([str(s) for s in ms_sizes], rotation=45)
        ax2.set_xlabel('Ukuran Transfer (bytes)')
        ax2.set_ylabel('Waktu (ms)')
        ax2.set_title('DMA I2C Multi-Ukuran')
    else:
        ax2.text(0.5, 0.5, 'Data tidak tersedia', ha='center', va='center')
        ax2.set_title('DMA I2C Multi-Ukuran')

    # --- Grafik 3: Status uptime ---
    ax3 = axes[1, 0]
    if results['status']:
        uptimes = [d.get('uptime', 0) for d in results['status']]
        states = [d.get('i2c_state', 0) for d in results['status']]
        ax3.plot(range(len(uptimes)), states, 'b-o', markersize=4)
        ax3.set_xlabel('Sample')
        ax3.set_ylabel('I2C State')
        ax3.set_title('Status I2C Seiring Waktu')
        ax3.grid(True, alpha=0.3)
    else:
        ax3.text(0.5, 0.5, 'Data tidak tersedia', ha='center', va='center')
        ax3.set_title('Status I2C Seiring Waktu')

    # --- Grafik 4: Ringkasan ---
    ax4 = axes[1, 1]
    if results['summary']:
        s = results['summary']
        labels = ['DMA OK', 'DMA Error', 'Poll OK', 'Poll Error']
        values = [s.get('dma_ok', 0), s.get('dma_err', 0),
                  s.get('poll_ok', 0), s.get('poll_err', 0)]
        colors = ['#2ecc71', '#e74c3c', '#3498db', '#e67e22']
        bars = ax4.bar(labels, values, color=colors, alpha=0.8)
        for bar, val in zip(bars, values):
            ax4.text(bar.get_x() + bar.get_width() / 2., bar.get_height() + 0.1,
                     str(val), ha='center', va='bottom', fontweight='bold')
        ax4.set_title('Ringkasan Transfer')
        ax4.set_ylabel('Jumlah')
    else:
        ax4.text(0.5, 0.5, 'Data tidak tersedia', ha='center', va='center')
        ax4.set_title('Ringkasan Transfer')

    plt.tight_layout()
    plt.savefig('dma_i2c_analysis.png', dpi=150)
    print("Grafik disimpan: dma_i2c_analysis.png")
    plt.show()


def main():
    parser = argparse.ArgumentParser(
        description='Debug Script STM32 DMA I2C Transfer')
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

    # Baca data
    if args.port:
        lines = read_from_serial(args.port, args.baud, args.timeout)
    elif args.file:
        lines = read_from_file(args.file)
    else:
        print("Gunakan --port atau --file untuk sumber data.")
        print("Contoh: python debug_dma_i2c.py --port /dev/ttyUSB0")
        print("        python debug_dma_i2c.py --file log.txt")
        # Demo dengan data simulasi
        print("\nMenjalankan mode demo dengan data simulasi...")
        lines = [
            "[DATA] POLL_TX,size=16,status=1,time_ms=12",
            "[DATA] DMA_TX,size=16,status=0,time_ms=8,error=0",
            "[DATA] POLL_TX,size=32,status=1,time_ms=15",
            "[DATA] DMA_TX,size=32,status=0,time_ms=9,error=0",
            "[DATA] POLL_TX,size=64,status=1,time_ms=22",
            "[DATA] DMA_TX,size=64,status=0,time_ms=10,error=0",
            "[DATA] MULTISIZE,size=1,status=0,time_ms=5",
            "[DATA] MULTISIZE,size=4,status=0,time_ms=6",
            "[DATA] MULTISIZE,size=16,status=0,time_ms=8",
            "[DATA] MULTISIZE,size=32,status=0,time_ms=9",
            "[DATA] MULTISIZE,size=64,status=0,time_ms=10",
            "[DATA] SUMMARY,tests=4,dma_ok=6,poll_ok=0,dma_err=3,poll_err=3",
        ]

    # Analisis
    results = analyze_data(lines)
    print_summary(results)

    # Visualisasi
    if not args.no_plot:
        plot_results(results)


if __name__ == '__main__':
    main()
