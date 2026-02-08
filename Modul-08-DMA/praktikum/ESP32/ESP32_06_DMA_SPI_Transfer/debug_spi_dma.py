#!/usr/bin/env python3
"""
===========================================================================
 Debug Script: ESP32 SPI DMA Transfer — Throughput & Crossover Analysis
===========================================================================
 Parses SPI benchmark data from serial, creates throughput vs size plot,
 DMA crossover analysis, and efficiency charts.

 Usage:
   python debug_spi_dma.py [--port /dev/ttyUSB0] [--baud 115200]
   python debug_spi_dma.py --file output.log
===========================================================================
"""

import re
import sys
import argparse
import time

try:
    import serial
    HAS_SERIAL = True
except ImportError:
    HAS_SERIAL = False

try:
    import matplotlib.pyplot as plt
    import numpy as np
    HAS_PLOT = True
except ImportError:
    HAS_PLOT = False


def parse_spi_bench_line(line):
    """Parse [SPI_BENCH] line."""
    pattern = (
        r'\[SPI_BENCH\]\s+Mode=(\w+)\s+Size=(\d+)\s+DMA=(\w+)\s+'
        r'Time=(\d+)\s+us\s+Throughput=([\d.]+)\s+MB/s\s+'
        r'Efficiency=([\d.]+)%'
    )
    m = re.search(pattern, line)
    if m:
        return {
            'mode': m.group(1),
            'size': int(m.group(2)),
            'dma_mode': m.group(3),
            'time_us': int(m.group(4)),
            'throughput': float(m.group(5)),
            'efficiency': float(m.group(6)),
        }
    return None


def read_from_serial(port, baud, duration=120):
    """Read benchmark data from serial port."""
    if not HAS_SERIAL:
        print("ERROR: pyserial not installed")
        sys.exit(1)

    lines = []
    print(f"Reading from {port} at {baud} baud for up to {duration}s...")
    try:
        ser = serial.Serial(port, baud, timeout=1)
        start = time.time()
        while (time.time() - start) < duration:
            raw = ser.readline()
            if raw:
                line = raw.decode('utf-8', errors='replace').strip()
                print(line)
                lines.append(line)
                if 'Benchmark complete' in line:
                    break
        ser.close()
    except serial.SerialException as e:
        print(f"Serial error: {e}")
        sys.exit(1)
    return lines


def read_from_file(filepath):
    """Read from log file."""
    with open(filepath, 'r') as f:
        return [line.strip() for line in f.readlines()]


def create_charts(results):
    """Create SPI DMA analysis charts."""
    if not HAS_PLOT:
        print("matplotlib not installed, skipping charts")
        return

    # Separate TX-only and full-duplex results
    tx_only = [r for r in results if r['mode'] == 'TX_Only']
    full_duplex = [r for r in results if r['mode'] == 'FullDuplex']

    fig, axes = plt.subplots(2, 2, figsize=(14, 10))
    fig.suptitle('ESP32 SPI DMA Transfer — Analysis', fontsize=14)

    # --- Chart 1: Throughput vs Size ---
    ax = axes[0][0]
    if tx_only:
        sizes = [r['size'] for r in tx_only]
        throughputs = [r['throughput'] for r in tx_only]
        ax.plot(sizes, throughputs, 'o-', color='#e74c3c', linewidth=2,
                markersize=8, label='TX Only')
    if full_duplex:
        sizes = [r['size'] for r in full_duplex]
        throughputs = [r['throughput'] for r in full_duplex]
        ax.plot(sizes, throughputs, 's-', color='#3498db', linewidth=2,
                markersize=8, label='Full Duplex')

    # Theoretical max
    theoretical = 10 * 1000000 / (8 * 1024 * 1024)  # 10 MHz / 8 bits, in MB/s
    ax.axhline(y=theoretical, color='gray', linestyle='--', alpha=0.7,
               label=f'Theoretical Max ({theoretical:.2f} MB/s)')

    ax.set_xlabel('Transfer Size (bytes)')
    ax.set_ylabel('Throughput (MB/s)')
    ax.set_title('SPI DMA — Throughput vs Transfer Size')
    ax.set_xscale('log', base=2)
    ax.legend()
    ax.grid(True, alpha=0.3)

    # --- Chart 2: Efficiency ---
    ax2 = axes[0][1]
    if full_duplex:
        sizes = [r['size'] for r in full_duplex]
        effs = [r['efficiency'] for r in full_duplex]
        colors_bar = ['#e74c3c' if r['dma_mode'] == 'CPU_COPY' else '#2ecc71'
                      for r in full_duplex]
        ax2.bar(range(len(sizes)), effs, color=colors_bar, alpha=0.8)
        ax2.set_xticks(range(len(sizes)))
        ax2.set_xticklabels([str(s) for s in sizes])

        # Legend
        from matplotlib.patches import Patch
        legend_elements = [
            Patch(facecolor='#e74c3c', label='CPU Copy'),
            Patch(facecolor='#2ecc71', label='DMA'),
        ]
        ax2.legend(handles=legend_elements)

    ax2.set_xlabel('Transfer Size (bytes)')
    ax2.set_ylabel('Efficiency (%)')
    ax2.set_title('SPI Clock Efficiency')
    ax2.set_ylim(0, 110)
    ax2.axhline(y=100, color='gray', linestyle='--', alpha=0.5)
    ax2.grid(axis='y', alpha=0.3)

    # --- Chart 3: DMA Crossover (full-duplex by size) ---
    ax3 = axes[1][0]
    if full_duplex:
        # Sort by size for crossover view
        sorted_fd = sorted(full_duplex, key=lambda x: x['size'])
        sizes = [r['size'] for r in sorted_fd]
        throughputs = [r['throughput'] for r in sorted_fd]
        dma_modes = [r['dma_mode'] for r in sorted_fd]

        for i, (s, t, m) in enumerate(zip(sizes, throughputs, dma_modes)):
            color = '#e74c3c' if m == 'CPU_COPY' else '#2ecc71'
            ax3.bar(i, t, color=color, alpha=0.8)

        ax3.set_xticks(range(len(sizes)))
        ax3.set_xticklabels([str(s) for s in sizes], rotation=45)

        # Mark crossover
        ax3.axvline(x=2.5, color='orange', linestyle=':', linewidth=2,
                    label='~DMA Crossover')

        from matplotlib.patches import Patch
        legend_elements = [
            Patch(facecolor='#e74c3c', label='CPU Copy'),
            Patch(facecolor='#2ecc71', label='DMA'),
        ]
        ax3.legend(handles=legend_elements)

    ax3.set_xlabel('Transfer Size (bytes)')
    ax3.set_ylabel('Throughput (MB/s)')
    ax3.set_title('DMA Crossover Point Analysis')
    ax3.grid(axis='y', alpha=0.3)

    # --- Chart 4: Transfer time comparison ---
    ax4 = axes[1][1]
    if tx_only and full_duplex:
        sizes_tx = [r['size'] for r in sorted(tx_only, key=lambda x: x['size'])]
        times_tx = [r['time_us'] for r in sorted(tx_only, key=lambda x: x['size'])]
        sizes_fd = [r['size'] for r in sorted(full_duplex, key=lambda x: x['size'])]
        times_fd = [r['time_us'] for r in sorted(full_duplex, key=lambda x: x['size'])]

        ax4.plot(sizes_tx, times_tx, 'o-', color='#e74c3c', label='TX Only')
        ax4.plot(sizes_fd, times_fd, 's-', color='#3498db', label='Full Duplex')
    elif full_duplex:
        sizes_fd = [r['size'] for r in sorted(full_duplex, key=lambda x: x['size'])]
        times_fd = [r['time_us'] for r in sorted(full_duplex, key=lambda x: x['size'])]
        ax4.plot(sizes_fd, times_fd, 's-', color='#3498db', label='Full Duplex')

    ax4.set_xlabel('Transfer Size (bytes)')
    ax4.set_ylabel(f'Total Time (μs) for {200} iterations')
    ax4.set_title('Transfer Time vs Size')
    ax4.set_xscale('log', base=2)
    ax4.set_yscale('log')
    ax4.legend()
    ax4.grid(True, alpha=0.3)

    plt.tight_layout()
    plt.savefig('spi_dma_analysis.png', dpi=150)
    print("Chart saved: spi_dma_analysis.png")
    plt.show()


def main():
    parser = argparse.ArgumentParser(description='ESP32 SPI DMA Debug Tool')
    parser.add_argument('--port', default='/dev/ttyUSB0', help='Serial port')
    parser.add_argument('--baud', type=int, default=115200, help='Baud rate')
    parser.add_argument('--file', help='Read from log file')
    parser.add_argument('--duration', type=int, default=120, help='Read duration (s)')
    args = parser.parse_args()

    if args.file:
        lines = read_from_file(args.file)
    else:
        lines = read_from_serial(args.port, args.baud, args.duration)

    results = []
    for line in lines:
        r = parse_spi_bench_line(line)
        if r:
            results.append(r)

    if not results:
        print("No SPI benchmark results found!")
        sys.exit(1)

    print(f"\nParsed {len(results)} SPI benchmark entries.")
    print(f"  TX-Only: {len([r for r in results if r['mode'] == 'TX_Only'])}")
    print(f"  Full-Duplex: {len([r for r in results if r['mode'] == 'FullDuplex'])}")

    create_charts(results)


if __name__ == '__main__':
    main()
