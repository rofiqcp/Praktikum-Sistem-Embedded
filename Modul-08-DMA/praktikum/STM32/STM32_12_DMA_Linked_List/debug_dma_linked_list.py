#!/usr/bin/env python3
"""
============================================================================
Debug Script - STM32_12_DMA_Linked_List
============================================================================
Deskripsi : Parsing dan visualisasi data linked-list DMA scatter-gather
Fitur     : - Parse deskriptor chain transfer
            - Visualisasi timing per deskriptor
            - Memory map buffer sumber dan tujuan
            - Statistik multi-chain dan scatter
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
    import matplotlib.patches as mpatches
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


def read_from_serial(port, baudrate=115200, timeout=90):
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


def analyze_linked_list(lines):
    """Ekstrak semua data linked-list DMA."""
    results = {
        'descriptors': [],
        'chain': [],
        'timing': [],
        'timing_summary': {},
        'multi_chain': [],
        'scatter': [],
        'verify': [],
        'summary': {},
    }

    for line in lines:
        if '[DATA]' not in line:
            continue
        tag, fields = parse_data_line(line)
        if not tag:
            continue

        if tag == 'DESC':
            results['descriptors'].append(fields)
        elif tag == 'CHAIN':
            results['chain'].append(fields)
        elif tag == 'CHAIN_ERROR':
            results['chain'].append({**fields, 'error': True})
        elif tag == 'TIMING':
            results['timing'].append(fields)
        elif tag == 'TIMING_SUMMARY':
            results['timing_summary'] = fields
        elif tag == 'MULTI_CHAIN':
            results['multi_chain'].append(fields)
        elif tag == 'SCATTER':
            results['scatter'].append(fields)
        elif tag == 'VERIFY':
            results['verify'].append(fields)
        elif tag == 'SUMMARY':
            results['summary'] = fields

    return results


def print_analysis(results):
    """Cetak ringkasan analisis."""
    print("\n" + "=" * 60)
    print("  ANALISIS DMA LINKED LIST (Scatter-Gather Software)")
    print("=" * 60)

    if results['descriptors']:
        print(f"\n  Deskriptor yang terdeteksi: {len(results['descriptors'])}")
        for d in results['descriptors']:
            status = "Selesai" if d.get('done', 0) else "Pending"
            print(f"    Desc[{d.get('id', '?')}]: src=0x{d.get('src', 0):08X} "
                  f"dst=0x{d.get('dst', 0):08X} "
                  f"size={d.get('size', 0)} [{status}]")

    if results['chain']:
        print(f"\n  Chain Transfer:")
        for c in results['chain']:
            if c.get('error'):
                print(f"    ERROR pada deskriptor {c.get('desc', '?')}")
            else:
                print(f"    {c.get('descs', '?')} descs, {c.get('bytes', 0)} bytes, "
                      f"{c.get('cycles', 0)} cycles ({c.get('us', 0):.2f} us), "
                      f"{c.get('mbs', 0):.2f} MB/s")

    if results['timing_summary']:
        ts = results['timing_summary']
        print(f"\n  Timing ({len(results['timing'])} iterasi):")
        print(f"    Minimum : {ts.get('min', 'N/A')} cycles")
        print(f"    Maksimum: {ts.get('max', 'N/A')} cycles")
        print(f"    Rata-rata: {ts.get('avg', 'N/A')} cycles ({ts.get('avg_us', 0):.2f} us)")
        print(f"    Jitter   : {ts.get('jitter', 'N/A')} cycles")

    if results['multi_chain']:
        print(f"\n  Multi-Chain:")
        for mc in results['multi_chain']:
            print(f"    Chain {mc.get('chain', '?')}: pola=0x{mc.get('pattern', 0):02X}, "
                  f"{mc.get('cycles', 0)} cycles, OK={mc.get('ok', 0)}")

    if results['summary']:
        s = results['summary']
        print(f"\n  Ringkasan Akhir:")
        print(f"    Total tes   : {s.get('tests', 0)}")
        print(f"    Chain sukses: {s.get('chains', 0)}")
        print(f"    Total bytes : {s.get('bytes', 0)}")
        print(f"    Error DMA   : {s.get('errors', 0)}")

    print()


def plot_results(results):
    """Visualisasi hasil linked-list DMA."""
    if not HAS_MATPLOTLIB:
        print("Matplotlib tidak tersedia, skip visualisasi.")
        return

    fig, axes = plt.subplots(2, 2, figsize=(14, 10))
    fig.suptitle('STM32 DMA Linked List - Software Scatter-Gather',
                 fontsize=14, fontweight='bold')

    # --- Grafik 1: Ukuran deskriptor ---
    ax1 = axes[0, 0]
    if results['descriptors']:
        # Ambil set unik deskriptor (hanya yang pertama kali muncul)
        seen_ids = set()
        unique_descs = []
        for d in results['descriptors']:
            did = d.get('id', 0)
            if did not in seen_ids:
                seen_ids.add(did)
                unique_descs.append(d)

        ids = [f"Desc {d.get('id', '?')}" for d in unique_descs]
        sizes = [d.get('size', 0) for d in unique_descs]
        colors = ['#e74c3c', '#2ecc71', '#3498db', '#f39c12', '#9b59b6']

        bars = ax1.bar(ids, sizes, color=colors[:len(ids)], alpha=0.85)
        for bar, sz in zip(bars, sizes):
            ax1.text(bar.get_x() + bar.get_width() / 2., bar.get_height() + 0.5,
                     f'{sz}B', ha='center', va='bottom', fontweight='bold')
        ax1.set_ylabel('Ukuran (bytes)')
        ax1.set_title('Ukuran per Deskriptor')
        ax1.grid(axis='y', alpha=0.3)
    else:
        ax1.text(0.5, 0.5, 'Data tidak tersedia', ha='center', va='center')

    # --- Grafik 2: Timing iterasi ---
    ax2 = axes[0, 1]
    if results['timing']:
        iters = [t.get('iter', 0) for t in results['timing']]
        cycles = [t.get('cycles', 0) for t in results['timing']]

        ax2.plot(iters, cycles, 'b-o', markersize=5, linewidth=1.5)
        if results['timing_summary']:
            avg = results['timing_summary'].get('avg', 0)
            ax2.axhline(y=avg, color='r', linestyle='--', alpha=0.7,
                        label=f'Avg: {avg} cycles')
            ax2.legend()
        ax2.set_xlabel('Iterasi')
        ax2.set_ylabel('CPU Cycles')
        ax2.set_title('Timing Chain Transfer per Iterasi')
        ax2.grid(True, alpha=0.3)
    else:
        ax2.text(0.5, 0.5, 'Data tidak tersedia', ha='center', va='center')

    # --- Grafik 3: Multi-chain comparison ---
    ax3 = axes[1, 0]
    if results['multi_chain']:
        chain_ids = [f"Chain {mc.get('chain', '?')}" for mc in results['multi_chain']]
        chain_cycles = [mc.get('cycles', 0) for mc in results['multi_chain']]
        chain_ok = [mc.get('ok', 0) for mc in results['multi_chain']]
        colors = ['#2ecc71' if ok else '#e74c3c' for ok in chain_ok]

        bars = ax3.bar(chain_ids, chain_cycles, color=colors, alpha=0.85)
        for bar, cy, ok in zip(bars, chain_cycles, chain_ok):
            label = f'{cy}\n{"OK" if ok else "FAIL"}'
            ax3.text(bar.get_x() + bar.get_width() / 2., bar.get_height() + 5,
                     label, ha='center', va='bottom', fontsize=9)
        ax3.set_ylabel('CPU Cycles')
        ax3.set_title('Perbandingan Multi-Chain')
        ax3.grid(axis='y', alpha=0.3)
    else:
        ax3.text(0.5, 0.5, 'Data tidak tersedia', ha='center', va='center')

    # --- Grafik 4: Memory map visualisasi ---
    ax4 = axes[1, 1]
    if results['descriptors']:
        seen_ids = set()
        unique_descs = []
        for d in results['descriptors']:
            did = d.get('id', 0)
            if did not in seen_ids:
                seen_ids.add(did)
                unique_descs.append(d)

        colors = ['#e74c3c', '#2ecc71', '#3498db', '#f39c12', '#9b59b6']
        y_pos = 0
        patches = []
        for i, d in enumerate(unique_descs):
            sz = d.get('size', 0)
            c = colors[i % len(colors)]
            rect = plt.Rectangle((0, y_pos), sz, 0.8, color=c, alpha=0.7)
            ax4.add_patch(rect)
            ax4.text(sz / 2, y_pos + 0.4, f"Desc {d.get('id', '?')}: {sz}B",
                     ha='center', va='center', fontweight='bold', fontsize=9)
            patches.append(mpatches.Patch(color=c, label=f"Desc {d.get('id', '?')}"))
            y_pos += 1

        total_size = sum(d.get('size', 0) for d in unique_descs)
        ax4.set_xlim(0, max(80, total_size))
        ax4.set_ylim(-0.5, y_pos + 0.5)
        ax4.set_xlabel('Ukuran (bytes)')
        ax4.set_title('Layout Buffer Destinasi')
        ax4.legend(handles=patches, loc='upper right', fontsize=8)
        ax4.set_yticks([])
    else:
        ax4.text(0.5, 0.5, 'Data tidak tersedia', ha='center', va='center')

    plt.tight_layout()
    plt.savefig('dma_linked_list.png', dpi=150)
    print("Grafik disimpan: dma_linked_list.png")
    plt.show()


def generate_demo_data():
    """Data simulasi untuk demo."""
    lines = [
        "[DATA] DESC,id=0,src=0x20000100,dst=0x20000400,size=32,done=1",
        "[DATA] DESC,id=1,src=0x20000200,dst=0x20000420,size=48,done=1",
        "[DATA] DESC,id=2,src=0x20000300,dst=0x20000450,size=24,done=1",
        "[DATA] DESC,id=3,src=0x20000350,dst=0x20000468,size=64,done=1",
        "[DATA] CHAIN,descs=4,bytes=168,cycles=1250,us=17.36,mbs=9.68",
        "[DATA] VERIFY,result=PASS,descs=4",
    ]
    for i in range(1, 6):
        cyc = 1200 + (i * 15) % 100
        lines.append(f"[DATA] TIMING,iter={i},cycles={cyc}")

    lines.append("[DATA] TIMING_SUMMARY,min=1200,max=1320,avg=1260,avg_us=17.50,jitter=120")

    for c in range(1, 4):
        pat = 0x10 + (c - 1) * 0x40
        cyc = 1250 + c * 20
        lines.append(f"[DATA] MULTI_CHAIN,chain={c},pattern=0x{pat:02X},cycles={cyc},ok=1")

    lines.append("[DATA] SCATTER,cycles=1180,descs=4")
    lines.append("[DATA] SUMMARY,tests=5,chains=8,bytes=1344,errors=0")
    return lines


def main():
    parser = argparse.ArgumentParser(
        description='Debug Script STM32 DMA Linked List')
    parser.add_argument('--port', '-p', type=str, default=None,
                        help='Serial port (misal: /dev/ttyUSB0)')
    parser.add_argument('--file', '-f', type=str, default=None,
                        help='File log input')
    parser.add_argument('--baud', '-b', type=int, default=115200,
                        help='Baudrate (default: 115200)')
    parser.add_argument('--timeout', '-t', type=int, default=90,
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

    results = analyze_linked_list(lines)
    print_analysis(results)

    if not args.no_plot:
        plot_results(results)


if __name__ == '__main__':
    main()
