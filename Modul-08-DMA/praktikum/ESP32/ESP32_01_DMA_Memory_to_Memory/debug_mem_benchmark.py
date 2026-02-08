#!/usr/bin/env python3
"""
===========================================================================
 Debug Script: ESP32 Memory Copy Benchmark — Parser & Visualization
===========================================================================
 Parses serial output from ESP32_01_DMA_Memory_to_Memory,
 creates comparison bar charts per method and buffer size.

 Usage:
   python debug_mem_benchmark.py [--port /dev/ttyUSB0] [--baud 115200]
   python debug_mem_benchmark.py --file output.log
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


def parse_bench_line(line):
    """Parse a [BENCH] line and return dict with method, size, time, throughput."""
    pattern = (
        r'\[BENCH\]\s+Method=(\w+)\s+Size=(\d+)\s+'
        r'(?:Time=(\d+)\s+us\s+Throughput=([\d.]+)\s+MB/s|SKIPPED.*)'
    )
    m = re.search(pattern, line)
    if m:
        method = m.group(1)
        size = int(m.group(2))
        if m.group(3):
            time_us = int(m.group(3))
            throughput = float(m.group(4))
            return {'method': method, 'size': size, 'time_us': time_us,
                    'throughput': throughput, 'skipped': False}
        else:
            return {'method': method, 'size': size, 'time_us': 0,
                    'throughput': 0.0, 'skipped': True}
    return None


def read_from_serial(port, baud, duration=60):
    """Read benchmark data from serial port."""
    if not HAS_SERIAL:
        print("ERROR: pyserial not installed. Run: pip install pyserial")
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
    """Read benchmark data from log file."""
    with open(filepath, 'r') as f:
        return [line.strip() for line in f.readlines()]


def parse_all(lines):
    """Parse all benchmark results from lines."""
    results = []
    for line in lines:
        r = parse_bench_line(line)
        if r:
            results.append(r)
    return results


def create_charts(results):
    """Create comparison bar charts."""
    if not HAS_PLOT:
        print("WARNING: matplotlib not installed. Skipping charts.")
        print("Install with: pip install matplotlib numpy")
        return

    # Method name mapping
    method_map = {
        'ForLoop': 'For-Loop',
        'Memcpy': 'memcpy (Default)',
        'IRAM': 'memcpy (IRAM)',
        'SPIRAM': 'memcpy (SPIRAM)',
        'SPI_DMA': 'SPI DMA Loopback',
    }

    # Organize data
    methods = []
    sizes = sorted(set(r['size'] for r in results))
    data = {}

    for r in results:
        mname = method_map.get(r['method'], r['method'])
        if mname not in data:
            data[mname] = {}
            methods.append(mname)
        data[mname][r['size']] = r['throughput'] if not r['skipped'] else 0

    if not data:
        print("No benchmark data found!")
        return

    # --- Chart 1: Grouped bar chart (throughput by buffer size) ---
    fig, axes = plt.subplots(1, 2, figsize=(16, 6))

    x = np.arange(len(sizes))
    width = 0.15
    colors = ['#e74c3c', '#3498db', '#2ecc71', '#9b59b6', '#f39c12']

    ax = axes[0]
    for i, method in enumerate(methods):
        vals = [data[method].get(s, 0) for s in sizes]
        ax.bar(x + i * width, vals, width, label=method, color=colors[i % len(colors)])

    ax.set_xlabel('Buffer Size (bytes)')
    ax.set_ylabel('Throughput (MB/s)')
    ax.set_title('ESP32 Memory Copy Benchmark — Throughput Comparison')
    ax.set_xticks(x + width * (len(methods) - 1) / 2)
    ax.set_xticklabels([f'{s}' for s in sizes])
    ax.legend(fontsize=8)
    ax.grid(axis='y', alpha=0.3)

    # --- Chart 2: Line chart (throughput vs size per method) ---
    ax2 = axes[1]
    for i, method in enumerate(methods):
        vals = [data[method].get(s, 0) for s in sizes]
        if any(v > 0 for v in vals):
            ax2.plot(sizes, vals, 'o-', label=method, color=colors[i % len(colors)],
                     linewidth=2, markersize=6)

    ax2.set_xlabel('Buffer Size (bytes)')
    ax2.set_ylabel('Throughput (MB/s)')
    ax2.set_title('ESP32 Memory Copy — Throughput vs Buffer Size')
    ax2.set_xscale('log', base=2)
    ax2.legend(fontsize=8)
    ax2.grid(True, alpha=0.3)

    plt.tight_layout()
    plt.savefig('mem_benchmark_results.png', dpi=150)
    print("Chart saved: mem_benchmark_results.png")
    plt.show()


def print_summary(results):
    """Print text summary of results."""
    print("\n" + "=" * 70)
    print("  ESP32 Memory Copy Benchmark — Summary")
    print("=" * 70)

    method_map = {
        'ForLoop': 'For-Loop',
        'Memcpy': 'memcpy',
        'IRAM': 'IRAM',
        'SPIRAM': 'SPIRAM',
        'SPI_DMA': 'SPI DMA',
    }

    sizes = sorted(set(r['size'] for r in results))
    methods = []
    seen = set()
    for r in results:
        if r['method'] not in seen:
            methods.append(r['method'])
            seen.add(r['method'])

    # Header
    header = f"{'Method':<18}"
    for s in sizes:
        header += f"  {s:>7}B"
    print(header)
    print("-" * len(header))

    for method in methods:
        row = f"{method_map.get(method, method):<18}"
        for s in sizes:
            match = [r for r in results if r['method'] == method and r['size'] == s]
            if match and not match[0]['skipped']:
                row += f"  {match[0]['throughput']:>7.2f}"
            else:
                row += f"      N/A"
        print(row)

    print("=" * 70)


def main():
    parser = argparse.ArgumentParser(description='ESP32 Memory Benchmark Debug Tool')
    parser.add_argument('--port', default='/dev/ttyUSB0', help='Serial port')
    parser.add_argument('--baud', type=int, default=115200, help='Baud rate')
    parser.add_argument('--file', help='Read from log file instead of serial')
    parser.add_argument('--duration', type=int, default=120, help='Read duration (s)')
    args = parser.parse_args()

    if args.file:
        lines = read_from_file(args.file)
    else:
        lines = read_from_serial(args.port, args.baud, args.duration)

    results = parse_all(lines)

    if not results:
        print("No benchmark results found in output!")
        sys.exit(1)

    print(f"\nParsed {len(results)} benchmark entries.")
    print_summary(results)
    create_charts(results)


if __name__ == '__main__':
    main()
