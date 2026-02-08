#!/usr/bin/env python3
"""
============================================================================
Debug Script - STM32_11_DMA_Benchmark
============================================================================
Deskripsi : Parsing dan visualisasi hasil benchmark DMA vs CPU
Fitur     : - Parse benchmark ukuran, alignment, crossover
            - Bar chart perbandingan throughput
            - Line plot cycles per byte
            - Identifikasi titik crossover DMA vs CPU
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


def read_from_serial(port, baudrate=115200, timeout=120):
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


def analyze_benchmark(lines):
    """Ekstrak semua data benchmark."""
    results = {
        'bench': [],       # Benchmark ukuran
        'align': [],       # Benchmark alignment
        'crossover': [],   # Data crossover
        'crossover_result': None,
        'summary': {},
    }

    for line in lines:
        if '[DATA]' not in line:
            continue
        tag, fields = parse_data_line(line)
        if not tag:
            continue

        if tag == 'BENCH':
            results['bench'].append(fields)
        elif tag == 'ALIGN':
            results['align'].append(fields)
        elif tag == 'CROSSOVER':
            results['crossover'].append(fields)
        elif tag == 'CROSSOVER_RESULT':
            results['crossover_result'] = fields.get('size', 0)
        elif tag == 'SUMMARY':
            results['summary'] = fields

    return results


def print_analysis(results):
    """Cetak ringkasan benchmark ke konsol."""
    print("\n" + "=" * 70)
    print("  HASIL BENCHMARK DMA vs CPU - STM32F103C8 @ 72MHz")
    print("=" * 70)

    if results['bench']:
        print("\n  Benchmark Ukuran (Word alignment, 100 iterasi):")
        print(f"  {'Size':>6s}  {'CPU cyc':>10s}  {'DMA cyc':>10s}  "
              f"{'CPU MB/s':>10s}  {'DMA MB/s':>10s}  {'Speedup':>8s}")
        print("  " + "-" * 62)
        for b in results['bench']:
            print(f"  {b.get('size', 0):>6}  {b.get('cpu_cycles', 0):>10}  "
                  f"{b.get('dma_cycles', 0):>10}  {b.get('cpu_mbs', 0):>10.2f}  "
                  f"{b.get('dma_mbs', 0):>10.2f}  {b.get('speedup', 0):>8.3f}")

    if results['align']:
        print("\n  Benchmark Alignment (256 bytes):")
        for a in results['align']:
            print(f"    {a.get('mode', '?'):>10s}: {a.get('cycles', 0)} siklus, "
                  f"{a.get('throughput', 0):.2f} MB/s, {a.get('cpb', 0):.2f} cyc/byte")

    if results['crossover_result'] is not None:
        cr = results['crossover_result']
        if cr > 0:
            print(f"\n  TITIK CROSSOVER: ~{cr} bytes")
            print(f"  DMA lebih cepat dari CPU untuk transfer >= {cr} bytes")
        else:
            print("\n  TITIK CROSSOVER: Tidak ditemukan dalam rentang uji")

    if results['summary']:
        s = results['summary']
        print(f"\n  Rata-rata CPU: {s.get('avg_cpu_mbs', 0):.2f} MB/s")
        print(f"  Rata-rata DMA: {s.get('avg_dma_mbs', 0):.2f} MB/s")

    print()


def plot_benchmark(results):
    """Visualisasi hasil benchmark."""
    if not HAS_MATPLOTLIB:
        print("Matplotlib tidak tersedia, skip visualisasi.")
        return

    fig, axes = plt.subplots(2, 2, figsize=(15, 11))
    fig.suptitle('STM32 DMA Benchmark - DMA vs CPU @ 72MHz', fontsize=14,
                 fontweight='bold')

    # --- Grafik 1: Throughput per ukuran ---
    ax1 = axes[0, 0]
    if results['bench']:
        sizes = [b.get('size', 0) for b in results['bench']]
        cpu_mbs = [b.get('cpu_mbs', 0) for b in results['bench']]
        dma_mbs = [b.get('dma_mbs', 0) for b in results['bench']]

        x = np.arange(len(sizes))
        width = 0.35
        ax1.bar(x - width / 2, cpu_mbs, width, label='CPU (memcpy)',
                color='#e74c3c', alpha=0.85)
        ax1.bar(x + width / 2, dma_mbs, width, label='DMA M2M',
                color='#2ecc71', alpha=0.85)
        ax1.set_xticks(x)
        ax1.set_xticklabels([str(s) for s in sizes])
        ax1.set_xlabel('Ukuran Transfer (bytes)')
        ax1.set_ylabel('Throughput (MB/s)')
        ax1.set_title('Throughput: CPU vs DMA')
        ax1.legend()
        ax1.grid(axis='y', alpha=0.3)

    # --- Grafik 2: Cycles per byte ---
    ax2 = axes[0, 1]
    if results['bench']:
        sizes = [b.get('size', 0) for b in results['bench']]
        cpu_cpb = [b.get('cpu_cpb', 0) for b in results['bench']]
        dma_cpb = [b.get('dma_cpb', 0) for b in results['bench']]

        ax2.plot(sizes, cpu_cpb, 'r-o', linewidth=2, markersize=6,
                 label='CPU (memcpy)')
        ax2.plot(sizes, dma_cpb, 'g-s', linewidth=2, markersize=6,
                 label='DMA M2M')
        ax2.set_xlabel('Ukuran Transfer (bytes)')
        ax2.set_ylabel('Cycles per Byte')
        ax2.set_title('Efisiensi: Cycles per Byte')
        ax2.legend()
        ax2.grid(True, alpha=0.3)
        ax2.set_xscale('log', base=2)

    # --- Grafik 3: Crossover analysis ---
    ax3 = axes[1, 0]
    if results['crossover']:
        co_sizes = [c.get('size', 0) for c in results['crossover']]
        co_cpu = [c.get('cpu', 0) for c in results['crossover']]
        co_dma = [c.get('dma', 0) for c in results['crossover']]

        ax3.plot(co_sizes, co_cpu, 'r-o', linewidth=2, markersize=5,
                 label='CPU (memcpy)')
        ax3.plot(co_sizes, co_dma, 'g-s', linewidth=2, markersize=5,
                 label='DMA M2M')
        ax3.set_xlabel('Ukuran Transfer (bytes)')
        ax3.set_ylabel('CPU Cycles')
        ax3.set_title('Pencarian Titik Crossover')
        ax3.legend()
        ax3.grid(True, alpha=0.3)

        if results['crossover_result'] and results['crossover_result'] > 0:
            ax3.axvline(x=results['crossover_result'], color='orange',
                        linestyle='--', linewidth=2, alpha=0.7,
                        label=f'Crossover ~{results["crossover_result"]}B')
            ax3.legend()

    # --- Grafik 4: Alignment comparison ---
    ax4 = axes[1, 1]
    if results['align']:
        modes = [a.get('mode', '?') for a in results['align']]
        throughputs = [a.get('throughput', 0) for a in results['align']]
        cycles = [a.get('cycles', 0) for a in results['align']]

        colors = ['#e74c3c', '#f39c12', '#2ecc71']
        bars = ax4.bar(modes, throughputs, color=colors[:len(modes)], alpha=0.85)
        for bar, tp in zip(bars, throughputs):
            ax4.text(bar.get_x() + bar.get_width() / 2., bar.get_height() + 0.1,
                     f'{tp:.1f}', ha='center', va='bottom', fontweight='bold')
        ax4.set_xlabel('Data Width')
        ax4.set_ylabel('Throughput (MB/s)')
        ax4.set_title('Perbandingan Alignment DMA (256 bytes)')
        ax4.grid(axis='y', alpha=0.3)

    plt.tight_layout()
    plt.savefig('dma_benchmark.png', dpi=150)
    print("Grafik disimpan: dma_benchmark.png")
    plt.show()


def generate_demo_data():
    """Data simulasi untuk demo tanpa hardware."""
    lines = []
    sizes = [32, 64, 128, 256, 512, 1024]
    for s in sizes:
        cpu_c = int(s * 2.5 + 30)
        dma_c = int(s * 1.0 + 180)
        cpu_mbs = s * 72.0 / cpu_c
        dma_mbs = s * 72.0 / dma_c
        speedup = cpu_c / dma_c
        cpu_cpb = cpu_c / s
        dma_cpb = dma_c / s
        lines.append(f"[DATA] BENCH,size={s},cpu_cycles={cpu_c},dma_cycles={dma_c},"
                     f"cpu_mbs={cpu_mbs:.2f},dma_mbs={dma_mbs:.2f},"
                     f"speedup={speedup:.3f},cpu_cpb={cpu_cpb:.2f},"
                     f"dma_cpb={dma_cpb:.2f},cpu_ok=1,dma_ok=1")

    for mode, cyc in [('byte', 900), ('halfword', 550), ('word', 350)]:
        tp = 256 * 72.0 / cyc
        cpb = cyc / 256.0
        lines.append(f"[DATA] ALIGN,mode={mode},cycles={cyc},"
                     f"throughput={tp:.2f},cpb={cpb:.2f},ok=1")

    cross_sizes = [4, 8, 16, 32, 64, 128, 256, 512]
    for s in cross_sizes:
        cpu_c = int(s * 2.5 + 30)
        dma_c = int(s * 1.0 + 180)
        winner = "DMA" if dma_c < cpu_c else "CPU"
        lines.append(f"[DATA] CROSSOVER,size={s},cpu={cpu_c},dma={dma_c},"
                     f"winner={winner}")

    lines.append("[DATA] CROSSOVER_RESULT,size=128")
    lines.append("[DATA] SUMMARY,avg_cpu_mbs=25.50,avg_dma_mbs=35.80,"
                 "byte_cyc=900,hword_cyc=550,word_cyc=350")
    return lines


def main():
    parser = argparse.ArgumentParser(
        description='Debug Script STM32 DMA Benchmark')
    parser.add_argument('--port', '-p', type=str, default=None,
                        help='Serial port (misal: /dev/ttyUSB0)')
    parser.add_argument('--file', '-f', type=str, default=None,
                        help='File log input')
    parser.add_argument('--baud', '-b', type=int, default=115200,
                        help='Baudrate (default: 115200)')
    parser.add_argument('--timeout', '-t', type=int, default=120,
                        help='Timeout serial (detik, default: 120)')
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

    results = analyze_benchmark(lines)
    print_analysis(results)

    if not args.no_plot:
        plot_benchmark(results)


if __name__ == '__main__':
    main()
