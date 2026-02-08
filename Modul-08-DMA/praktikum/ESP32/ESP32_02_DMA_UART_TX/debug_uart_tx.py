#!/usr/bin/env python3
"""
===========================================================================
 Debug Script: ESP32 UART TX Benchmark — Parser & Visualization
===========================================================================
 Parses serial output from ESP32_02_DMA_UART_TX,
 creates throughput comparison charts.

 Usage:
   python debug_uart_tx.py [--port /dev/ttyUSB0] [--baud 115200]
   python debug_uart_tx.py --file output.log
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
    """Parse a [BENCH] line for UART TX benchmark."""
    pattern = (
        r'\[BENCH\]\s+Method=(\w+)\s+Size=(\d+)\s+'
        r'TotalTime=(\d+)\s+us\s+Throughput=([\d.]+)\s+kbps'
    )
    m = re.search(pattern, line)
    if m:
        return {
            'method': m.group(1),
            'size': int(m.group(2)),
            'time_us': int(m.group(3)),
            'throughput_kbps': float(m.group(4)),
        }
    return None


def read_from_serial(port, baud, duration=120):
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
    """Parse all benchmark results."""
    results = []
    for line in lines:
        r = parse_bench_line(line)
        if r:
            results.append(r)
    return results


def create_charts(results):
    """Create UART TX throughput comparison charts."""
    if not HAS_PLOT:
        print("WARNING: matplotlib not installed. Skipping charts.")
        return

    method_map = {
        'write_bytes': 'uart_write_bytes()',
        'tx_chars': 'uart_tx_chars()',
        'printf_uart0': 'printf (UART0)',
    }

    sizes = sorted(set(r['size'] for r in results))
    methods = []
    seen = set()
    for r in results:
        if r['method'] not in seen:
            methods.append(r['method'])
            seen.add(r['method'])

    fig, axes = plt.subplots(1, 2, figsize=(14, 5))
    colors = ['#2ecc71', '#e74c3c', '#3498db']

    # Chart 1: Bar chart
    ax = axes[0]
    x = np.arange(len(sizes))
    width = 0.25

    for i, method in enumerate(methods):
        vals = []
        for s in sizes:
            match = [r for r in results if r['method'] == method and r['size'] == s]
            vals.append(match[0]['throughput_kbps'] if match else 0)
        ax.bar(x + i * width, vals, width,
               label=method_map.get(method, method),
               color=colors[i % len(colors)])

    ax.set_xlabel('Data Size (bytes)')
    ax.set_ylabel('Throughput (kbps)')
    ax.set_title('ESP32 UART TX — Throughput Comparison')
    ax.set_xticks(x + width)
    ax.set_xticklabels([str(s) for s in sizes])
    ax.axhline(y=115.2, color='gray', linestyle='--', alpha=0.7, label='Theoretical Max')
    ax.legend(fontsize=8)
    ax.grid(axis='y', alpha=0.3)

    # Chart 2: Efficiency (% of theoretical max)
    ax2 = axes[1]
    theoretical_max = 115.2  # kbps at 115200 baud

    for i, method in enumerate(methods):
        vals = []
        for s in sizes:
            match = [r for r in results if r['method'] == method and r['size'] == s]
            if match:
                eff = (match[0]['throughput_kbps'] / theoretical_max) * 100
                vals.append(min(eff, 100))
            else:
                vals.append(0)
        ax2.bar(x + i * width, vals, width,
                label=method_map.get(method, method),
                color=colors[i % len(colors)])

    ax2.set_xlabel('Data Size (bytes)')
    ax2.set_ylabel('Efficiency (%)')
    ax2.set_title('ESP32 UART TX — Efficiency vs Theoretical Max')
    ax2.set_xticks(x + width)
    ax2.set_xticklabels([str(s) for s in sizes])
    ax2.set_ylim(0, 110)
    ax2.axhline(y=100, color='gray', linestyle='--', alpha=0.7)
    ax2.legend(fontsize=8)
    ax2.grid(axis='y', alpha=0.3)

    plt.tight_layout()
    plt.savefig('uart_tx_benchmark.png', dpi=150)
    print("Chart saved: uart_tx_benchmark.png")
    plt.show()


def main():
    parser = argparse.ArgumentParser(description='ESP32 UART TX Benchmark Debug Tool')
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
        print("No benchmark results found!")
        sys.exit(1)

    print(f"\nParsed {len(results)} entries.")
    create_charts(results)


if __name__ == '__main__':
    main()
